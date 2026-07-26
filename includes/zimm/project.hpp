#pragma once

#include "config.hpp"
#include "installer.hpp"
#include "target.hpp"
#include "tester.hpp"
#include <cassert>
#include <filesystem>
#include <optional>
#include <queue>
#include <set>
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

public:
    Project(std::string name, Config config)
        : m_name(std::move(name)), m_config(std::move(config)), m_buildDir(m_config.build_dir),
          m_featureDetectionDir(m_config.install_dir), m_installDir(m_buildDir.make("install")),
          m_installer(m_config.build_dir)
    {
        namespace fs = std::filesystem;
        fs::create_directories(fs::path{m_buildDir.path()});
        fs::create_directories(fs::path{m_featureDetectionDir.path()});
    }

    std::string_view name() const noexcept { return m_name; }
    const Config &config() const noexcept { return m_config; }
    std::span<const PropertyObject> global_properties() const noexcept
    {
        return m_globalProperties;
    }
    Installer *installer() noexcept { return &m_installer; }
    const Installer *installer() const noexcept { return &m_installer; }

    Tester *tester() noexcept { return &m_tester; }
    const Tester *tester() const noexcept { return &m_tester; }

    void register_top_level_target(Target *target) { m_topLevelTargets.emplace_back(target); }
    void add_global_property(PropertyObject property) { m_globalProperties.push_back(property); }

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
    Directory m_buildDir;
    Directory m_featureDetectionDir;
    Directory m_installDir;

    Installer m_installer;
    Tester m_tester;
};

void generate_build(Project &project);

} // namespace zimm
