#pragma once

#include "definitions.hpp"
#include "detail/utils.hpp"
#include "path.hpp"
#include "properties.hpp"

#include <span>
#include <string>
#include <vector>

namespace zimm
{
enum class TargetType
{
    Executable,
    StaticLibrary,
    SharedLibrary,
    ThirdPartyTarget,
    CustomTarget
};

std::string to_string(const TargetType &);
std::string to_string(const Target &);

class Target
{
    const TargetType m_type;
    const std::string m_name;

    std::vector<Target *> m_dependsOn;      // Target depends on all these targets
    std::vector<Target *> m_isDependencyOf; // These targets depend on Target

    std::vector<PropertyObject> m_publicProperties;
    std::vector<PropertyObject> m_privateProperties;

    void add_property_impl(std::vector<PropertyObject> &properties, PropertyObject property);

protected:
    Target(TargetType type, std::string name) noexcept : m_type(type), m_name(std::move(name)) {}
    friend void add_dependency_rel(Target *target, Target *dependency);

public:
    virtual ~Target() = default;

    std::string_view name() const noexcept { return m_name; }
    TargetType type() const noexcept { return m_type; }

    // clang-format off
    std::span<Target * const> dependencies() const noexcept { return m_dependsOn; }
    std::span<Target * const> dependents() const noexcept { return m_isDependencyOf; }

    std::span<const PropertyObject> public_properties() const noexcept { return m_publicProperties; }
    std::span<const PropertyObject> private_properties() const noexcept { return m_privateProperties; }
    // clang-format on

    void add_property(const PublicTag *, PropertyObject property)
    {
        if (property->type() == PropertyType::AssumedProperty)
            LOGW("Setting ASSUMED as public property");
        add_property_impl(m_publicProperties, property);
    }
    void add_property(const PrivateTag *, PropertyObject property)
    {
        add_property_impl(m_privateProperties, property);
    }
};

namespace detail
{
class Sources
{
    std::vector<std::string> m_sources;

public:
    void add_source(std::string source)
    {
        if (!utils::is_valid_source(source)) LOGF("invalid source = " << std::quoted(source));
        m_sources.push_back(std::move(source));
    }
    std::span<const std::string> sources() const noexcept { return m_sources; }

    void link_with(const PublicTag *, Library *linkLib);
    void link_with(const PrivateTag *, Library *linkLib);
    virtual ~Sources() = default;
};

struct CustomTargetBase
{
    virtual std::string generate_cmd() const = 0;
    virtual std::span<const std::string> inputs() const = 0;
    virtual std::span<const std::string> outputs() const = 0;
    virtual const Directory &dir() const = 0;
};
} // namespace detail

class Library : public Target, public detail::Sources
{
protected:
    Library(TargetType type, std::string name) noexcept : Target(type, std::move(name)) {}
};

class Executable : public Target, public detail::Sources
{
public:
    Executable(std::string name) noexcept : Target(TargetType::Executable, std::move(name)) {}
};

class StaticLibrary : public Library
{
public:
    explicit StaticLibrary(std::string name) noexcept
        : Library(TargetType::StaticLibrary, std::move(name))
    {
    }
};

class SharedLibrary : public Library
{
public:
    explicit SharedLibrary(std::string name) noexcept
        : Library(TargetType::SharedLibrary, std::move(name))
    {
        add_property(private_, CompileFlagProperty{"-fPIC"});
    }
};

inline LeakyPtr<Executable> make_executable(std::string name)
{
    return LeakyPtr<Executable>(new Executable{std::move(name)});
}

inline LeakyPtr<StaticLibrary> make_static_library(std::string name)
{
    return LeakyPtr<StaticLibrary>(new StaticLibrary{std::move(name)});
}

inline LeakyPtr<SharedLibrary> make_shared_library(std::string name)
{
    return LeakyPtr<SharedLibrary>(new SharedLibrary{std::move(name)});
}

class ThirdPartyTarget : public Target
{
    Directory m_thirdPartyBuildDir;
    std::string m_metaBuildCmd;
    std::string m_buildCmd;

public:
    LeakyPtr<Executable> assume_executable(std::string name,
                                           std::string pathRelToThirdPartyBuildDir)
    {
        auto target = make_executable(std::move(name));
        add_dependency_rel(target.get(), this);
        target->add_property(
            private_, AssumedProperty{m_thirdPartyBuildDir.make(pathRelToThirdPartyBuildDir)});
        return target;
    }

