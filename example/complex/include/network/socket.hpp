#pragma once

#include <print>
#include <string>

namespace network
{

struct Address
{
    std::string host;
    int port;
};

class Socket
{
public:
    explicit Socket(Address addr) : m_addr(std::move(addr)) {}

    bool connect();
    void disconnect();
    void send(std::string_view data) const;

private:
    Address m_addr;
    bool m_connected = false;
};

} // namespace network
