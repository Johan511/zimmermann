import sys

numStr = sys.argv[1]
num = int(numStr)

with open("adder.h", "w") as f:
    sourceStr = """
        int add_{0}(int num);
    """
    f.write(sourceStr.format(numStr))

with open("adder.cpp", "w") as f:
    sourceStr = """
        #include <adder.h>
        int add_{0}(int num)
        {{
            return num + {0};
        }}
    """
    f.write(sourceStr.format(numStr))
