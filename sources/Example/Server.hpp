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
            ::fmt::print(
                "<- C{} '{}'\n"
                , message.pull<::xrn::Id>()
                , message.pull<::std::string>()
            );
            this->tcpSendToAllClients(message, connection);
            break;
        }}
    }

    virtual auto onSend(
        ::xrn::network::Message<::example::MessageType>& message,
        ::std::shared_ptr<::xrn::network::Connection<::example::MessageType>> connection
    ) -> bool override
    {
        switch (message.getType()) {
        default: {
            ::fmt::print(
                "C{} -> C{} '{}'\n"
                , message.pull<::xrn::Id>()
                , connection->getId()
                , message.pull<::std::string>()
            );
            break;
        }}
        return true;
    }
};

}
