#include "math/vec3.hpp"
#include "core/logging.hpp"

namespace math
{

// Vec3 is mostly inline in the header; this TU exists to force a dependency
// on libcore, demonstrating transitive include propagation in the build.
namespace
{
struct MathInit
{
    MathInit() { core::log(core::Level::Debug, "libmath: static init complete"); }
};
MathInit _math_init;
} // namespace

} // namespace math
