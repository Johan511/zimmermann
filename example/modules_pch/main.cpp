import math;
import extra;
#include "common.hpp"

int main()
{
    const int result = combined(2, 40); // add(2,40)+mul(2,40) = 42 + 80 = 122
    std::puts(mpch::banner("module + pch example").c_str());
    std::printf("result=%d\n", result);
    return 0;
}
