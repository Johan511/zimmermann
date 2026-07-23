#include "core/logging.hpp"
#include "math/vec3.hpp"
#include "network/socket.hpp"

int main()
{
    core::log(core::Level::Info, "Complex example starting");

    // Use libmath
    math::Vec3 v{3.0, 4.0, 0.0};
    math::print(v);
    core::log(core::Level::Info,
              std::format("Vector length: {:.2f}", v.length()));

    // Use libnetwork (which also uses libcore)
    network::Socket sock({"localhost", 8080});
    sock.connect();
    sock.send("Hello from zimmermann!");
    sock.disconnect();

    core::log(core::Level::Info, "Complex example finished");
}
