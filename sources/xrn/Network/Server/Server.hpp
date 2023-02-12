#pragma once

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/AClient.hpp>

namespace xrn::network::server {

///////////////////////////////////////////////////////////////////////////
/// \brief Allows simple Time manipulations
/// \ingroup network
///
/// \include Time.hpp <xrn/Util/Time.hpp>
///
/// ::xrn::network::server::Server's purpuse is to provide abstracted
/// functionalities that can be implemented.
///
/// Usage example:
/// \code
/// \endcode
///
///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> class Server
    : public ::xrn::network::AClient<UserEnum>
{

public:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Start the server
    ///
    /// \return True if the server started successfully, false otherwise
    ///
    ///////////////////////////////////////////////////////////////////////////
    explicit Server(
        ::std::uint16_t port
    );



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Rule of 5
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    ///
    /// Clears the registry opon destruction.
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual ~Server();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    Server(
        const Server& other
    ) noexcept = delete;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy assign operator
    ///
    ///////////////////////////////////////////////////////////////////////////
    auto operator=(
        const Server& other
    ) noexcept
        -> Server& = delete;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    Server(
        Server&& that
    ) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Move assign operator
    ///
    ///////////////////////////////////////////////////////////////////////////
    auto operator=(
        Server&& that
    ) noexcept
        -> Server&;



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Run managment
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Start the server
    ///
    ///////////////////////////////////////////////////////////////////////////
    void start();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Stop the server
    ///
    ///////////////////////////////////////////////////////////////////////////
    void stop();



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Run managment
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message over UDP to the clients given as argument
    ///
    ///////////////////////////////////////////////////////////////////////////
    void send(
        const ::xrn::network::Message<UserEnum>& message,
        ::xrn::meta::constraint::sameAs<::std::shared_ptr<
            ::xrn::network::Connection<UserEnum>
        >> auto... clients
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message over UDP to the clients given as argument
    ///
    ///////////////////////////////////////////////////////////////////////////
    void send(
        const ::xrn::network::Message<UserEnum>& message,
        ::xrn::meta::constraint::sameAs<::xrn::Id> auto... clients
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message over UDP to all clients execpt the one in
    /// argument
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendToAll(
        const ::xrn::network::Message<UserEnum>& message,
        ::xrn::meta::constraint::sameAs<::std::shared_ptr<
            ::xrn::network::Connection<UserEnum>
        >> auto... ignoredClients
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message over UDP to all clients execpt the one in
    /// argument
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendToAll(
        const ::xrn::network::Message<UserEnum>& message,
        ::xrn::meta::constraint::sameAs<::xrn::Id> auto... ignoredClients
    );



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Incomming messages
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Start accepting the connections
    ///
    ///////////////////////////////////////////////////////////////////////////
    void startReceivingConnections();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Handle if the message is a system message
    ///
    /// \param message Message to handle
    /// \param connection Connection that sent the message
    ///
    /// \return True if the message was handled, false otherwise
    ///
    /// \see ::xrn::network::Message, ::xrn::network::Connection
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] virtual auto handleIncommingSystemMessages(
        ::std::shared_ptr<::xrn::network::Connection<UserEnum>> connection
        , ::xrn::network::Message<UserEnum>& message
    ) -> bool override;



private:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Members
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Hardware connection to the server
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::asio::ip::tcp::acceptor m_connectionAcceptor;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Endpoint of the accepted connection
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::asio::ip::udp::endpoint m_remoteEndpoint;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief List of connections to the server
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::deque<::std::shared_ptr<::xrn::network::Connection<UserEnum>>> m_connections;


};

} // namespace xrn::network::server

///////////////////////////////////////////////////////////////////////////
// Implementation Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/Server/Server.impl.hpp>