#pragma once
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace zimm
{
class Executable;

namespace detail
{
struct Test
{
    Executable *exec;
    std::string conflatedArgs;
};
} // namespace detail

class Tester
{
    std::vector<detail::Test> m_tests;
    friend class Project;

public:
    template <typename... Args>
    void add_test(Executable *exec, const Args &...args)
    {
        std::ostringstream oss;
        ((oss << args << ' '), ...);
        std::string conflatedArgs = std::move(oss).str();
        if constexpr (sizeof...(args)) conflatedArgs.pop_back(); // remove the last space
        m_tests.emplace_back(exec, std::move(conflatedArgs));
    }

    std::span<const detail::Test> tests() { return m_tests; }
};
} // namespace zimm
