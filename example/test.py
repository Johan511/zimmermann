import os
import shutil
import subprocess
import sys
import shutil
import time

EXAMPLES_DIR = os.path.dirname(os.path.abspath(__file__))

EXAMPLES = [
    {
        "name": "hello_world",
        "binary": "bin/HelloWorld",
        "expected": ["Hello World from include!"],
        "install": [
            "bin/HelloWorld",
            "include/messages.hpp",
        ],
        "tests": ["HelloWorld"]
    },
    {
        "name": "complex",
        "binary": "bin/myapp",
        "expected": [
            "[INFO] Complex example starting",
            "Vec3(3, 4, 0)",
            "[INFO] Vector length: 5.00",
            "[INFO] Connecting to localhost:8080",
            "Sent 22 bytes to localhost:8080",
            "[INFO] Disconnecting",
            "[INFO] Complex example finished",
        ],
        "install": [
            "bin/myapp",
            "lib/libnetwork.so",
            "include/core/logging.hpp",
            "include/math/vec3.hpp",
            "include/network/socket.hpp",
        ],
    },
    {
        "name": "feature_detection",
        "binary": "bin/feature_detect",
        "expected": [
            "HAVE_CSTDIO=1",
            "HAVE_NONEXISTENT_HEADER=0",
            "HAVE_PRINTF=1",
            "HAVE_NONEXISTENT_FUNC=0",
            "HAVE_O_RDONLY=1",
            "INT_SIZE=4",
            "LLONG_SIZE=8",
            "PTR_SIZE=8",
        ],
        "install": [
            "bin/feature_detect",
        ],
    },
    {
        "name": "third_party_target",
        "binary": "bin/a",
        "expected": ["43"],
        "install": [
            "bin/a",
        ],
    },
    {
        "name": "ze_test",
        "binary": "bin/ze_test",
        "expected": ["Test runner example"],
        "install": [
            "bin/ze_test",
        ],
        "tests": ["test_pass", "test_args"]
    },
]


def build_zimm(test_dir: str) -> str | None:
    """Compile ze_build.cpp in *test_dir* and return the path to the built binary.

    Returns the absolute path to the ze_build executable on success, or None on failure.
    """
    resolved = os.path.abspath(test_dir)
    source = os.path.join(resolved, "ze_build.cpp")
    build_dir = os.path.join(resolved, "build")

    if os.path.isdir(build_dir):
        shutil.rmtree(build_dir)
    os.makedirs(build_dir)

    repo_root = os.path.dirname(EXAMPLES_DIR)
    includes = os.path.join(repo_root, "includes")
    lib_dir = os.path.join(repo_root, "build")

    ze_bin = os.path.join(build_dir, "ze_build")
    cmd = [
        "g++", source,
        "-std=c++23",
        f"-I{includes}",
        f"-L{lib_dir}", "-lzimmermann",
        "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-o", ze_bin,
    ]
    result = subprocess.run(cmd, cwd=resolved)
    return True if result.returncode == 0 else False

def run_zimm(test_build_dir: str) -> int | None:
    ze_bin = os.path.join(test_build_dir, "ze_build")
    if not os.path.isfile(ze_bin):
        return False
    log_path = os.path.join(test_build_dir, "zimm.log")
    with open(log_path, "w") as log:
        result = subprocess.run([ze_bin], stdout=log, stderr=subprocess.STDOUT, cwd=test_build_dir)
    return True if result.returncode == 0 else False 


def run_ninja(build_dir: str) -> bool:
    log_path = os.path.join(build_dir, "build.log")
    with open(log_path, "w") as log:
        result = subprocess.run(
            ["ninja", "-C", build_dir],
            stdout=log,
            stderr=subprocess.STDOUT,
        )
    if result.returncode != 0:
        print(f"ninja failed with exit code {result.returncode}", file=sys.stderr)
        return False
    return True


def verify_install(install_dir: str, install: list[str]) -> bool:
    for path in install:
        full = os.path.join(install_dir, path)
        if not os.path.exists(full):
            print(f"error: installed file not found: {full}", file=sys.stderr)
            return False
    return True


def run_binary(install_dir: str, binary: str) -> tuple[str, int] | None:
    bin_path = os.path.join(install_dir, binary)
    if not os.path.isfile(bin_path):
        print(f"error: binary not found: {bin_path}", file=sys.stderr)
        return None

    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = os.path.join(install_dir, "lib")
    result = subprocess.run([bin_path], capture_output=True, text=True, cwd=install_dir, env=env)
    return result.stdout if result.returncode == 0 else None

