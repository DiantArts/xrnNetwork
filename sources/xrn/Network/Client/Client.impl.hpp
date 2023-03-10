#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Constructors
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::client::Client<UserEnum>::Client() = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::client::Client<UserEnum>::Client(
    const ::std::string& host
    , ::std::uint16_t port
)
{
    if (!this->connectToServer(host, port)) {
        XRN_FATAL("Could not connect to the server in constructor");
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Rule of 5
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::client::Client<UserEnum>::~Client()
{
    XRN_DEBUG("Destroying client");
    this->disconnectFromServer();
    XRN_DEBUG("...Client destroyed");
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::client::Client<UserEnum>::Client(
    Client&& that
) noexcept = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::client::Client<UserEnum>::operator=(
    Client&& that
) noexcept
    -> Client& = default;



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Connection
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::client::Client<UserEnum>::connectToServer(
    const ::std::string& host
    , ::std::uint16_t port
) -> bool
{
    this->m_isRunning = true;
    m_connection = ::std::make_shared<::xrn::network::Connection<UserEnum>>(*this);
    m_connection->connectToServer(host, port);

    // wait for a notification to make sure the connection is well initialized
    this->waitForIncommingMessages();
    return this->isConnectedToServer();
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::disconnectFromServer()
{
    if (this->m_isRunning) {
        this->m_isRunning = false;
        this->notifyIncommingMessageQueue();
        m_connection.reset();
        XRN_DEBUG("Client disconnected");
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::client::Client<UserEnum>::isConnectedToServer() const
    -> bool
{
    return m_connection && m_connection->isConnected();

}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::client::Client<UserEnum>::getConnectionId() const
    -> ::xrn::Id
{
    return m_connection->getId();
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::removeConnection(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> _ [[ maybe_unused ]]
)
{
    this->disconnectFromServer();
}




///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Tcp
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::tcpSendToServer(
    typename ::xrn::network::Message<UserEnum>::SystemType messageType,
    auto&&... args
)
{
    m_connection->tcpSend(
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            messageType
            , ::std::forward<decltype(args)>(args)...
        )
    );
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::tcpSendToServer(
    UserEnum messageType,
    auto&&... args
)
{
    m_connection->tcpSend(
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            messageType
            , ::std::forward<decltype(args)>(args)...
        )
    );
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::tcpSendToServer(
    const ::xrn::network::Message<UserEnum>& message
)
{
    m_connection->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(::std::move(message)));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::tcpSendToServer(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    m_connection->tcpSend(::std::move(message));
}




///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Udp
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::udpSendToServer(
    typename ::xrn::network::Message<UserEnum>::SystemType messageType,
    auto&&... args
)
{
    m_connection->udpSend(
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            messageType
            , ::std::forward<decltype(args)>(args)...
        )
    );
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::udpSendToServer(
    UserEnum messageType,
    auto&&... args
)
{
    m_connection->udpSend(
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            messageType
            , ::std::forward<decltype(args)>(args)...
        )
    );
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::udpSendToServer(
    const ::xrn::network::Message<UserEnum>& message
)
{
    m_connection->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(::std::move(message)));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::client::Client<UserEnum>::udpSendToServer(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    m_connection->udpSend(::std::move(message));
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Incomming messages
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::client::Client<UserEnum>::handleIncommingSystemMessages(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> connection [[ maybe_unused ]]
    , ::xrn::network::Message<UserEnum>& message [[ maybe_unused ]]
) -> bool
{
    return false;
}
