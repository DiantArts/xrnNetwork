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
    : m_connectionAcceptor{
        this->getAsioContext()
        , ::asio::ip::tcp::endpoint{ ::asio::ip::tcp::v4(), port }
    }
{
    this->start();
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
    this->stop();
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
// Run managment
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::start()
{
    this->m_isRunning = true;
    this->startReceivingConnections();
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::stop()
{
    if (this->isRunning()) {
        this->m_isRunning = false;
        this->getAsioContext().stop();
        this->notifyIncommingMessageQueue();
    }
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
> void ::xrn::network::server::Server<UserEnum>::send(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... clients
)
{
    for (auto& client : { clients... }) {
        client->send(message);
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::send(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... clients
)
{
    for (auto& client : m_connections) {
        if (((client == clients) || ...)) {
            client->send(message);
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::sendToAll(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::std::shared_ptr<
        ::xrn::network::Connection<UserEnum>
    >> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client.getId()() != ignoredClients) && ...)) {
            client->send(message);
        }
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::server::Server<UserEnum>::sendToAll(
    const ::xrn::network::Message<UserEnum>& message
    , ::xrn::meta::constraint::sameAs<::xrn::Id> auto... ignoredClients
)
{
    for (auto& client : m_connections) {
        if (((client != ignoredClients) && ...)) {
            client->send(message);
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
> void ::xrn::network::server::Server<UserEnum>::startReceivingConnections()
{
    XRN_DEBUG();
    m_connectionAcceptor.async_accept(
        [this](
            const ::std::error_code& errCode,
            ::asio::ip::tcp::socket socket
        ) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("Server: Operation canceled");
                    return this->stop();
                } else {
                    XRN_ERROR("Server: Failed to accept connection");
                }
            } else {
                XRN_LOG(
                    "Server: New incomming connection: {}:{}"
                    , socket.remote_endpoint().address().to_string()
                    , socket.remote_endpoint().port()
                );
                m_connections.push_back(
                    ::std::make_shared<::xrn::network::Connection<UserEnum>>(
                        ::std::move(socket), *this
                    )
                );
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
> auto ::xrn::network::server::Server<UserEnum>::handleIncommingSystemMessages(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> connection
    , ::xrn::network::Message<UserEnum>& message
) -> bool
{
    return false;
}