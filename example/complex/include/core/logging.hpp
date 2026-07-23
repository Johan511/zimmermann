#pragma once

#include <string_view>

namespace core
{

enum class Level
{
    Debug,
    Info,
    Warning,
    Error,
};

void log(Level level, std::string_view message);

} // namespace core
