#pragma once

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/Detail/Constraint.hpp>


namespace xrn::network {

///////////////////////////////////////////////////////////////////////////
/// \brief Allows simple Time manipulations
/// \ingroup network
///
/// \include Time.hpp <xrn/Util/Time.hpp>
///
/// ::xrn::network::AServer's purpuse is to provide abstracted
/// functionalities that can be implemented.
///
/// Usage example:
/// \code
/// \endcode
///
///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::hasValueLast MessageType
> class AServer {

public:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Run managment
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Start the server
    ///
    /// \return True if the server started successfully, false otherwise
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] virtual auto start()
        -> bool = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Stop the server
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual void stop() = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Check whether the server is running or not
    ///
    /// \return True if the server is started and running
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] virtual auto isRunning()
        -> bool = 0;



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
    virtual void send(
        const ::xrn::network::Message<MessageType>& message,
        ::xrn::meta::constraint::sameAs<::std::shared_ptr<
            ::xrn::network::Connection<UserMessageType>
        >> auto... clients
    ) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message over UDP to the clients given as argument
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual void send(
        const ::xrn::network::Message<MessageType>& message,
        ::xrn::meta::constraint::sameAs<::std::shared_ptr<
            ::xrn::network::Id
        >> auto... clients
    ) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message over UDP to all clients execpt the one in
    /// argument
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual void sendToAll(
        const ::xrn::network::Message<MessageType>& message,
        ::xrn::meta::constraint::sameAs<::std::shared_ptr<
            ::xrn::network::Connection<UserMessageType>
        >> auto... ignoredClients
    ) = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message over UDP to all clients execpt the one in
    /// argument
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual void sendToAll(
        const ::xrn::network::Message<MessageType>& message,
        ::xrn::meta::constraint::sameAs<::std::shared_ptr<
            ::xrn::network::Id
        >> auto... ignoredClients
    ) = 0;


};

} // namespace xrn::network

///////////////////////////////////////////////////////////////////////////
// Implementation Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/AServer.impl.hpp>
