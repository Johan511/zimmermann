# Zimmermann - Still in Beta

Build system to declare project hierarchy using C++

## Build

```bash
./run_build.sh --clean
```

## Test

```bash
python3 example/test.py
```

# How it works?

## Targets and Properties

zimmermann uses the concept of `Target` and `Property`.

- Targets are statically allocated move-only objects which mirror the build targets.
- Properties are locally allocated objects used to inject behaviour into targets.

Target and Property are closed-polymorphic sets of the following types

### Targets
- **Executable**
- **StaticLibrary**
- **SharedLibrary**
- **ThirdPartyTarget**: Used to introduce third party dependencies into your project
- **CustomTarget**: Used to add targets with configurable command, inputs and outputs

### Properties
- **IncludeProperty**: Used to include a certain directory to a certain target
- **CompileFlagProperty**: Used to set extra compile flags to a certain target
- **LinkFlagProperty**: Used to set extra link flags to a certain target

## Config

Generated using the `parse_config(int argc, char *argv[])` factory and allows to inject behaviour into the project when executing ze_build

Usage:

```sh
./ze_build build_type=release cxx_flags=-march=native install_dir=/usr/
```

The defined parameters can be accessed as `cfg.build_type`, `cfg.install_dir`, ...

Config also supports miscellenous parameters with string key-value arguments

Example:
```sh
./ze_build asio_path=/opt/asio/install
```

And can be accessed in the zimmermann program as
```cpp
Config cfg = parse_config(argc, argv);
cfg.misc["asio_path"] // Config::misc is std::unordered_map<std::string, std::string>
```

## Project monolith

The project is the roots of zimmermann and contains a lot of useful features dumped together for simplicity without any concern for SRP.

```cpp
Project project("ProjectName", std::move(config))
project.add_global_property(/*property*/);

Installer &installer = project.installer();
Tester &tester = project.tester();
project.register_top_level_target(/*top level target*/)

// Feature Detection
bool compilesAndLinks = project.try_compile(/* string meant to be compiled */, /*link=*/true);
std::optional<std::string> runStdOut = project.try_run(/* string meant to be run */);
bool headerExists = project.check_header("stdio");
bool funcExists = project.check_function_exists("fork");
bool symbolExists = project.check_symbol_exists("stat", {"sys/stat.h"});
std::optional<size_t> typeSizeOpt = project.check_type_size("ptrdiff_t", {"stddef.h"});

// helpers
std::string_view buildDir = project.build_dir();
std::string_view installDir = project.install_dir();
```

## Installer

`Installer` is a utility to setup the install directory

It works by managing a key value map.
- value: the path (file or directory) to be copied into install directory
- key: the subDir in the install directory where the path specified by value should be copied

```cpp
Installer &installer = project.installer();

project.install_binary(/* Executable */); // key="bin", value=executable_path
project.install_lib(/* Static or Shared library */); // key="lib", value=executable_path
project.install_headers(/* directory to be copied */,
                        /* subdirectory in {INSTALL_DIR}/include it should be copied into */);
```

## Tester

`Tester` is a utility to register tests associated with the project

```cpp
Tester& tester = project.tester();
tester->add_test(executable, /* args to be passed */);
```

- executable needs to seperately be registered with project to ensure it is built
- args can be of any type as long as they support `operator<<(std::ostream&, const Arg&)`
- The command executed is `./executable <space seperated stringified args>`

The tests can be executed using
- `ninja test`: executes all tests
- `ninja <executable name>`: executes all tests associated with that executable

## generate_build(Project&)

generate_build is the function to be called at the end of `int main() {}` and is responsible for generating the ninja build file

## Feature Detection

`Project` exposes feature detection helpers to configure the build.
The checks respects the project's global properties (includes, compile flags, link flags) added via `add_global_property`, and is carried out in a scratch directory.

```cpp
bool compilesAndLink = project.try_compile(/* string meant to be compiled */, /*link=*/true);
std::optional<std::string> runStdOut = project.try_run(/* string meant to be run */);

bool headerExists = project.check_header("stdio");
bool funcExists = project.check_function_exists("fork");
bool symbolExists = project.check_symbol_exists("stat", {"sys/stat.h"});
std::optional<size_t> typeSizeOpt = project.check_type_size("ptrdiff_t", {"stddef.h"});
```

- `try_compile` compiles a source snippet and optionally links it; `try_run` further executes it and returns the program's stdout.
- `check_header` / `check_function_exists` / `check_symbol_exists` / `check_type_size` are conveniences built on top of the two primitives.
- Results are typically forwarded to targets as compile flags, e.g. `app->add_property(private_, CompileFlagProperty{"-DHAVE_CSTDIO"})`.

## Utilities

1. **`rel_path(std::string)`**: Generate a path relative to the directory containing the file
2. **`zimm_dir()`**: Returns the directory where zimmermann has been installed (figures out from where the headers are found by the compiler)

## Third Party Target
C++ package management is an operational debacle, we try our best to keep it intuitive at the cost of making it a bit verbose.

