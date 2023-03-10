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

    void messageServer(
        const ::std::string& str
    )
    {
        this->tcpSendToServer(::example::MessageType::message, m_connection->getId(), str);
    }

    virtual void onReceive(
        ::xrn::network::Message<::example::MessageType>& message,
        ::std::shared_ptr<::xrn::network::Connection<::example::MessageType>> connection [[ maybe_unused ]]
    ) override
    {
        switch (message.getType()) {
        default: {
            ::fmt::print(
                "<- C{} '{}'\n"
                , message.pull<::xrn::Id>()
                , message.pull<::std::string>()
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
                "-> C{} '{}'\n"
                , connection->getId()
                , message.pull<::std::string>()
            );
            break;
        }}
        return true;
    }
};

}
