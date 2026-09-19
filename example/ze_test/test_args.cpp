#include <format>
#include <iostream>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
        std::cout << std::format("arg{}: {}\n", i, argv[i]);
    return 0;
}
