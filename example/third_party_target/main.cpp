#include "my_lib.hpp"
#include <gtest/gtest.h>
#include <httplib.h>
#include <iostream>

int main()
{
    my_lib_func();                           // from the found package
    EXPECT_EQ(1, 1);                         // from googletest (FetchContent)
    httplib::Client cli("http://localhost"); // from cpp-httplib (fallback → FetchContent)
    std::cout << "tpt_demo ok" << std::endl;
    return 0;
}
