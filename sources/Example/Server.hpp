#pragma once

#include <xrn/Network/Server/Server.hpp>
#include <Example/MessageType.hpp>

namespace example {

class Server
    : public ::xrn::network::server::Server<::example::MessageType>
{

public:

    Server(
        ::std::uint16_t port
    )
        : ::xrn::network::server::Server<::example::MessageType>{ port }
    {}

    virtual void onReceive(
        ::xrn::network::Message<::example::MessageType>& message,
        ::std::shared_ptr<::xrn::network::Connection<::example::MessageType>> connection
    ) override
    {
        switch (message.getType()) {
        default: {
            auto str{ message.pull<::std::string>() };
            ::fmt::print("<- C{}: '{}'\n", connection->getId(), str);
            this->tcpSendToAll(message);
            break;
        }}
    }
};

}
