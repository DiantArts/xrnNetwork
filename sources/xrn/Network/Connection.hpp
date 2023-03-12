#pragma once

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/Detail/Constraint.hpp>
#include <xrn/Network/Detail/Queue.hpp>
#include <xrn/Network/Message.hpp>
#include <xrn/Network/OwnedMessage.hpp>

///////////////////////////////////////////////////////////////////////////
// Forward declarations
///////////////////////////////////////////////////////////////////////////
namespace xrn::network {
    template <::xrn::network::detail::constraint::isValidEnum> class AClient;
}

namespace xrn::network {

///////////////////////////////////////////////////////////////////////////
/// \brief A UDP Connection using boost asio
///
/// \include Connection.hpp <xrn/Network/Connection.hpp>
///
/// Its purpuse is to simplifie communications between two UDP endpoints
/// It uses ::xrn::network::Message to communicate
///
/// \see ::network::Message
///
///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> class Connection
    : public ::std::enable_shared_from_this<::xrn::network::Connection<UserEnum>>
{

public:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // constructors
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Construct a new udp connection for a client
    ///
    /// Starts an asynchronous connection to a server on the asio thread
    /// disconnect() will be automatically called if needed
    ///
    /// /warning Not calling connectToServer() after creating the connection
    /// leads to undefined behavior
    ///
    ///////////////////////////////////////////////////////////////////////////
    explicit Connection(
        ::xrn::network::AClient<UserEnum>& owner
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Construct a new udp connection for a server
    ///
    /// Starts an asynchronous connection to a server on the asio thread
    /// disconnect() will be automatically called if needed
    ///
    /// /warning Not calling connectToClient() after creating the connection
    /// leads to undefined behavior
    ///
    /// \param socket The udp socket from asio, generated beforehand
    ///
    ///////////////////////////////////////////////////////////////////////////
    Connection(
        ::asio::ip::tcp::socket&& socket
        , ::xrn::network::AClient<UserEnum>& owner
        , ::xrn::Id clientId
    );



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Rule of 5
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Calls disconnect before deleting all the resources attached
    /// to the Connection
    ///
    ///////////////////////////////////////////////////////////////////////////
    ~Connection();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    inline Connection(
        const Connection& other
    ) noexcept = delete;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy assign operator
    ///
    ///////////////////////////////////////////////////////////////////////////
    inline auto operator=(
        const Connection& other
    ) noexcept
        -> Connection& = delete;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ///////////////////////////////////////////////////////////////////////////
    inline Connection(
        Connection&& that
    ) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Move assign operator
    ///
    ///////////////////////////////////////////////////////////////////////////
    inline auto operator=(
        Connection&& that
    ) noexcept
        -> Connection&;



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Connection
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Connect to a client
    ///
    /// Server only function
    ///
    ///////////////////////////////////////////////////////////////////////////
    void connectToClient();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Connect to a server
    ///
    /// Client only function
    ///
    /// \parm host IP address of the host (remote)
    /// \parm port Port of the IP address of the host (remote)
    ///
    ///////////////////////////////////////////////////////////////////////////
    void connectToServer(
        const ::std::string& host
        , ::std::uint16_t port
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Close the connection from the target
    ///
    /// Cancels every asynchronous actions and close the socket
    ///
    /// \arg isAlreadyDestroyed Calls m_owner.removeConnection if not destroyed
    ///
    ///////////////////////////////////////////////////////////////////////////
    void disconnect(
        bool isAlreadyDestroyed = false
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Check if the connection is valid on both UDP and TCP
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto isConnected()
        -> bool;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Start receiving messages
    ///
    /// Both tcp and udp
    ///
    ///////////////////////////////////////////////////////////////////////////
    void startReceivingMessages();



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Tcp
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Retrieve the id of the connection from the server
    ///
    /// Client only
    ///
    ///////////////////////////////////////////////////////////////////////////
    void synchronizeId();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a tcp message
    ///
    /// Asynchronously pushes the to the message into the queue of outgoing
    /// messages
    /// If the queue was empty before the push, it then calls
    /// sendAwaitingMessages()
    ///
    /// \param message pointer to the message to send
    ///
    ///////////////////////////////////////////////////////////////////////////
    void tcpSend(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a tcp message and block the execution till it is sent
    ///
    /// \param message pointer to the message to send
    ///
    ///////////////////////////////////////////////////////////////////////////
    void blockingTcpSend(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Test whether there is Awaiting tcp messages to be sent
    ///
    /// Test if the outgoing queue of messages is empty. If it is,
    /// it means no messages are awaiting to be sent
    ///
    /// \return True if messages are awaiting to be sent
    ///
    ///////////////////////////////////////////////////////////////////////////
    auto hasSendingTcpMessagesAwaiting() const
        -> bool;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send awaiting tcp messages
    ///
    /// Asynchronously sends all the messages inside the outgoing queue of
    /// messages by calling sendMessage()
    /// The function recursivly clears the outgoing queue of messages
    ///
    /// \warning This method assumes at least one message waits to be sent
    /// Calling this methods without any message waiting leads to
    /// undefined behavior and/or potential segfaults
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendAwaitingTcpMessages();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Start receiving messages
    ///
    /// Waits for incomming messages. On receive, the buffer of incomming
    /// messages is filled with the incomming message, it calls
    /// transferInMessageToOwner()
    ///
    /// \warning the buffer needs to be empty or it may lead to undefined
    /// behaviors
    ///
    ///////////////////////////////////////////////////////////////////////////
    void startReceivingTcpMessages();



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Udp
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Retrieve udp information from the server
    ///
    ///////////////////////////////////////////////////////////////////////////
    void retrieveUdpInformation();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief set a target for the UDP messages
    ///
    ///////////////////////////////////////////////////////////////////////////
    void setUdpTarget(
        const ::std::string& host
        , const ::std::uint16_t port
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a udp message
    ///
    /// Asynchronously pushes the to the message into the queue of outgoing
    /// messages
    /// If the queue was empty before the push, it then calls
    /// sendAwaitingMessages()
    ///
    /// \param message pointer to the message to send
    ///
    ///////////////////////////////////////////////////////////////////////////
    void udpSend(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a udp message and block the execution till it is sent
    ///
    /// \param message pointer to the message to send
    ///
    ///////////////////////////////////////////////////////////////////////////
    void blockingUdpSend(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Test whether there is Awaiting udp messages to be sent
    ///
    /// Test if the outgoing queue of messages is empty. If it is,
    /// it means no messages are awaiting to be sent
    ///
    /// \return True if messages are awaiting to be sent
    ///
    ///////////////////////////////////////////////////////////////////////////
    auto hasSendingUdpMessagesAwaiting() const
        -> bool;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send awaiting udp messages
    ///
    /// Asynchronously sends all the messages inside the outgoing queue of
    /// messages by calling sendMessage()
    /// The function recursivly clears the outgoing queue of messages
    ///
    /// \warning This method assumes at least one message waits to be sent
    /// Calling this methods without any message waiting leads to
    /// undefined behavior and/or potential segfaults
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendAwaitingUdpMessages();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Start receiving messages
    ///
    /// Waits for incomming messages. On receive, the buffer of incomming
    /// messages is filled with the incomming message, it calls
    /// transferInMessageToOwner()
    ///
    /// \warning the buffer needs to be empty or it may lead to undefined
    /// behaviors
    ///
    ///////////////////////////////////////////////////////////////////////////
    void startReceivingUdpMessages();



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Helpers
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Get the port
    ///
    /// The port is choosen by the user when creating the Connection
    ///
    /// \returns Port used by the socket
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto getId() const
        -> ::xrn::Id;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Get the port
    ///
    /// The port is choosen by the user when creating the Connection
    ///
    /// \returns Port used by the socket
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto getPort() const
        -> ::std::uint16_t;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Get the address
    ///
    /// The adderss is choosen by the user when creating the Connection
    ///
    /// \returns Address used by the socket
    ///
    ///////////////////////////////////////////////////////////////////////////
    [[ nodiscard ]] auto getAddress() const
        -> ::std::string;



private:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Helpers
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Push incomming mesages to the incomming queue messages
    ///
    /// Convert the ::xrn::network::Message to ::xrn::network::OwnedMessage and
    /// and assign the OwnedMessage's remote as this
    /// Once the ::xrn::network::OwnedMessage is created, it is just pushed to the
    /// queue of incomming messages that can later be pulled by the Owner
    ///
    ///////////////////////////////////////////////////////////////////////////
    void transferInMessageToOwner(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    );



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Tcp helpers
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// Sends the message in front of the queue. Once sent, the message is
    /// deleted
    ///
    /// \param successCallback Function, lambda or class called on success
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendTcpQueueMessage(
        ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// Sends the message in front of the queue. Once sent, the message is
    /// deleted. Skip the check of m_isSendingAllowed
    ///
    /// \param successCallback Function, lambda or class called on success
    ///
    ///////////////////////////////////////////////////////////////////////////
    void forceSendTcpQueueMessage(
        ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// Same as sendNonQueuedTcpMessage(), but does not copy the message, but
    /// sends the message in front of the queue
    /// Once sent, the message is deleted
    ///
    /// \param message Message to send
    /// \param successCallback Function, lambda or class called on success
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendNonQueuedTcpMessage(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
        , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// Same as sendNonQueuedTcpMessage(), but does not copy the message, but
    /// sends the message in front of the queue
    /// Once sent, the message is deleted
    /// Skip the check of m_isSendingAllowed
    ///
    /// \param message Message to send
    /// \param successCallback Function, lambda or class called on success
    ///
    ///////////////////////////////////////////////////////////////////////////
    void forceSendNonQueuedTcpMessage(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
        , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// \param message Message to send
    /// \param successCallback Function, lambda or class called on success
    /// \param bytesAlreadySent Allows to send a message over multiple packet
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendTcpMessage(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
        , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
        , ::std::size_t bytesAlreadySent = 0
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Receive a message
    ///
    /// Asynchronously wait on the asio thread for a messages to arrive
    /// once a message arrived, it's tranfered to the incomming buffer
    /// and the successCallback is called
    /// If already receiving, m_tcpBufferInLocker will block the call till
    /// unlocked
    /// In case of errors, Both Tcp and Tcp connection are closed
    ///
    /// \warning must call m_tcpBufferInLocker.unlock() to release the
    /// buffer once received
    ///
    /// \param successCallback Function, lambda or class called on success
    /// \param bytesAlreadyReceived Allows to receive a message over multiple
    /// packet
    ///
    ///////////////////////////////////////////////////////////////////////////
    void receiveTcpMessage(
        ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
        , ::std::size_t bytesAlreadyReceived = 0
    );



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Udp helpers
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// Sends the message in front of the queue. Once sent, the message is
    /// deleted
    ///
    /// \param successCallback Function, lambda or class called on success
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendUdpQueueMessage(
        ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// Sends the message in front of the queue. Once sent, the message is
    /// deleted. Skip the check of m_isSendingAllowed
    ///
    /// \param successCallback Function, lambda or class called on success
    ///
    ///////////////////////////////////////////////////////////////////////////
    void forceSendUdpQueueMessage(
        ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// Same as sendNonQueuedUdpMessage(), but does not copy the message, but
    /// sends the message in front of the queue
    /// Once sent, the message is deleted
    ///
    /// \param message Message to send
    /// \param successCallback Function, lambda or class called on success
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendNonQueuedUdpMessage(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
        , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a message in the queue
    ///
    /// Same as sendNonQueuedUdpMessage(), but does not copy the message, but
    /// sends the message in front of the queue
    /// Once sent, the message is deleted
    /// Skip the check of m_isSendingAllowed
    ///
    /// \param message Message to send
    /// \param successCallback Function, lambda or class called on success
    ///
    ///////////////////////////////////////////////////////////////////////////
    void forceSendNonQueuedUdpMessage(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
        , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send the header of a message in the queue
    ///
    /// Once sent, if the body is not empty, it is automatically sent as well
    ///
    /// \param message Message to send
    /// \param successCallback Function, lambda or class called on success
    /// \param bytesAlreadySent Allows to send a message over multiple packet
    ///
    ///////////////////////////////////////////////////////////////////////////
    void sendUdpMessage(
        ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
        , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
        , ::std::size_t bytesAlreadySent = 0
    );

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Receive a message
    ///
    /// Asynchronously wait on the asio thread for a messages to arrive
    /// once a message arrived, it's tranfered to the incomming buffer
    /// and the successCallback is called
    /// If already receiving, m_udpBufferInLocker will block the call till
    /// unlocked
    /// In case of errors, Both Udp and Udp connection are closed
    ///
    /// \warning must call m_udpBufferInLocker.unlock() to release the
    /// buffer once received
    ///
    /// \param successCallback Function, lambda or class called on success
    /// \param bytesAlreadyReceived Allows to receive a message over multiple
    /// packet
    ///
    ///////////////////////////////////////////////////////////////////////////
    void receiveUdpMessage(
        ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
        , ::std::size_t bytesAlreadyReceived = 0
    );



private:

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Members
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Owner of the connection
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::xrn::network::AClient<UserEnum>& m_owner;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Create a unique identificating number for every connection
    ///
    /// Server only
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::xrn::Id m_id{ 0 };



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Tcp Members
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    /////////////UserEnum//////////////////////////////////////////////////////////////
    /// \brief Asio socket representing the socket for tcp communication
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::asio::ip::tcp::socket m_tcpSocket;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Buffer allowing messages to be received
    ///
    /// This message will be copied to a queue of incomming messages with
    /// transferInMessageToOwner()
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> m_tcpBufferIn;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Lock the access of the buffer allowing messages to be received
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::mutex m_tcpBufferInLocker;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Queue of all the tcp messages about to be sent
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::xrn::network::detail::Queue<::std::unique_ptr<
        ::xrn::network::Message<UserEnum>
    >> m_tcpMessagesOut;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Signals that messages are currenly being sent
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::atomic_uint8_t m_numberOfTcpSendingInstances{ 0 };

    ///////////////////////////////////////////////////////////////////////////
    /// \brief If false, no messages will be sent
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::atomic_bool m_isTcpSendingAllowed{ false };



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Udp Members
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    /////////////UserEnum//////////////////////////////////////////////////////////////
    /// \brief Asio socket representing the socket for udp communication
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::asio::ip::udp::socket m_udpSocket;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Buffer allowing messages to be received
    ///
    /// This message will be copied to a queue of incomming messages with
    /// transferInMessageToOwner()
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> m_udpBufferIn;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Lock the access of the buffer allowing messages to be received
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::mutex m_udpBufferInLocker;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Queue of all the udp messages about to be sent
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::xrn::network::detail::Queue<::std::unique_ptr<
        ::xrn::network::Message<UserEnum>
    >> m_udpMessagesOut;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Signals that messages are currenly being sent
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::atomic_uint8_t m_numberOfUdpSendingInstances{ 0 };

    ///////////////////////////////////////////////////////////////////////////
    /// \brief If false, no messages will be sent
    ///
    ///////////////////////////////////////////////////////////////////////////
    ::std::atomic_bool m_isUdpSendingAllowed{ false };

};

} // namespace xrn::network

///////////////////////////////////////////////////////////////////////////
// Implementation Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/Connection.impl.hpp>