    LeakyPtr<StaticLibrary> assume_static_library(std::string name,
                                                  std::string pathRelToThirdPartyBuildDir)
    {
        auto target = make_static_library(std::move(name));
        add_dependency_rel(target.get(), this);
        target->add_property(
            private_, AssumedProperty{m_thirdPartyBuildDir.make(pathRelToThirdPartyBuildDir)});
        return target;
    }

    LeakyPtr<SharedLibrary> assume_shared_library(std::string name,
                                                  std::string pathRelToThirdPartyBuildDir)
    {
        auto target = make_shared_library(std::move(name));
        add_dependency_rel(target.get(), this);
        target->add_property(
            private_, AssumedProperty{m_thirdPartyBuildDir.make(pathRelToThirdPartyBuildDir)});
        return target;
    }

    ThirdPartyTarget(std::string name, Directory buildSubDir)
        : Target(TargetType::ThirdPartyTarget, std::move(name)),
          m_thirdPartyBuildDir(std::move(buildSubDir))
    {
    }

    void set_meta_build_cmd(std::string metaBuildCmd) { m_metaBuildCmd = std::move(metaBuildCmd); }
    void set_build_cmd(std::string buildCmd) { m_buildCmd = std::move(buildCmd); }
    std::string_view meta_build_cmd() const noexcept { return m_metaBuildCmd; }
    std::string_view build_cmd() const noexcept { return m_buildCmd; }
    std::string_view build_dir() const noexcept { return m_thirdPartyBuildDir.path(); }
};

inline LeakyPtr<ThirdPartyTarget> make_third_party_target(std::string name, Directory buildSubDir)
{
    return LeakyPtr<ThirdPartyTarget>(
        new ThirdPartyTarget(std::move(name), std::move(buildSubDir)));
}

template <typename T>
concept CustomTargetTagConcept = requires {
    { T::name() } -> std::same_as<std::string_view>;
    {
        T::cmd(std::declval<std::span<const std::string>>() /* inputs */,
               std::declval<std::span<const std::string>>() /* outputs */)
    } -> std::same_as<std::string>;
};

template <CustomTargetTagConcept CustomTargetTag>
class CustomTarget : public Target, public detail::CustomTargetBase
{
    std::vector<std::string> m_inputs;
    std::vector<std::string> m_outputs;
    Directory m_dir;

public:
    CustomTarget(std::string name)
        : Target(TargetType::CustomTarget, std::move(name)),
          m_dir(std::string{build_dir()} + std::string{CustomTargetTag::name()} + "_" +
                std::string{this->name()})
    {
    }

    void add_input(std::string input) { m_inputs.push_back(std::move(input)); }
    // TODO: fix the paths
    void add_output(std::string output) { m_outputs.push_back(std::move(output)); }

    std::string generate_cmd() const override { return CustomTargetTag::cmd(m_inputs, m_outputs); }
    std::span<const std::string> inputs() const override { return m_inputs; }
    std::span<const std::string> outputs() const override { return m_outputs; }
    const Directory &dir() const noexcept override { return m_dir; }
};

template <CustomTargetTagConcept T>
inline LeakyPtr<CustomTarget<T>> make_custom_target(std::string name)
{
    return LeakyPtr<CustomTarget<T>>(new CustomTarget<T>{std::move(name)});
}

} // namespace zimm
