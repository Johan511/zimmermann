#pragma once

#include <iostream>

// clang-format off
#define LOGD(...) { std::cout << __VA_ARGS__ << std::endl; }
#define LOGI(...) { std::cout << __VA_ARGS__ << std::endl; }
#define LOGW(...) { std::cout << __VA_ARGS__ << std::endl; }
#define LOGE(...) { std::cout << __VA_ARGS__ << std::endl; }
#define LOGF(...) { std::cout << __VA_ARGS__ << std::endl; std::exit(1); }
// clang-format on
