#include <cstdio>

int main()
{
#ifdef HAVE_CSTDIO
    std::printf("HAVE_CSTDIO=1\n");
#else
    std::printf("HAVE_CSTDIO=0\n");
#endif

#ifdef HAVE_NONEXISTENT_HEADER
    std::printf("HAVE_NONEXISTENT_HEADER=1\n");
#else
    std::printf("HAVE_NONEXISTENT_HEADER=0\n");
#endif

#ifdef HAVE_PRINTF
    std::printf("HAVE_PRINTF=1\n");
#else
    std::printf("HAVE_PRINTF=0\n");
#endif

#ifdef HAVE_NONEXISTENT_FUNC
    std::printf("HAVE_NONEXISTENT_FUNC=1\n");
#else
    std::printf("HAVE_NONEXISTENT_FUNC=0\n");
#endif

#ifdef HAVE_O_RDONLY
    std::printf("HAVE_O_RDONLY=1\n");
#else
    std::printf("HAVE_O_RDONLY=0\n");
#endif

#ifdef INT_SIZE
    std::printf("INT_SIZE=%d\n", INT_SIZE);
#endif

#ifdef LLONG_SIZE
    std::printf("LLONG_SIZE=%d\n", LLONG_SIZE);
#endif

#ifdef PTR_SIZE
    std::printf("PTR_SIZE=%d\n", PTR_SIZE);
#endif

    return 0;
}
