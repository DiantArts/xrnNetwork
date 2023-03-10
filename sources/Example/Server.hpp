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
        if (message.getType() == ::example::MessageType::message) {
            XRN_DEBUG("Preparing to print pull");
            auto id{ message.template pull<::xrn::Id>() };
            auto string{ message.template pull<::std::string>() };
            XRN_DEBUG("{} <- '{}'", id, string);
            message.resetPullPosition();
        }
        switch (message.getType()) {
        default: {
            ::fmt::print(
                "<- C{} '{}'\n"
                , message.pull<::xrn::Id>()
                , message.pull<::std::string>()
            );
            this->tcpSendToAllClients(
                ::std::make_unique<::xrn::network::Message<::example::MessageType>>(message)
                , connection
            );
            break;
        }}
    }

    virtual auto onSend(
        ::xrn::network::Message<::example::MessageType>& message,
        ::std::shared_ptr<::xrn::network::Connection<::example::MessageType>> connection
    ) -> bool override
    {
        if (message.getType() == ::example::MessageType::message) {
            XRN_DEBUG("Preparing to print pull");
            auto id{ message.template pull<::xrn::Id>() };
            auto string{ message.template pull<::std::string>() };
            XRN_DEBUG("{} <- '{}'", id, string);
            message.resetPullPosition();
        }
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
