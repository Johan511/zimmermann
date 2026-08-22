module;
export module extra;

import math;

// A module that *imports* another module — exercises the generator's
// topological ordering of module interface units (extra must be built after math).
export int combined(int a, int b)
{
    return add(a, b) + mul(a, b);
}
