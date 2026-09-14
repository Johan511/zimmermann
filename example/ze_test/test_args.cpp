#include <print>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
        std::println("arg{}: {}", i, argv[i]);
    return 0;
}
