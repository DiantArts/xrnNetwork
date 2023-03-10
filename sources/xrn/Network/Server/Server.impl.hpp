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
> ::xrn::network::server::Server<UserEnum>::Server(
    ::std::uint16_t port
)
    : ::xrn::network::AClient<UserEnum>{ true }
    , m_connectionAcceptor{
        this->getAsioContext()
        , ::asio::ip::tcp::endpoint{ ::asio::ip::tcp::v4(), port }
    }
{
    this->startServer();
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
> ::xrn::network::server::Server<UserEnum>::~Server()
{
    this->stopServer();
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::server::Server<UserEnum>::Server(
    Server&& that
) noexcept = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::server::Server<UserEnum>::operator=(
    Server&& that
) noexcept
    -> Server& = default;



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Connection managment
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::startReceivingConnections()
{
    m_connectionAcceptor.async_accept(
        [this](
            const ::std::error_code& errCode,
            ::asio::ip::tcp::socket socket
        ) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("Server: Operation canceled");
                    return this->stopServer();
                } else {
                    XRN_ERROR("Server: Failed to accept connection");
                }
            } else {
                XRN_LOG(
                    "Server: New incomming connection: {}:{}"
                    , socket.remote_endpoint().address().to_string()
                    , socket.remote_endpoint().port()
                );
                auto connection{ ::std::make_shared<::xrn::network::Connection<UserEnum>>(
                    ::std::move(socket), *this, ++m_IdGenerator
                ) };
                connection->connectToClient();
                m_connections.push_back(::std::move(connection));
            }

            if (this->isRunning()) {
                this->startReceivingConnections();
            }
        }
    );
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::removeConnection(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> disconnectedConnection
)
{
    ::std::erase_if(
        m_connections,
        [disconnectedConnection](const auto& connection){
            return connection.get() == disconnectedConnection.get();
        }
    );
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Run managment
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::startServer()
{
    this->m_isRunning = true;
    this->startReceivingConnections();
    XRN_LOG("Server started");
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::stopServer()
{
    if (this->isRunning()) {
        this->m_isRunning = false;
        this->getAsioContext().stop();
        this->notifyIncommingMessageQueue();
    }
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
> void ::xrn::network::server::Server<UserEnum>::tcpSendToClient(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... clients
)
{
    for (auto& client : { clients... }) {
        client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToClient(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... clients
)
{
    for (auto& client : m_connections) {
        if (((client == clients) || ...)) {
            client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToAllClients(
    const ::xrn::network::Message<UserEnum>& message
)
{
    for (auto& client : m_connections) {
        client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToAllClients(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client->getId() != ignoredClients->getId()) && ...)) {
            client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToAllClients(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client != ignoredClients) && ...)) {
            client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToClient(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... clients
)
{
    for (auto& client : { clients... }) {
        client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToClient(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... clients
)
{
    for (auto& client : m_connections) {
        if (((client == clients) || ...)) {
            client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToAllClients(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    for (auto& client : m_connections) {
        client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToAllClients(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client->getId() != ignoredClients->getId()) && ...)) {
            client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::tcpSendToAllClients(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client != ignoredClients) && ...)) {
            client->tcpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
        }
    }
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
> void ::xrn::network::server::Server<UserEnum>::udpSendToClient(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... clients
)
{
    for (auto& client : { clients... }) {
        client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToClient(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... clients
)
{
    for (auto& client : m_connections) {
        if (((client == clients) || ...)) {
            client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToAllClients(
    const ::xrn::network::Message<UserEnum>& message
)
{
    for (auto& client : m_connections) {
        client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToAllClients(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client.getId()() != ignoredClients) && ...)) {
            client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToAllClients(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client != ignoredClients) && ...)) {
            client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToClient(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... clients
)
{
    for (auto& client : { clients... }) {
        client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToClient(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... clients
)
{
    for (auto& client : m_connections) {
        if (((client == clients) || ...)) {
            client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToAllClients(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    for (auto& client : m_connections) {
        client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToAllClients(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client.getId()() != ignoredClients) && ...)) {
            client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::udpSendToAllClients(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client != ignoredClients) && ...)) {
            client->udpSend(::std::make_unique<::xrn::network::Message<UserEnum>>(*message));
        }
    }
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
> auto ::xrn::network::server::Server<UserEnum>::handleIncommingSystemMessages(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> connection [[ maybe_unused ]]
    , ::xrn::network::Message<UserEnum>& message [[ maybe_unused ]]
) -> bool
{
    return false;
}

