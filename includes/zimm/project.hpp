#pragma once

#include "config.hpp"
#include "installer.hpp"
#include "target.hpp"
#include "tester.hpp"
#include <cassert>
#include <initializer_list>
#include <optional>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace zimm
{

class Project
{
    std::string m_name;
    Config m_config;
    std::vector<Target *> m_topLevelTargets;
    std::vector<PropertyObject> m_globalProperties;

public:
    Project(std::string name, Config config,
            std::source_location mainFile = std::source_location::current());

    std::string_view name() const noexcept { return m_name; }
    const Config &config() const noexcept { return m_config; }
    std::span<const PropertyObject> global_properties() const noexcept
    {
        return m_globalProperties;
    }
    std::string_view main_file_path() const noexcept { return m_mainFilePath; }
    Installer &installer() noexcept { return m_installer; }
    Tester &tester() noexcept { return m_tester; }

    void register_top_level_target(Target *target) { m_topLevelTargets.emplace_back(target); }
    void register_top_level_targets(std::initializer_list<Target *> targets)
    {
        for (auto *target : targets) register_top_level_target(target);
    }
    std::span<Target *const> top_level_targets() const noexcept { return m_topLevelTargets; }
    void add_global_property(PropertyObject property)
    {
        m_globalProperties.push_back(std::move(property));
    }

    std::string_view build_dir() { return m_config.build_dir; }

    std::unordered_set<Target *> seach_all_targets() const;

    // feature detection
    bool try_compile(std::string source, bool link);
    std::optional<std::string> try_run(std::string source);
    bool check_header(std::string header);
    bool check_function_exists(std::string function_name);
    bool check_symbol_exists(std::string symbol, std::vector<std::string> headers);
    std::optional<size_t> check_type_size(std::string type, std::vector<std::string> headers);

private:
    Directory m_featureDetectionDir;

    Installer m_installer;
    Tester m_tester;

    std::string m_mainFilePath;
};

void generate_build(Project &project);

} // namespace zimm
