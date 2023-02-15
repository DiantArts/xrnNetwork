#pragma once

#include <xrn/Network/Client/Client.hpp>
#include <Example/MessageType.hpp>

namespace example {

class Client
    : public ::xrn::network::client::Client<::example::MessageType>
{

public:

    Client(
        const ::std::string& host
        , ::std::uint16_t port
    )
        : ::xrn::network::client::Client<::example::MessageType>{ host, port }
    {}

    void commandHelp()
    {
        ::std::cout << "h: help\n";
        ::std::cout << "q: quit\n";
        ::std::cout << "u: message using UDP instead of TCP\n";
        ::std::cout << "n: rename\n";
        ::std::cout << "c: display connected clients" << ::std::endl;
    }

    void messageServer(
        const ::std::string& message
    )
    {
        this->udpSend(::example::MessageType::messageAll, message);
    }

    virtual void onReceive(
        ::xrn::network::Message<::example::MessageType>& message,
        ::std::shared_ptr<::xrn::network::Connection<::example::MessageType>> connection
    ) override
    {
        switch (message.getType()) {
        default: {
            ::fmt::print("<- C{}: '{}'\n", connection->getId(), message.pull<::std::string>());
        break;
        }}
    }
};

}
