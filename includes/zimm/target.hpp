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

class ThirdPartyTarget;
namespace detail
{

class AssumedTrait
{
    friend class zimm::ThirdPartyTarget;
    std::string m_assumedPath;

public:
    std::string_view assumed_path() const noexcept { return m_assumedPath; }
};

class SourcesTrait
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
    virtual ~SourcesTrait() = default;
};

struct CustomTargetBase
{
    virtual std::string generate_cmd() const = 0;
    virtual std::span<const std::string> inputs() const = 0;
    virtual std::span<const std::string> outputs() const = 0;
    virtual const Directory &dir() const = 0;
};

} // namespace detail

class Target : public detail::AssumedTrait
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
        add_property_impl(m_publicProperties, property);
    }
    void add_property(const PrivateTag *, PropertyObject property)
    {
        add_property_impl(m_privateProperties, property);
    }
};

class Library : public Target, public detail::SourcesTrait
{
protected:
    Library(TargetType type, std::string name) noexcept : Target(type, std::move(name)) {}
};

class Executable : public Target, public detail::SourcesTrait
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// CUSTOM TARGET

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
    CustomTarget(std::string name, Directory dir)
        : Target(TargetType::CustomTarget, std::move(name)), m_dir(std::move(dir))
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// FACTORIES

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

template <CustomTargetTagConcept T>

inline LeakyPtr<CustomTarget<T>> make_custom_target(std::string name, Directory dir)
{
    return LeakyPtr<CustomTarget<T>>(new CustomTarget<T>{std::move(name), std::move(dir)});
}

} // namespace zimm