def run_test(build_dir: str, tests: list[str]) -> bool:
    for t in tests:
        result = subprocess.run(
            ["ninja", "-C", build_dir, f"test-{t}"],
            stdout=subprocess.DEVNULL,
            cwd=build_dir,
        )
        if result.returncode != 0:
            return False
    result = subprocess.run(
        ["ninja", "-C", build_dir, "test"],
        stdout=subprocess.DEVNULL,
        cwd=build_dir,
    )
    return result.returncode == 0

def run_ninja_install(build_dir: str) -> bool:
    log_path = os.path.join(build_dir, "install.log")
    with open(log_path, "w") as log:
        result = subprocess.run(
            ["ninja", "-C", build_dir, "install"],
            stdout=log,
            stderr=subprocess.STDOUT,
        )
    if result.returncode != 0:
        print(f"ninja install failed with exit code {result.returncode}", file=sys.stderr)
        return False
    return True

def check_output(stdout: str, expected: list[str]) -> bool:
    rest = stdout
    for line in expected:
        idx = rest.find(line)
        if idx == -1:
            print(f"error: expected line not found in output: {line!r}", file=sys.stderr)
            print(stdout, file=sys.stderr)
            return False
        rest = rest[idx + len(line):]
    return True

def test_example(example: dict, perf: bool = False) -> bool:
    """Build, run, and verify a single example.  Returns True on success."""
    name = example["name"]
    binary = example["binary"]
    expected = example["expected"]
    test_dir = os.path.join(EXAMPLES_DIR, name)
    build_dir = os.path.join(test_dir, "build")
    install_dir = os.path.join(test_dir, "install")

    shutil.rmtree(install_dir, ignore_errors=True)
    shutil.rmtree(build_dir, ignore_errors=True)

    t0 = time.perf_counter()
    if not build_zimm(test_dir):
        print(f"FAIL  {name}: build_zimm")
        return False
    t1 = time.perf_counter()

    if not run_zimm(build_dir):
        print(f"FAIL  {name}: run_zimm")
        return False
    t2 = time.perf_counter()

    if not run_ninja(build_dir):
        print(f"FAIL  {name}: ninja")
        return False
    t3 = time.perf_counter()

    if not run_ninja_install(build_dir):
        print(f"FAIL  {name}: ninja install")
        return False
    t4 = time.perf_counter()

    if not verify_install(install_dir, example.get("install", [])):
        print(f"FAIL  {name}: install verification")
        return False

    stdout = run_binary(install_dir, binary)
    if stdout is None:
        print(f"FAIL  {name}: binary not found or exited with non-zero error")
        return False
    t5 = time.perf_counter()

    if not check_output(stdout, expected):
        print(f"FAIL  {name}: output mismatch")
        return False
    t6 = time.perf_counter()

    if("tests" in example):
        if not run_test(build_dir, example["tests"]):
            print(f"FAIL  {name}: ninja test failed")
            return False

    msg = f"PASS  {name}"
    if perf:
        parts = [
            msg,
            f"  build_zimm:        {(t1 - t0) * 1000:.1f}ms",
            f"  run_zimm:          {(t2 - t1) * 1000:.1f}ms",
            f"  run_ninja:         {(t3 - t2) * 1000:.1f}ms",
            f"  run_ninja_install: {(t4 - t3) * 1000:.1f}ms",
            f"  run_binary:        {(t5 - t4) * 1000:.1f}ms",
            f"  check_exec:        {(t6 - t5) * 1000:.1f}ms",
        ]
        print("\n".join(parts))
    else:
        print(msg)
    return True


def main() -> int:
    examples = EXAMPLES
    if "--ex" in sys.argv:
        idx = sys.argv.index("--ex")
        if idx + 1 < len(sys.argv) and sys.argv[idx + 1]:
            selected = set(sys.argv[idx + 1].split(","))
            examples = [ex for ex in EXAMPLES if ex["name"] in selected]
            if not examples:
                print(f"error: no matching examples for --ex {sys.argv[idx + 1]!r}", file=sys.stderr)
                return 1

    perf = "--perf" in sys.argv

    failures = 0
    for ex in examples:
        if not test_example(ex, perf=perf):
            failures += 1

    total = len(examples)
    if failures == 0:
        print(f"all {total} example(s) passed")
    else:
        print(f"{failures}/{total} example(s) FAILED")
    return failures


if __name__ == "__main__":
    sys.exit(main())
