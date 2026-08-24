#pragma once

#include <cstdlib>
#include <iostream>
#include <source_location>

// clang-format off
#define DETAIL_LOG_PRE(level) do{ auto loc = std::source_location::current(); std::cout << '[' << level << "] " << loc.file_name() << ":" << loc.line() << ' '; } while(0)
#define LOGD(...) do { DETAIL_LOG_PRE('D'); std::cout << __VA_ARGS__ << std::endl; } while(0)
#define LOGI(...) do { DETAIL_LOG_PRE('I'); std::cout << __VA_ARGS__ << std::endl; } while(0)
#define LOGW(...) do { DETAIL_LOG_PRE('W'); std::cout << __VA_ARGS__ << std::endl; } while(0)
#define LOGE(...) do { DETAIL_LOG_PRE('E'); std::cout << __VA_ARGS__ << std::endl; } while(0)
#define LOGF(...) do { DETAIL_LOG_PRE('F'); std::cout << __VA_ARGS__ << std::endl; std::exit(1); } while(0)
// clang-format on
