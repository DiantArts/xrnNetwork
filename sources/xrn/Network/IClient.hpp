#pragma once

#include <xrn/Network/Message.hpp>

namespace xrn::network {

///////////////////////////////////////////////////////////////////////////
/// \brief Interface detailing basic clients' features
/// \ingroup network
///
/// \include IClient.hpp <xrn/Network/Client/IClient.hpp>
///
/// ::xrn::network::IClient's purpuse is to provide an interface that every
/// clients will have to implement.
/// The client is designed as Event programming
///
/// Usage: Implemented by ::xrn::network::AClient
///
/// \see ::xrn::network::AClient, ::xrn::network::Message
///
///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::hasValueLast UserEnum
> class IClient {

public:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Rule of 5
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Clears the registry opon destruction.
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual ~IClient() = 0;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    IClient(
        const IClient& other
    ) noexcept = delete;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy assign operator
    ///
    ///////////////////////////////////////////////////////////////////////////
    auto operator=(
        const IClient& other
    ) noexcept
        -> IClient& = delete;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    IClient(
        IClient&& that
    ) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Move assign operator
    ///
    ///////////////////////////////////////////////////////////////////////////
    auto operator=(
        IClient&& that
    ) noexcept
        -> IClient&;



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Actions
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message to a targeted client
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual void send(
        const ::xrn::network::IClient<UserEnum>& target
        , const ::xrn::network::Message<UserEnum>& message
    ) = 0;



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Events
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Event triggered when a connection with a client is established
    ///
    /// Event called just before after a connection with another client is
    /// successfully established, secured and ready to be used
    ///
    /// \param target Client with who the connection has been established
    ///
    /// \return False to prevent the message to be sent, True otherwise
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] virtual auto onConnect(
        ::xrn::network::IClient<UserEnum>& target
    ) -> bool;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Event triggered when disconnected from a client
    ///
    /// Event called just before after the client is disconnected. This
    /// event is called, whether the connection has been cut or it has been
    /// lost for any other reason
    ///
    /// \param target Client with who the connection has been stopped
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual void onDisconnect(
        ::xrn::network::IClient<UserEnum>& target
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Event triggered when a message is about to be sent
    ///
    /// Event called just before a message is sent to a client. This event can
    /// prevent the message to be sent or modify its content
    ///
    /// \param target  Client targeted by the message
    /// \param message Mutable message sent to the target
    ///
    /// \return False to prevent the message to be sent, True otherwise
    ///
    /// \see ::xrn::network::Message
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] virtual auto onSend(
        ::xrn::network::IClient<UserEnum>& target
        , ::xrn::network::Message<UserEnum>& message
    ) -> bool;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Event triggered when a message is received
    ///
    /// Event called just after receiving a message from a client.
    ///
    /// \param target  Client that sent the message
    /// \param message Mutable message sent by the target
    ///
    /// \see ::xrn::network::Message
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual void onReceive(
        ::xrn::network::IClient<UserEnum>& target
        , ::xrn::network::Message<UserEnum>& message
    );



private:

};

} // namespace xrn::network

///////////////////////////////////////////////////////////////////////////
// Implementation Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/IClient.impl.hpp>
