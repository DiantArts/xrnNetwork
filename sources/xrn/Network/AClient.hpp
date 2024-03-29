#pragma once

#include <xrn/Network/IClient.hpp>

namespace xrn::network {

///////////////////////////////////////////////////////////////////////////
/// \brief Interface detailing basic clients' features
/// \ingroup network
///
/// \include AClient.hpp <xrn/Network/Client/AClient.hpp>
///
/// ::xrn::network::AClient's purpuse is simplify data share by providing a
/// simple way to regroup and act onto multiple data into a single object.
/// The client is designed as Event programming and the user must implement
/// the on... methods to interact with it
///
/// \see ::xrn::network::IClient, ::xrn::network::Message
///
///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> class AClient
    : public ::xrn::network::IClient<UserEnum>
{

public:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    explicit AClient(
        bool isServer = false
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
    /// Join the threads
    ///
    ///////////////////////////////////////////////////////////////////////////
    virtual ~AClient();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    AClient(
        const AClient& other
    ) noexcept = delete;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy assign operator
    ///
    ///////////////////////////////////////////////////////////////////////////
    auto operator=(
        const AClient& other
    ) noexcept
        -> AClient& = delete;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    AClient(
        AClient&& that
    ) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Move assign operator
    ///
    ///////////////////////////////////////////////////////////////////////////
    auto operator=(
        AClient&& that
    ) noexcept
        -> AClient&;



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Incomming messages
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Wait for messages to arrive, but do not pull them
    ///
    ///////////////////////////////////////////////////////////////////////////
    void waitForIncommingMessages();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Notify the inccomming message queue that an event occured
    ///
    /// The event is most likely that a new message arrived (can be that
    /// the client disconnects and more).
    ///
    ///////////////////////////////////////////////////////////////////////////
    void notifyIncommingMessageQueue();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief push an incomming message into the queue
    ///
    /// \arg message Message to push
    /// \arg connection Connection from which the message came from
    ///
    /// \see ::xrn::network::Message, ::xrn::network::Connection
    ///
    ///////////////////////////////////////////////////////////////////////////
    void pushIncommingMessage(
        ::std::shared_ptr<::xrn::network::Connection<UserEnum>> connection
        , ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    );



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Getters
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Get the asio context running on the thread (getThreadContext())
    ///
    /// \see getThreadContext()
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto isServer() const
        -> bool;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief True if the Client/Server is still running correctly
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto isRunning() const
        -> bool;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Get the asio context running on the thread (getThreadContext())
    ///
    /// \see getThreadContext()
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto getAsioContext() const
        -> const ::asio::io_context&;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Get the asio context running on the thread (getThreadContext())
    ///
    /// \see getThreadContext()
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto getAsioContext()
        -> ::asio::io_context&;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Get the thread containing the asio context
    ///
    /// \see getAsioContext()
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto getThreadContext()
        -> ::std::thread&;



protected:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Incomming messages helpers
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

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
    virtual auto handleIncommingSystemMessages(
        ::std::shared_ptr<::xrn::network::Connection<UserEnum>> connection
        , ::xrn::network::Message<UserEnum>& message
    ) -> bool = 0;



protected:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Members
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Check if the client is still running
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::atomic_bool m_isRunning;



private:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Incomming messages helpers
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Pull a single incomming message
    ///
    /// Calls handleIncommingSystemMessage() method to clear out system
    /// messages, then call onReceive() is the message was not handled
    ///
    /// \handleIncommingSystemMessage(), onReceive()
    ///
    ///////////////////////////////////////////////////////////////////////////
    void pullIncommingMessage();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Pull all incomming messages
    ///
    /// \see pullIncommingMessage
    ///
    ///////////////////////////////////////////////////////////////////////////
    void pullIncommingMessages();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Wait for messages to arrive, then call pullIncommingMessages()
    ///
    /// \see pullIncommingMessage
    ///
    ///////////////////////////////////////////////////////////////////////////
    void blockingPullIncommingMessages();



private:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Members
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Is the client a server
    ///
    ///////////////////////////////////////////////////////////////////////////
    bool m_isServer;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Queue of all the messages received
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::xrn::network::detail::Queue<::std::unique_ptr<
        ::xrn::network::OwnedMessage<UserEnum>
    >> m_messagesIn;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Asio context running on m_asioContextThread
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::asio::io_context m_asioContext;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Idle work so the asio context never ends
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::asio::io_context::work m_idleWork;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Thread containing the asio context
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::thread m_asioContextThread;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Thread containing the automatic pull of messages
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::thread m_inMessagesThread;

};

} // namespace xrn::network

///////////////////////////////////////////////////////////////////////////
// Implementation Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/AClient.impl.hpp>
