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
        auto message{ ::std::make_unique<::xrn::network::Message<::example::MessageType>>(::example::MessageType::message) };
        *message << m_connection->getId() << str;
        this->tcpSendToServer(::std::move(message));
    }

    virtual void onReceive(
        ::xrn::network::Message<::example::MessageType>& message,
        ::std::shared_ptr<::xrn::network::Connection<::example::MessageType>> connection [[ maybe_unused ]]
    ) override
    {
        switch (message.getType()) {
        default: {
            ::xrn::Id id;
            ::std::string string;
            message >> id >> string;
            ::fmt::print("<- C{} '{}'\n", id, string);
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
            ::std::string string;
            message >> string;
            ::fmt::print("-> C{} '{}'\n", connection->getId(), string);
            break;
        }}
        return true;
    }
};

}
