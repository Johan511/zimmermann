#pragma once

#include <iostream>

// clang-format off
#define LOGD(...) do { std::cout << __VA_ARGS__ << std::endl; } while(0)
#define LOGI(...) do { std::cout << __VA_ARGS__ << std::endl; } while(0)
#define LOGW(...) do { std::cout << __VA_ARGS__ << std::endl; } while(0)
#define LOGE(...) do { std::cout << __VA_ARGS__ << std::endl; } while(0)
#define LOGF(...) do { std::cout << __VA_ARGS__ << std::endl; std::exit(1); } while(0)
// clang-format on
