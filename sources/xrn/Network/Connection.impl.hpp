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
    , m_udpSocket{ m_owner.getAsioContext() }
{
    m_udpSocket.open(::asio::ip::udp::v4());
    m_udpSocket.bind(::asio::ip::udp::endpoint(::asio::ip::udp::v4(), 0));
    XRN_DEBUG("C{} Created", m_id);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::Connection<UserEnum>::Connection(
    ::asio::ip::tcp::socket&& socket
    , ::xrn::network::AClient<UserEnum>& owner
    , ::xrn::Id clientId
)
    : m_owner{ owner }
    , m_id{ clientId }
    , m_tcpSocket{ ::std::move(socket) }
    , m_udpSocket{ m_owner.getAsioContext() }
{
    m_udpSocket.open(::asio::ip::udp::v4());
    m_udpSocket.bind(::asio::ip::udp::endpoint(::asio::ip::udp::v4(), 0));
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
    XRN_DEBUG("C{} Deleting...", m_id);
    this->disconnect(true);
    XRN_DEBUG("C{} Deleted", m_id);
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
        XRN_LOG("TCP{} Connection refused (onConnect)", m_id);
        return this->disconnect();
    }

    XRN_LOG("TCP{} Connection accepted", m_id);
    this->retrieveUdpInformation();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::connectToServer(
    const ::std::string& host
    , const ::std::uint16_t port
)
{
    XRN_LOG("TCP{} -> Request connection to server {}:{}", m_id, host, port);
    m_tcpSocket.async_connect(
        ::asio::ip::tcp::endpoint{ ::asio::ip::address::from_string(host), port }
        , [this, &host = host, port](
            const ::std::error_code& errCode
        ) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_WARNING("TCP Connection canceled");
                } else {
                    XRN_ERROR(
                        "TCP{} Failed to connect to {}:{}: {}"
                        , m_id
                        , host
                        , port
                        , errCode.message()
                    );
                }
                return this->disconnect();
            }

            if (!m_owner.onConnect(this->shared_from_this())) {
                XRN_LOG("TCP{} Connection refused (onConnect)", m_id);
                return this->disconnect();
            }

            XRN_LOG("TCP{} Connection accepted", m_id);
            this->retrieveUdpInformation();
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

        m_isTcpSendingAllowed = false;

        // tcp
        m_tcpSocket.cancel();
        m_tcpSocket.close();

        // udp
        if (m_udpSocket.is_open()) {
            m_udpSocket.cancel();
            m_udpSocket.close();
        }

        XRN_DEBUG("C{} ...Disconnected", m_id);

        // might be waiting, notify it
        m_owner.notifyIncommingMessageQueue();

        if (!isAlreadyDestroyed) {
            m_owner.removeConnection(this->shared_from_this());
        }
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Connection<UserEnum>::isConnected()
    -> bool
{
    return m_tcpSocket.is_open() && m_udpSocket.is_open();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::startReceivingMessages()
{
    ::asio::post(
        m_owner.getAsioContext()
        , [this]() {
            XRN_LOG("C{} Connection successfull, start receiving...", m_id);
            m_isTcpSendingAllowed = true;
            this->startReceivingTcpMessages();
            m_isUdpSendingAllowed = true;
            this->startReceivingUdpMessages();
        }
    );
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
> void ::xrn::network::Connection<UserEnum>::synchronizeId()
{
    if (m_owner.isServer()) {
        // send udp informations
        auto message{ ::std::make_unique<::xrn::network::Message<UserEnum>>(
            ::xrn::network::Message<UserEnum>::SystemType::builtinIdInformation
        ) };
        *message << m_id;
        this->forceSendNonQueuedTcpMessage(
            ::std::move(message)
            , [this](){ this->startReceivingMessages(); } // next step to start
        );
    } else {
        // receive the udp information
        this->receiveTcpMessage(
            [this](){
                if (
                    m_tcpBufferIn->getTypeAsSystemType() !=
                    ::xrn::network::Message<UserEnum>::SystemType::builtinIdInformation
                ) {
                    XRN_ERROR(
                        "TCP{} Failed to synchronize ID (unexpected message received {})"
                        , m_id
                        , m_tcpBufferIn->getTypeAsInt()
                    );
                    this->disconnect();
                    return;
                }
                m_id = m_tcpBufferIn->template pull<::xrn::Id>();
                XRN_DEBUG("TCP{} ID information received", m_id);

                m_owner.notifyIncommingMessageQueue();

                this->startReceivingMessages(); // next step to start
            }
        );
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::tcpSend(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    message->resetPullPosition();
    if (!m_owner.onSend(*message, this->shared_from_this())) {
        XRN_WARNING("TCP Send canceled");
        return;
    }

    m_tcpMessagesOut.push_back(::std::move(message));
    this->sendAwaitingTcpMessages();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::blockingTcpSend(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    message->resetPullPosition();
    if (!m_owner.onSend(*message, this->shared_from_this())) {
        XRN_WARNING("TCP Send canceled");
        return;
    }

    ::std::mutex locker;
    locker.lock();

    this->sendNonQueuedTcpMessage(::std::move(message), [&locker](){ locker.unlock(); });

    // selfblock (avoid using a condition variable)
    locker.lock();
    message = nullptr;
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
> void ::xrn::network::Connection<UserEnum>::startReceivingTcpMessages()
{
    XRN_DEBUG("TCP{} Start receiving messages", m_id);

    this->receiveTcpMessage(
        [this](){
            XRN_DEBUG("message received");
            // send the message to the queue and start to receive messages again
            if (m_tcpBufferIn->getType() == UserEnum::message) {
                XRN_DEBUG("Preparing to print pull");
                ::xrn::Id id;
                ::std::string string;
                *m_tcpBufferIn >> id >> string;
                XRN_DEBUG("{} <- '{}'", id, string);
                m_tcpBufferIn->resetPullPosition();
            }
            this->transferInMessageToOwner(::std::move(m_tcpBufferIn));

            // m_tcpBufferInLocker.unlock();
            this->startReceivingTcpMessages();
        }
    );
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Udp
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::retrieveUdpInformation()
{
    XRN_LOG("TCP{} -> Request udp information of the server", m_id);

    // send udp informations
    {
        auto message{ ::std::make_unique<::xrn::network::Message<UserEnum>>(
            ::xrn::network::Message<UserEnum>::SystemType::builtinUdpInformation
        ) };
        *message << m_udpSocket.local_endpoint().port();
        this->forceSendNonQueuedTcpMessage(::std::move(message), [](){});
    }

    // receive the udp information
    this->receiveTcpMessage(
        [this](){
            if (
                m_tcpBufferIn->getTypeAsSystemType() !=
                ::xrn::network::Message<UserEnum>::SystemType::builtinUdpInformation
            ) {
                XRN_ERROR(
                    "TCP{} Failed to setup UDP (unexpected message received {})"
                    , m_id
                    , m_tcpBufferIn->getTypeAsInt()
                );
                this->disconnect();
                return;
            }
            XRN_DEBUG("TCP{} UDP information received", m_id);
            auto port{ m_tcpBufferIn->template pull<::std::uint16_t>() };

            this->setUdpTarget(this->getAddress(), port); // next step to start
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::setUdpTarget(
    const ::std::string& host
    , const ::std::uint16_t port
)
{
    XRN_LOG("UDP{} Request connection to server {}:{}", m_id, host, port);

    m_udpSocket.async_connect(
        ::asio::ip::udp::endpoint{ ::asio::ip::address::from_string(host), port },
        [=](const ::std::error_code& errCode) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_WARNING("UDP Target canceled");
                } else if (errCode == ::asio::error::eof) {
                    XRN_LOG("UDP{} Connection closed", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{} Failed to target {}:{}", m_id, host, port);
                    this->disconnect();
                }
                return;
            }

            XRN_LOG("UDP{} Target setup", m_id);
            this->synchronizeId(); // next step to start
        }
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::udpSend(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    if (message->getType() == UserEnum::message) {
        XRN_DEBUG("Preparing to print pull");
        ::xrn::Id id;
        ::std::string string;
        *message >> id >> string;
        XRN_DEBUG("{} <- '{}'", id, string);
        m_tcpBufferIn->resetPullPosition();
    }
    if (!m_owner.onSend(*message, this->shared_from_this())) {
        XRN_WARNING("UDP Send canceled");
        return;
    }

    m_udpMessagesOut.push_back(::std::move(message));
    this->sendAwaitingUdpMessages();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::blockingUdpSend(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    message->resetPullPosition();
    if (!m_owner.onSend(*message, this->shared_from_this())) {
        XRN_WARNING("UDP Send canceled");
        return;
    }

    ::std::mutex locker;
    locker.lock();

    this->sendNonQueuedUdpMessage(::std::move(message), [&locker](){ locker.unlock(); });

    // selfblock (avoid using a condition variable)
    locker.lock();
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Connection<UserEnum>::hasSendingUdpMessagesAwaiting() const
    -> bool
{
    return !m_udpMessagesOut.empty();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendAwaitingUdpMessages()
{
    ++m_numberOfUdpSendingInstances;
    this->sendUdpQueueMessage([this](){ --m_numberOfUdpSendingInstances; });
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::startReceivingUdpMessages()
{
    XRN_DEBUG("UDP{} Start receiving messages", m_id);

    this->receiveUdpMessage(
        [this](){
            XRN_DEBUG("message received");
            // send the message to the queue and start to receive messages again
            if (m_udpBufferIn->getType() == UserEnum::message) {
                XRN_DEBUG("Preparing to print pull");
                ::xrn::Id id;
                ::std::string string;
                *m_udpBufferIn >> id >> string;
                XRN_DEBUG("{} <- '{}'", id, string);
                m_tcpBufferIn->resetPullPosition();
            }

            this->transferInMessageToOwner(::std::move(m_udpBufferIn));

            // m_udpBufferInLocker.unlock();
            this->startReceivingUdpMessages();
        }
    );
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
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    if (message->getType() == UserEnum::message) {
        XRN_DEBUG("Preparing to print pull");
        ::xrn::Id id;
        ::std::string string;
        *message >> id >> string;
        XRN_DEBUG("{} <- '{}'", id, string);
        m_tcpBufferIn->resetPullPosition();
    }
    m_owner.pushIncommingMessage(this->shared_from_this(), ::std::move(message));
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
    if (!m_isTcpSendingAllowed) {
        XRN_ERROR(
            "TCP{} {}"
            , m_id
            , "The connection is currently unable to send messages"
        );
        return;
    }

    this->forceSendTcpQueueMessage(successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendTcpQueueMessage(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    XRN_DEBUG("locking");
    m_tcpMessagesOut.getMutex().lock();
    if (!m_tcpMessagesOut.lockFreeEmpty()) {
        auto message{ m_tcpMessagesOut.lockFree_pop_front() };
        m_tcpMessagesOut.getMutex().unlock();
        XRN_DEBUG("unlocking");

        XRN_DEBUG("TCP{} -> Start sending messages", m_id);
        this->sendTcpMessage(::std::move(message), successCallback);
    } else {
        m_tcpMessagesOut.getMutex().unlock();
        XRN_DEBUG("unlocking");
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendNonQueuedTcpMessage(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    if (!m_isTcpSendingAllowed) {
        XRN_ERROR(
            "TCP{} {}"
            , m_id
            , "Messages are already being sent or the connection cannot send messages"
        );
        return;
    }
    this->sendTcpMessage(::std::move(message), successCallback);
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendNonQueuedTcpMessage(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    this->sendTcpMessage(::std::move(message), successCallback);
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpMessage(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the body
    auto lambda{
        [this, bytesAlreadySent, successCallback](
            ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
            , const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_WARNING("TCP Send canceled");
                } else if (errCode == ::asio::error::eof) {
                    XRN_LOG("TCP{} Connection closed", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{} Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if packet has not been fully sent
            XRN_DEBUG(
                "TCP{} <- Is message fully sent? {} ({} / {})"
                , m_id
                , !(bytesAlreadySent + length < message->getSize())
                , bytesAlreadySent + length
                , message->getSize()
            );
            if (bytesAlreadySent + length < message->getSize()) {
                XRN_DEBUG(
                    "TCP{} -> message not fully sent ({} / {})"
                    , m_id
                    , bytesAlreadySent + length
                    , message->getBodySize()
                );
                return this->sendTcpMessage(
                    ::std::move(message)
                    , successCallback
                    , bytesAlreadySent + length
                );
            }

            XRN_DEBUG("TCP{} -> Message sent", m_id);
            successCallback();
        }
    };

    XRN_DEBUG(
        "TCP{} -> Request (size:{};Sent:{})"
        , m_id
        , m_tcpBufferIn->getSize() - bytesAlreadySent
        , bytesAlreadySent
    );
    m_tcpSocket.async_send(
        ::asio::buffer(
            message->getAddr() + bytesAlreadySent
            , message->getSize() - bytesAlreadySent
        )
        , ::std::bind_front(lambda, ::std::move(message))
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveTcpMessage(
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
                    XRN_WARNING("TCP Receive canceled");
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

            // if header has not been received
            XRN_DEBUG(
                "TCP{} <- Is header fully received? {} ({} / {})"
                , m_id
                , !(bytesAlreadyReceived + length < m_tcpBufferIn->getHeaderSize())
                , bytesAlreadyReceived + length
                , m_tcpBufferIn->getHeaderSize()
            );
            if (bytesAlreadyReceived + length < m_tcpBufferIn->getHeaderSize()) {
                XRN_DEBUG(
                    "TCP{} <- Header not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_tcpBufferIn->getHeaderSize()
                );
                // m_tcpBufferInLocker.unlock();
                return this->receiveTcpMessage(
                    successCallback, bytesAlreadyReceived + length
                );
            }

            // if body has not been received
            XRN_DEBUG(
                "TCP{} <- Is body fully received? {} ({} / {})"
                , m_id
                , !(bytesAlreadyReceived + length < m_tcpBufferIn->getSize())
                , bytesAlreadyReceived + length
                , m_tcpBufferIn->getSize()
            );
            if (bytesAlreadyReceived + length < m_tcpBufferIn->getSize()) {
                XRN_DEBUG(
                    "TCP{} <- Body not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_tcpBufferIn->getSize()
                );
                // m_tcpBufferInLocker.unlock();
                return this->receiveTcpMessage(successCallback, bytesAlreadyReceived + length);
            }

            XRN_DEBUG("TCP{} <- Body received", m_id);
            successCallback();
            // m_tcpBufferInLocker.unlock();
        }
    };

    XRN_DEBUG(
        "TCP{} <- Request (size:{};Received{})"
        , m_id
        , m_tcpBufferIn->getSize() - bytesAlreadyReceived
        , bytesAlreadyReceived
    );
    m_tcpSocket.async_receive(
        ::asio::buffer(
            m_tcpBufferIn->getAddr() + bytesAlreadyReceived
            , m_tcpBufferIn->getSize() - bytesAlreadyReceived
        )
        , lambda
    );
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Udp helpers
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendUdpQueueMessage(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    if (!m_isUdpSendingAllowed) {
        XRN_ERROR(
            "UDP{} {}"
            , m_id
            , "The connection is currently unable to send messages"
        );
        return;
    }

    this->forceSendUdpQueueMessage(successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendUdpQueueMessage(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    XRN_DEBUG("locking");
    m_udpMessagesOut.getMutex().lock();
    if (!m_udpMessagesOut.lockFreeEmpty()) {
        auto message{ m_udpMessagesOut.lockFree_pop_front() };
        m_udpMessagesOut.getMutex().unlock();
        XRN_DEBUG("unlocking");

        XRN_DEBUG("UDP{} -> Start sending messages", m_id);
        this->sendUdpMessage(::std::move(message), successCallback);
    } else {
        m_udpMessagesOut.getMutex().unlock();
        XRN_DEBUG("unlocking");
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendNonQueuedUdpMessage(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    if (!m_isUdpSendingAllowed) {
        XRN_ERROR(
            "UDP{} {}"
            , m_id
            , "Messages are already being sent or the connection cannot send messages"
        );
        return;
    }
    this->sendUdpMessage(::std::move(message), successCallback);
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendNonQueuedUdpMessage(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    this->sendUdpMessage(::std::move(message), successCallback);
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendUdpMessage(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the body
    auto lambda{
        [this, bytesAlreadySent, successCallback](
            ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
            , const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_WARNING("UDP Send canceled");
                } else if (errCode == ::asio::error::eof) {
                    XRN_LOG("UDP{} Connection closed", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{} Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if packet has not been fully sent
            XRN_DEBUG(
                "UDP{} <- Is message fully sent? {} ({} / {})"
                , m_id
                , !(bytesAlreadySent + length < message->getSize())
                , bytesAlreadySent + length
                , message->getSize()
            );
            if (bytesAlreadySent + length < message->getSize()) {
                XRN_DEBUG(
                    "UDP{} -> message not fully sent ({} / {})"
                    , m_id
                    , bytesAlreadySent + length
                    , message->getBodySize()
                );
                return this->sendUdpMessage(
                    ::std::move(message)
                    , successCallback
                    , bytesAlreadySent + length
                );
            }

            XRN_DEBUG("UDP{} -> Message sent", m_id);
            successCallback();
        }
    };

    XRN_DEBUG(
        "UDP{} -> Request (size:{};Sent:{})"
        , m_id
        , m_udpBufferIn->getSize() - bytesAlreadySent
        , bytesAlreadySent
    );
    m_udpSocket.async_send(
        ::asio::buffer(
            message->getAddr() + bytesAlreadySent
            , message->getSize() - bytesAlreadySent
        )
        , ::std::bind_front(lambda, ::std::move(message))
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveUdpMessage(
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
                    XRN_WARNING("UDP Receive canceled");
                } else if (errCode == ::asio::error::eof) {
                    XRN_LOG("UDP{} Connection closed", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{} Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                // m_udpBufferInLocker.unlock();
                return;
            }

            // if header has not been received
            XRN_DEBUG(
                "UDP{} <- Is header fully received? {} ({} / {})"
                , m_id
                , !(bytesAlreadyReceived + length < m_udpBufferIn->getHeaderSize())
                , bytesAlreadyReceived + length
                , m_udpBufferIn->getHeaderSize()
            );
            if (bytesAlreadyReceived + length < m_udpBufferIn->getHeaderSize()) {
                XRN_DEBUG(
                    "UDP{} <- Header not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_udpBufferIn->getHeaderSize()
                );
                // m_udpBufferInLocker.unlock();
                return this->receiveUdpMessage(
                    successCallback, bytesAlreadyReceived + length
                );
            }

            // if body has not been received
            XRN_DEBUG(
                "UDP{} <- Is body fully received? {} ({} / {})"
                , m_id
                , !(bytesAlreadyReceived + length < m_udpBufferIn->getSize())
                , bytesAlreadyReceived + length
                , m_udpBufferIn->getSize()
            );
            if (bytesAlreadyReceived + length < m_udpBufferIn->getSize()) {
                XRN_DEBUG(
                    "UDP{} <- Body not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_udpBufferIn->getSize()
                );
                // m_udpBufferInLocker.unlock();
                return this->receiveUdpMessage(successCallback, bytesAlreadyReceived + length);
            }

            XRN_DEBUG("UDP{} <- Body received", m_id);
            successCallback();
            // m_udpBufferInLocker.unlock();
        }
    };

    XRN_DEBUG(
        "UDP{} <- Request (size:{};Received{})"
        , m_id
        , m_udpBufferIn->getSize() - bytesAlreadyReceived
        , bytesAlreadyReceived
    );
    m_udpSocket.async_receive(
        ::asio::buffer(
            m_udpBufferIn->getAddr() + bytesAlreadyReceived
            , m_udpBufferIn->getSize() - bytesAlreadyReceived
        )
        , lambda
    );
}
