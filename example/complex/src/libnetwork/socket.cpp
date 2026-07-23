#include "network/socket.hpp"
#include "core/logging.hpp"

namespace network
{

bool Socket::connect()
{
    core::log(core::Level::Info, std::format("Connecting to {}:{}", m_addr.host, m_addr.port));
    m_connected = true;
    return true;
}

void Socket::disconnect()
{
    core::log(core::Level::Info, "Disconnecting");
    m_connected = false;
}

void Socket::send(std::string_view data) const
{
    if (m_connected)
        std::println("Sent {} bytes to {}:{}", data.size(), m_addr.host, m_addr.port);
}

} // namespace network
