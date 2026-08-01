#include "core/logging.hpp"
#include <print>

namespace core
{

void log(Level level, std::string_view message)
{
    constexpr auto level_str = [](Level l) -> std::string_view
    {
        switch (l)
        {
        case Level::Debug:
            return "[DEBUG]";
        case Level::Info:
            return "[INFO]";
        case Level::Warning:
            return "[WARN]";
        case Level::Error:
            return "[ERROR]";
        }
        return "[???]";
    };

    std::println("{} {}", level_str(level), message);
}

} // namespace core