`ThirdPartyTarget` (TPT) is a concrete Target type which is defined by the tuple `(name, directory, metaBuildCmd, buildCmd)`.

- `metaBuildCmd` is the cmd which runs only once during setup (ie when ze_build is executed), it is not run on every build invocation
- `buildCmd` is the cmd which runs on every execution to build TPT if any of its dependencies are modified
- `directory` is the directory in which `metaBuildCmd` and `buildCmd` are executed

Taking note of the fact that manually figuring out the `metaBuildCmd`, `buildCmd`, `directory` could be too tiring we provide factories to generate them.

```cpp
template <typename Strategy>
concept ThirdPartyTargetStrategy = requires(const Strategy &s) {
    // Strong failure safety -> if it fails and returns nullptr, there are no visible changes
    { s.attempt(std::string_view{} /* name */) } -> std::same_as<LeakyPtr<class ThirdPartyTarget>>;
};
```

`ThirdPartyTarget::make` has two overloads:

**1. Strategy-based overload** — tries each strategy in order, returns the TPT from the first one that succeeds:
```cpp
template <ThirdPartyTargetStrategy... Strategies>
static LeakyPtr<ThirdPartyTarget> make(std::string name, const Strategies &...strategies);
```
If no strategy succeeds, logs a message and returns `nullptr`.

**2. Direct-construction overload** — creates a TPT with explicit parameters:
```cpp
static LeakyPtr<ThirdPartyTarget> make(std::string name, Directory dir,
                                       MetaBuildCmd metaBuildCmd = {}, BuildCmd buildCmd = {});
```

We provide the following strategies based on popular package management approaches

### FindPackageTptStrat

Attempts to find the installed package from among the paths provided in its ctor.

```cpp
FindPackageTptStrat(std::string searchPath);
FindPackageTptStrat(std::vector<std::string> searchPaths);
FindPackageTptStrat() : FindPackageTptStrat(/* default zimmermann search paths */);
```

`attempt(std::string_view tptName)` searches all the directories in the search path to find if they have a child directory named `tptName` or if they themselves are called `tptName`

### FetchContentTptStrategy

Attempts to fetch the library from the given url and build it using the given metaBuildCmd and buildCmd.

```cpp
FetchContentTptStrategy(Directory dir,
                        std::string fetchContentCmd,
                        MetaBuildCmd metaBuildCmd,
                        BuildCmd buildCmd);
```

Example Usage:
```cpp
// cmake setup at ze_build execution, build library which building using generator
FetchContentTptStrategy{build_dir("googletest"), fetchContentCmd, MetaBuildCmd{"cmake -S . -B build"}, BuildCmd{"cmake --build build"}}

// cmake setup, package build and install - all during ze_build execution
FetchContentTptStrategy{build_dir("googletest"), fetchContentCmd, MetaBuildCmd{"cmake -S . -B build && cmake --build build && cmake --install build"}}

// fetch the package directly and unpack it
FetchContentTptStrategy{build_dir(), "wget https://archives.boost.io/release/1.91.0/source/boost_1_91_0.tar.gz", tarUnpackCmd, BuildCmd{""}}
```

The final `metaBuildCmd` of the `ThirdPartyTarget` is set as `{fetchContentCmd} && {metaBuildCmd}`. The fetch command is **deferred** to meta-build time (not run eagerly), so `FetchContentTptStrategy::attempt` always returns a TPT. This is done so `attempt` can assure strong failure safety.

If you don't want to manually write the fetchContentCmd, zimmermann provides the following utilities for that
- `git_fetch(const Directory &dir, std::string_view url, std::string_view id)`. Example:
  ```cpp
  fetchContentCmd = git_fetch(build_dir("json"), "git@github.com:nlohmann/json.git", "ad94fb0" /* sha-hash */)
  fetchContentCmd = git_fetch(build_dir("json"), "git@github.com:nlohmann/json.git", "master" /* branch name */)
  fetchContentCmd = git_fetch(build_dir("json"), "git@github.com:nlohmann/json.git", "v3.12.0" /* tag */)

  // explicitly state that this is a tag, and not to be confused with a branch
  fetchContentCmd = git_fetch(build_dir("json"), "git@github.com:nlohmann/json.git", "tag v3.12.0" /* tag */)
  ```

## Recommended usage instructions

Create a file `ze_build.cpp` with the `int main(int argc, char *argv[])` method - this will be the .cpp file which will be compiled into the `ze_build` executable which generates the `ninja` build instructions.

Compiling `ze_build.cpp`

```sh
g++ -std=c++20 ze_build.cpp /path/to/zimm/lib64/libzimmermann.a -I /path/to/zimm/include -o build/ze_build
```

On executing ze_build, the ninja build file is created in the build directory (defaults to cwd if not set in config).

We recommend using per directory `ze_build.hpp` files which contains a factory function to build targets associated with that directory, this `ze_build.hpp` can be included into `ze_build.cpp`.

**NOTE**: targets have static lifetime, the user doesn't need to burden themselves with managing lifetime of targets.

Using the library requires atleast C++20, building and developed requires C++26
