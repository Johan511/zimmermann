#include "network/socket.hpp"
#include "core/logging.hpp"

#include <format>
#include <iostream>

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
        std::cout << "Sent " << data.size() << " bytes to " << m_addr.host << ':' << m_addr.port
                  << '\n';
}

} // namespace network
