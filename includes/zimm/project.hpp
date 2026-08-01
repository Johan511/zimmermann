#pragma once

#include "config.hpp"
#include "installer.hpp"
#include "target.hpp"
#include "tester.hpp"
#include <cassert>
#include <filesystem>
#include <optional>
#include <queue>
#include <source_location>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
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

    std::unordered_set<const Target *> all_targets() const
    {
        std::unordered_set<const Target *> seen;
        {
            // check that top level targets unique
            for (const Target *t : m_topLevelTargets)
                if (!seen.emplace(t).second) LOGF("Duplicate top level target");
            seen.clear();
        }

        std::stack<const Target *> stk;
        stk.push_range(m_topLevelTargets);

        while (!stk.empty())
        {
            const Target *top = stk.top();
            stk.pop();

            if (seen.contains(top)) continue;
            seen.insert(top);

            for (auto dep : top->dependencies())
                if (!seen.contains(dep)) stk.push(dep);
        }

        return seen;
    }

    class SourceLoc : private std::source_location
    {
        friend class Project;
        SourceLoc(std::source_location sl) : std::source_location::source_location(sl) {}
    };

public:
    Project(std::string name, Config config, SourceLoc mainFile = std::source_location::current())
        : m_name(std::move(name)), m_config(std::move(config)),
          m_featureDetectionDir(m_config.build_dir), m_installer(m_config.build_dir),
          m_mainFilePath(mainFile.file_name())
    {
        namespace fs = std::filesystem;

        fs::create_directories(m_config.build_dir);
        fs::create_directories(fs::path{m_featureDetectionDir.path()});
    }

    // clang-format off
    std::string_view name() const noexcept { return m_name; }
    const Config &config() const noexcept { return m_config; }
    std::span<const PropertyObject> global_properties() const noexcept { return m_globalProperties; }
    std::string_view main_file_path() const noexcept { return m_mainFilePath; }
    Installer &installer() noexcept { return m_installer; }
    Tester &tester() noexcept { return m_tester; }

    void register_top_level_target(Target *target) { m_topLevelTargets.emplace_back(target); }
    std::span<Target *const> top_level_targets() const noexcept { return m_topLevelTargets; }
    void add_global_property(PropertyObject property) { m_globalProperties.push_back(property); }
    // clang-format on

    std::string_view build_dir() { return m_config.build_dir; }

    auto fold_post_order(auto init, auto foo)
    {
        std::unordered_set<const Target *> allTargets = all_targets();
        std::queue<const Target *> visitQueue;
        std::unordered_map<const Target *, size_t> inDegreeMap;

        for (const Target *t : allTargets)
        {
            auto numDeps = t->dependencies().size();
            if (numDeps == 0) visitQueue.push(t);
            inDegreeMap[t] = numDeps;
        }

        std::vector<Target *> topologicalOrder;
        while (!visitQueue.empty())
        {
            auto front = visitQueue.front();
            visitQueue.pop();

            topologicalOrder.push_back(const_cast<Target *>(front));
            for (const Target *dependent : front->dependents())
                if (--inDegreeMap[dependent] == 0) visitQueue.push(dependent);
        }

        if (topologicalOrder.size() != allTargets.size())
            LOGF("fold failed because graph has cycles");

        for (auto *t : topologicalOrder) init = foo(std::move(init), t);
        return init;
    }

    auto fold_post_order(auto foo)
    {
        static_assert(
            requires { foo(static_cast<Target *>(NULL)); }, "foo must be callable with Target *");
        static_assert(std::is_same_v<decltype(foo(NULL)), void>);

        auto wrapped_foo = [foo = std::forward<decltype(foo)>(foo)](std::nullptr_t, Target *t)
        {
            foo(t);
            return nullptr;
        };

        return fold_post_order(nullptr, std::move(wrapped_foo));
    }

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
