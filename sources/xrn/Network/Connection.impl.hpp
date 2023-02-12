///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// constructors
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::Connection<UserEnum>::Connection(
    ::xrn::network::AClient<UserEnum>& owner
)
    : m_owner{ owner }
    , m_tcpSocket{ m_owner.getAsioContext() }
    , m_tcpBufferIn{ ::xrn::network::Message<UserEnum>::ProtocolType::tcp }
    , m_udpSocket{ m_owner.getAsioContext() }
    , m_udpBufferIn{ ::xrn::network::Message<UserEnum>::ProtocolType::udp }
{
    XRN_DEBUG("C{} Created", m_id);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::Connection<UserEnum>::Connection(
    ::asio::ip::tcp::socket&& socket
    , ::xrn::network::AClient<UserEnum>& owner
)
    : m_owner{ owner }
    , m_tcpSocket{ ::std::move(socket) }
    , m_tcpBufferIn{ ::xrn::network::Message<UserEnum>::ProtocolType::tcp }
    , m_udpSocket{ m_owner.getAsioContext() }
    , m_udpBufferIn{ ::xrn::network::Message<UserEnum>::ProtocolType::udp }
{
    XRN_DEBUG("C{} Created", m_id);
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Rule of 5
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::Connection<UserEnum>::~Connection()
{
    this->disconnect(true);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> inline ::xrn::network::Connection<UserEnum>::Connection(
    Connection&& that
) noexcept = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> inline auto ::xrn::network::Connection<UserEnum>::operator=(
    Connection&& that
) noexcept
    -> Connection& = default;



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Connection
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::connectToClient()
{
    if (!m_tcpSocket.is_open()) {
        XRN_THROW("TCP{} Invalid socket", m_id);
    }

    if (!m_owner.onConnect(this->shared_from_this())) {
        XRN_LOG("TCP{} Connection Refused", m_id);
        return this->disconnect();
    }

    XRN_LOG("TCP{} Connection accepted", m_id);
    this->startReceivingTcpMessage();
    m_isSendingAllowed = true;
    // this->setUdpTarget();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::connectToServer(
    const ::std::string& host
    , const ::std::uint16_t port
)
{
    XRN_LOG("TCP{} request connect to server {}:{}", m_id, host, port);
    m_tcpSocket.async_connect(
        ::asio::ip::tcp::endpoint{ ::asio::ip::address::from_string(host), port }
        , [this, &host = host, port](
            const ::std::error_code& errCode
        ) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("TCP{} Connection canceled", m_id);
                } else {
                    XRN_ERROR(
                        "TCP{} Failed to connect to {}:{}: {}"
                        , m_id
                        , host
                        , port
                        , errCode.message()
                    );
                }
                return;
            }

            if (!m_owner.onConnect(this->shared_from_this())) {
                return this->disconnect();
            }

            XRN_LOG("TCP{} Connection accepted", m_id);
            m_isSendingAllowed = true;
            this->startReceivingTcpMessage();
            // this->setUdpTarget();
        }
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::disconnect(
    bool isAlreadyDestroyed // = false
)
{
    if (m_tcpSocket.is_open()) {
        XRN_DEBUG("C{} Disconnecting...", m_id);

        if (!isAlreadyDestroyed) {
            m_owner.onDisconnect(this->shared_from_this());
        }

        m_isSendingAllowed = false;

        // tcp
        m_tcpSocket.cancel();
        m_tcpSocket.close();

        // udp
        if (m_udpSocket.is_open()) {
            m_udpSocket.cancel();
            m_udpSocket.close();
        }

        // probably waiting, notify it
        m_owner.notifyIncommingMessageQueue();

        XRN_DEBUG("C{} ...Disconnected", m_id);

        if (!isAlreadyDestroyed) {
            m_owner.removeConnection(this->shared_from_this());
        }
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Tcp
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::tcpSend(
    ::xrn::network::Message<UserEnum>& message
)
{
    if (!m_owner.onSend(message, this->shared_from_this())) {
        XRN_ERROR("TCP{} Send canceled", m_id);
        return;
    }

    m_tcpMessagesOut.push_back(message);
    this->sendAwaitingTcpMessages();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::tcpSend(
    ::xrn::network::Message<UserEnum>&& message
)
{
    if (!m_owner.onSend(message, this->shared_from_this())) {
        XRN_ERROR("TCP{} Send canceled", m_id);
        return;
    }

    m_tcpMessagesOut.push_back(::std::move(message));
    this->sendAwaitingTcpMessages();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::blockingTcpSend(
    const ::xrn::network::Message<UserEnum>& message
)
{
    if (!m_owner.onSend(message, this->shared_from_this())) {
        XRN_ERROR("TCP{} Send canceled", m_id);
        return;
    }

    ::std::mutex locker;
    locker.lock();

    this->sendNonQueuedTcpMessage(message, [&locker](){ locker.unlock(); });

    // selfblock (avoid using a condition variable)
    locker.lock();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::blockingTcpSend(
    ::xrn::network::Message<UserEnum>&& message
)
{
    if (!m_owner.onSend(message, this->shared_from_this())) {
        XRN_ERROR("TCP{} Send canceled", m_id);
        return;
    }

    ::std::mutex locker;
    locker.lock();

    this->sendNonQueuedTcpMessage(::std::move(message), [&locker](){ locker.unlock(); });

    // selfblock (avoid using a condition variable)
    locker.lock();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Connection<UserEnum>::hasSendingTcpMessagesAwaiting() const
    -> bool
{
    return !m_tcpMessagesOut.empty();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendAwaitingTcpMessages()
{
    ++m_numberOfTcpSendingInstances;
    this->sendTcpQueueMessage([this](){ --m_numberOfTcpSendingInstances; });
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::startReceivingTcpMessage()
{
    XRN_DEBUG("TCP{} Start receiving messages", m_id);
    this->receiveTcpMessage(
        [this](){
            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner(::std::move(m_tcpBufferIn));
            // m_tcpBufferInLocker.unlock();
            this->startReceivingTcpMessage();
        }
    );
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// UDP
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// template <
    // ::xrn::network::detail::constraint::isValidEnum UserEnum
// > void ::xrn::network::Connection<UserEnum>::setUdpTarget(
    // const ::std::string& host
    // , const ::std::uint16_t port
// )
// {
    // send udp informations
    // this->forceSendNonQueuedTcpMessage(
        // [](){}
        // , ::network::Message<UserEnum>{
            // ::network::Message<UserEnum>::SystemType::udpInformations,
            // m_connection->udp.getPort()
        // }
    // );

    // receive the udp information
    // this->receiveMessage<[](::std::shared_ptr<::network::Connection<UserEnum>> connection){
        // if (
            // connection->tcp.m_bufferIn.getTypeAsSystemType() !=
            // ::network::Message<UserEnum>::SystemType::udpInformations
        // ) {
            // ::std::cerr << "[ERROR:TCP:" << connection->getId() << "] Failed to setup Udp, "
                // << "unexpected message received: " << connection->tcp.m_bufferIn.getTypeAsInt() << ".\n";
            // connection->disconnect();
        // } else {
            // connection->udp.target(
                // connection->tcp.getAddress(),
                // connection->tcp.m_bufferIn.template pull<::std::uint16_t>()
            // );
            // connection->tcp.m_isSendingAllowed = true;
            // connection->m_owner.onConnectionValidated(connection);
            // connection->tcp.m_blocker.notify_all();
        // }
    // }>();
// }

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::setUdpTarget(
    const ::std::string& host [[ maybe_unused ]]
    , const ::std::uint16_t port [[ maybe_unused ]]
)
{
    // m_udpSocket.async_connect(
        // ::asio::ip::udp::endpoint{ ::asio::ip::address::from_string(host), port }
        // , [=](const ::std::error_code& errCode) {
            // if (errCode) {
                // if (errCode == ::asio::error::operation_aborted) {
                    // XRN_ERROR("UDP{}: Target canceled", m_id);
                // } else {
                    // XRN_ERROR("UDP{}: Failed to target {}:{}", m_id, host, port);
                // }
                // this->disconnect();
                // return;
            // }

            // XRN_LOG("UDP{}: targeting {}:{}", m_id, host, port);
            // m_isSendingAllowed = true;
            // auto locker{ m_udpMessagesOut.lock() };
            // this->sendAwaitingUdpMessages();
        // }
    // );
    // XRN_DEBUG("UDP{}: request targeting {}:{}", m_id, host, port);
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Connection<UserEnum>::getId() const
    -> ::xrn::Id
{
    return m_id;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Connection<UserEnum>::getPort() const
    -> ::std::uint16_t
{
    return m_tcpSocket.local_endpoint().port();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Connection<UserEnum>::getAddress() const
    -> ::std::string
{
    return m_tcpSocket.local_endpoint().address().to_string();
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::transferInMessageToOwner(
    const ::xrn::network::Message<UserEnum>& buffer
)
{
    m_owner.pushIncommingMessage(this->shared_from_this(), buffer);
    m_owner.notifyIncommingMessageQueue();
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Tcp helpers
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpQueueMessage(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    if (!m_isSendingAllowed) {
        XRN_ERROR(
            "TCP{} {}"
            , m_id
            , "The connection is currently unable to send messages"
        );
        return;
    }

    if (
        ::std::scoped_lock locker{ m_tcpMessagesOut.getMutex() };
        !m_tcpMessagesOut.lockFreeEmpty()
    ) {
        XRN_DEBUG("TCP{} Start sending messages", m_id);
        this->sendTcpMessageHeader(m_tcpMessagesOut.lockFree_pop_front(), successCallback);
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendTcpQueueMessage(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    if (
        ::std::scoped_lock locker{ m_tcpMessagesOut.getMutex() };
        !m_tcpMessagesOut.lockFreeEmpty()
    ) {
        XRN_DEBUG("TCP{} -> Start sending messages", m_id);
        this->sendTcpMessageHeader(m_tcpMessagesOut.lockFree_pop_front(), successCallback);
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendNonQueuedTcpMessage(
    ::xrn::network::Message<UserEnum>&& message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    if (!m_isSendingAllowed) {
        XRN_ERROR(
            "TCP{} {}"
            , m_id
            , "Messages are already being sent or the connection cannot send messages"
        );
        return;
    }
    this->sendTcpMessageHeader(::std::move(message), successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendNonQueuedTcpMessage(
    ::xrn::network::Message<UserEnum>&& message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    this->sendTcpMessageHeader(::std::move(message), successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpMessageHeader(
    ::xrn::network::Message<UserEnum>&& message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the header and call the sending of the body
    auto lambda{
        [this, bytesAlreadySent, successCallback](
            ::xrn::network::Message<UserEnum>&& message
            , const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("TCP{} Send canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_LOG("TCP{} Connection closed", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{} Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been sent
            if (bytesAlreadySent + length < message.getHeaderSize()) {
                XRN_DEBUG(
                    "TCP{} -> Header not fully sent ({} / {})"
                    , m_id
                    , bytesAlreadySent + length
                    , message.getHeaderSize()
                );
                return this->sendTcpMessageHeader(
                    ::std::move(message)
                    , successCallback
                    , bytesAlreadySent + length
                );
            }
            XRN_DEBUG("TCP{} -> Header sent", m_id);
            return successCallback(); // TODO

            // send body if present
            if (message.getBodySize()) {
                XRN_DEBUG("TCP{} -> Header sent", m_id);
                return this->sendTcpMessageBody(
                    ::std::move(message)
                    , successCallback
                );
            }

            XRN_DEBUG("TCP{} <- Body is empty", m_id);
            successCallback();
        }
    };

    XRN_DEBUG(
        "TCP{} -> Request header (size {})"
        , m_id
        , message.getHeaderSize() - bytesAlreadySent
    );
    m_tcpSocket.async_send(
        ::asio::buffer(
            &message.getHeader() + bytesAlreadySent
            , message.getHeaderSize() - bytesAlreadySent
        ), ::std::bind_front(lambda, ::std::move(message))
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpMessageBody(
    ::xrn::network::Message<UserEnum>&& message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the body
    auto lambda{
        [this, bytesAlreadySent, successCallback](
            ::xrn::network::Message<UserEnum>&& message
            , const ::std::error_code& errCode
            , const ::std::size_t length [[ maybe_unused ]]
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("TCP{} Send canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_LOG("TCP{} Connection closed", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{} Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been sent
            if (bytesAlreadySent + length < message.getBodySize()) {
                XRN_DEBUG(
                    "TCP{} -> Body not fully sent ({} / {})"
                    , m_id
                    , bytesAlreadySent + length
                    , message.getBodySize()
                );
                return this->sendTcpMessageBody(
                    ::std::move(message)
                    , successCallback
                    , bytesAlreadySent + length
                );
            }

            XRN_DEBUG("TCP{} -> Body sent", m_id);
            successCallback();
        }
    };

    XRN_DEBUG(
        "TCP{} -> Request body (size {})"
        , m_id
        , message.getBodySize() - bytesAlreadySent
    );
    m_tcpSocket.async_send(
        ::asio::buffer(
            &message.getBody() + bytesAlreadySent
            , message.getBodySize() - bytesAlreadySent
        ), ::std::bind_front(lambda, ::std::move(message))
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveTcpMessage(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    // m_tcpBufferInLocker.lock();
    this->receiveTcpMessageHeader(successCallback, 0);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveTcpMessageHeader(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    , ::std::size_t bytesAlreadyReceived // = 0
)
{
    // lambda called by asio to receive the body
    auto lambda{
        [this, bytesAlreadyReceived, successCallback](
            const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("TCP{} Receive canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{} Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                // m_tcpBufferInLocker.unlock();
                return;
            }

            XRN_DEBUG(
                "TCP{} <- Is header fully received??? ({} / {})"
                , m_id
                , bytesAlreadyReceived + length
                , m_tcpBufferIn.getHeaderSize()
            );
            // if not everything has been received
            if (bytesAlreadyReceived + length < m_tcpBufferIn.getHeaderSize()) {
                XRN_DEBUG(
                    "TCP{} <- Header not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_tcpBufferIn.getHeaderSize()
                );
                // m_tcpBufferInLocker.unlock();
                return this->receiveTcpMessageHeader(
                    successCallback, bytesAlreadyReceived + length
                );
            }
            XRN_DEBUG("TCP{} <- Header received", m_id);

            m_tcpBufferIn.updateBodySize();
            if (!m_tcpBufferIn.isBodyEmpty()) {
                XRN_DEBUG("TCP{} <- Header received", m_id);
                // m_tcpBufferInLocker.unlock();
                return this->receiveTcpMessageBody(
                    successCallback, bytesAlreadyReceived + length
                );
            }

            XRN_DEBUG("TCP{} <- Body is empty", m_id);
            // send the message to the queue and start to receive messages again
            successCallback();
            // m_tcpBufferInLocker.unlock();
        }
    };

    XRN_DEBUG(
        "TCP{} <- Request header (size {})"
        , m_id
        , m_tcpBufferIn.getHeaderSize() - bytesAlreadyReceived
    );

    m_tcpSocket.async_receive(
        ::asio::buffer(
            &m_tcpBufferIn.getHeader() + bytesAlreadyReceived
            , m_tcpBufferIn.getHeaderSize() - bytesAlreadyReceived
        ), lambda
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveTcpMessageBody(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    , ::std::size_t bytesAlreadyReceived // = 0
)
{
    // lambda called by asio to receive the body
    auto lambda{
        [this, bytesAlreadyReceived, successCallback](
            const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("TCP{} Receive canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_LOG("TCP{} Connection closed", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{} Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                // m_tcpBufferInLocker.unlock();
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_tcpBufferIn.getBodySize()) {
                XRN_DEBUG(
                    "TCP{} <- Body not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_tcpBufferIn.getBodySize()
                );
                // m_tcpBufferInLocker.unlock();
                return this->receiveTcpMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            XRN_DEBUG("TCP{} <- Body received", m_id);
            successCallback();
            // m_tcpBufferInLocker.unlock();
        }
    };

    XRN_DEBUG(
        "TCP{} <- Request body (size {})"
        , m_id
        , m_tcpBufferIn.getBodySize() - bytesAlreadyReceived
    );
    m_tcpSocket.async_receive(
        ::asio::buffer(
            &m_tcpBufferIn.getBody() + bytesAlreadyReceived
            , m_tcpBufferIn.getBodySize() - bytesAlreadyReceived
        ), lambda
    );
}
