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
    , m_tcpBufferIn{
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            ::xrn::network::Message<UserEnum>::ProtocolType::tcp
        )
    }, m_udpSocket{ m_owner.getAsioContext() }
    , m_udpBufferIn{
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            ::xrn::network::Message<UserEnum>::ProtocolType::udp
        )
    }
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
)
    : m_owner{ owner }
    , m_tcpSocket{ ::std::move(socket) }
    , m_tcpBufferIn{
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            ::xrn::network::Message<UserEnum>::ProtocolType::tcp
        )
    }, m_udpSocket{ m_owner.getAsioContext() }
    , m_udpBufferIn{
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            ::xrn::network::Message<UserEnum>::ProtocolType::udp
        )
    }
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
                return;
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
        XRN_WARNING("TCP Send canceled");
        return;
    }

    m_tcpMessagesOut.push_back(::std::make_unique<::xrn::network::Message<UserEnum>>(
        message
    ));
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
        XRN_WARNING("TCP Send canceled");
        return;
    }

    m_tcpMessagesOut.push_back(::std::make_unique<::xrn::network::Message<UserEnum>>(
        ::std::move(message)
    ));
    this->sendAwaitingTcpMessages();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::blockingTcpSend(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    if (!m_owner.onSend(message, this->shared_from_this())) {
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
> void ::xrn::network::Connection<UserEnum>::startReceivingTcpMessage()
{
    XRN_DEBUG("TCP{} Start receiving messages", m_id);
    this->receiveTcpMessage(
        [this](){
            XRN_DEBUG("message received");
            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner(::std::move(m_tcpBufferIn));

            // recreate a buffer as it has been set to null by move
            m_tcpBufferIn = ::std::make_unique<::xrn::network::Message<UserEnum>>(
                ::xrn::network::Message<UserEnum>::ProtocolType::tcp
            );

            // m_tcpBufferInLocker.unlock();
            this->startReceivingTcpMessage();
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
    this->forceSendNonQueuedTcpMessage(
        ::std::make_unique<::xrn::network::Message<UserEnum>>(
            ::xrn::network::Message<UserEnum>::SystemType::builtinUdpInformation
            , ::xrn::network::Message<UserEnum>::ProtocolType::tcp // sent by tcp
            , m_udpSocket.local_endpoint().port()
        )
        , [](){}
    );

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

            // recreate a buffer
            m_tcpBufferIn = ::std::make_unique<::xrn::network::Message<UserEnum>>(
                ::xrn::network::Message<UserEnum>::ProtocolType::tcp
            );

            this->setUdpTarget(this->getAddress(), port);
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
                    XRN_LOG("TCP{} Connection closed", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{} Failed to target {}:{}", m_id, host, port);
                    this->disconnect();
                }
                return;
            }

            XRN_LOG("UDP{} Connection accepted", m_id);
            ::asio::post(
                m_owner.getAsioContext()
                , [this]() {
                    m_isTcpSendingAllowed = true;
                    this->startReceivingTcpMessage();
                    m_isUdpSendingAllowed = true;
                    this->startReceivingUdpMessage();
                }
            );
        }
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::udpSend(
    ::xrn::network::Message<UserEnum>& message
)
{
    if (!m_owner.onSend(message, this->shared_from_this())) {
        XRN_WARNING("TCP Send canceled");
        return;
    }

    m_udpMessagesOut.push_back(::std::make_unique<::xrn::network::Message<UserEnum>>(
        message
    ));
    this->sendAwaitingUdpMessages();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::udpSend(
    ::xrn::network::Message<UserEnum>&& message
)
{
    if (!m_owner.onSend(message, this->shared_from_this())) {
        XRN_WARNING("TCP Send canceled");
        return;
    }

    m_udpMessagesOut.push_back(::std::make_unique<::xrn::network::Message<UserEnum>>(
        ::std::move(message)
    ));
    this->sendAwaitingUdpMessages();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::blockingUdpSend(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
)
{
    if (!m_owner.onSend(message, this->shared_from_this())) {
        XRN_WARNING("TCP Send canceled");
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
> void ::xrn::network::Connection<UserEnum>::startReceivingUdpMessage()
{
    XRN_DEBUG("TCP{} Start receiving messages", m_id);
    this->receiveUdpMessage(
        [this](){
            XRN_DEBUG("message received");
            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner(::std::move(m_udpBufferIn));

            // recreate a buffer as it has been set to null by move
            m_udpBufferIn = ::std::make_unique<::xrn::network::Message<UserEnum>>(
                ::xrn::network::Message<UserEnum>::ProtocolType::udp
            );

            // m_udpBufferInLocker.unlock();
            this->startReceivingUdpMessage();
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
    m_owner.pushIncommingMessage(this->shared_from_this(), ::std::move(message));
    m_owner.notifyIncommingMessageQueue();
    message = nullptr;
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
        this->sendTcpMessageHeader(::std::move(message), successCallback);
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
    this->sendTcpMessageHeader(::std::move(message), successCallback);
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
    this->sendTcpMessageHeader(::std::move(message), successCallback);
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpMessageHeader(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the header and call the sending of the body
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

            // if not everything has been sent
            if (bytesAlreadySent + length < message->getHeaderSize()) {
                XRN_DEBUG(
                    "TCP{} -> Header not fully sent ({} / {})"
                    , m_id
                    , bytesAlreadySent + length
                    , message->getHeaderSize()
                );
                return this->sendTcpMessageHeader(
                    ::std::move(message)
                    , successCallback
                    , bytesAlreadySent + length
                );
            }

            // send body if present
            if (message->getBodySize()) {
                XRN_DEBUG("TCP{} -> Header sent", m_id);
                return this->sendTcpMessageBody(
                    ::std::move(message)
                    , successCallback
                );
            }

            XRN_DEBUG("TCP{} -> Body is empty", m_id);
            successCallback();
        }
    };

    XRN_DEBUG(
        "TCP{} -> Request header (size:{})"
        , m_id
        , message->getHeaderSize() - bytesAlreadySent
    );
    m_tcpSocket.async_send(
        ::asio::buffer(
            message->getHeaderAddr() + bytesAlreadySent
            , message->getHeaderSize() - bytesAlreadySent
        )
        , ::std::bind_front(lambda, ::std::move(message))
    );
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpMessageBody(
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
            , const ::std::size_t length [[ maybe_unused ]]
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

            // if not everything has been sent
            if (bytesAlreadySent + length < message->getBodySize()) {
                XRN_DEBUG(
                    "TCP{} -> Body not fully sent ({} / {})"
                    , m_id
                    , bytesAlreadySent + length
                    , message->getBodySize()
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

    m_tcpSocket.async_send(
        ::asio::buffer(
            message->getBodyAddr() + bytesAlreadySent
            , message->getBodySize() - bytesAlreadySent
        )
        , ::std::bind_front(lambda, ::std::move(message))
    );
    message = nullptr;
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
                    XRN_WARNING("TCP Receive canceled");
                } else if (errCode == ::asio::error::eof) {
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{} Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                // m_tcpBufferInLocker.unlock();
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_tcpBufferIn->getHeaderSize()) {
                XRN_DEBUG(
                    "TCP{} <- Header not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_tcpBufferIn->getHeaderSize()
                );
                // m_tcpBufferInLocker.unlock();
                return this->receiveTcpMessageHeader(
                    successCallback, bytesAlreadyReceived + length
                );
            }

            m_tcpBufferIn->updateBodySize();
            if (!m_tcpBufferIn->isBodyEmpty()) {
                XRN_DEBUG("TCP{} <- Header received", m_id);
                // m_tcpBufferInLocker.unlock();
                return this->receiveTcpMessageBody(successCallback);
            }

            XRN_DEBUG("TCP{} <- Body is empty", m_id);
            // send the message to the queue and start to receive messages again
            successCallback();
            // m_tcpBufferInLocker.unlock();
        }
    };

    XRN_DEBUG(
        "TCP{} <- Request header (size:{})"
        , m_id
        , m_tcpBufferIn->getHeaderSize() - bytesAlreadyReceived
    );

    m_tcpSocket.async_receive(
        ::asio::buffer(
            m_tcpBufferIn->getHeaderAddr() + bytesAlreadyReceived
            , m_tcpBufferIn->getHeaderSize() - bytesAlreadyReceived
        )
        , lambda
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

            XRN_DEBUG(
                "TCP{} <- Is body not fully received??? ({} / {})"
                , m_id
                , bytesAlreadyReceived + length
                , m_tcpBufferIn->getBodySize()
            );
            // if not everything has been received
            if (bytesAlreadyReceived + length < m_tcpBufferIn->getBodySize()) {
                XRN_DEBUG(
                    "TCP{} <- Body not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_tcpBufferIn->getBodySize()
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
        "TCP{} <- Request body (size:{})({})"
        , m_id
        , m_tcpBufferIn->getBodySize() - bytesAlreadyReceived
        , bytesAlreadyReceived
    );
    m_tcpSocket.async_receive(
        ::asio::buffer(
            m_tcpBufferIn->getBodyAddr() + bytesAlreadyReceived // move the ptr to the end of the body
            , m_tcpBufferIn->getBodySize() - bytesAlreadyReceived
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
        this->sendUdpMessageHeader(::std::move(message), successCallback);
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
    this->sendUdpMessageHeader(::std::move(message), successCallback);
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
    this->sendUdpMessageHeader(::std::move(message), successCallback);
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendUdpMessageHeader(
    ::std::unique_ptr<::xrn::network::Message<UserEnum>> message
    , ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the header and call the sending of the body
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

            // if not everything has been sent
            if (bytesAlreadySent + length < message->getHeaderSize()) {
                XRN_DEBUG(
                    "UDP{} -> Header not fully sent ({} / {})"
                    , m_id
                    , bytesAlreadySent + length
                    , message->getHeaderSize()
                );
                return this->sendUdpMessageHeader(
                    ::std::move(message)
                    , successCallback
                    , bytesAlreadySent + length
                );
            }

            // send body if present
            if (message->getBodySize()) {
                XRN_DEBUG("UDP{} -> Header sent", m_id);
                return this->sendUdpMessageBody(
                    ::std::move(message)
                    , successCallback
                );
            }

            XRN_DEBUG("UDP{} -> Body is empty", m_id);
            successCallback();
        }
    };

    XRN_DEBUG(
        "UDP{} -> Request header (size:{})"
        , m_id
        , message->getHeaderSize() - bytesAlreadySent
    );
    m_udpSocket.async_send(
        ::asio::buffer(
            message->getHeaderAddr() + bytesAlreadySent
            , message->getHeaderSize() - bytesAlreadySent
        )
        , ::std::bind_front(lambda, ::std::move(message))
    );
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendUdpMessageBody(
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
            , const ::std::size_t length [[ maybe_unused ]]
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

            // if not everything has been sent
            if (bytesAlreadySent + length < message->getBodySize()) {
                XRN_DEBUG(
                    "UDP{} -> Body not fully sent ({} / {})"
                    , m_id
                    , bytesAlreadySent + length
                    , message->getBodySize()
                );
                return this->sendUdpMessageBody(
                    ::std::move(message)
                    , successCallback
                    , bytesAlreadySent + length
                );
            }

            XRN_DEBUG("UDP{} -> Body sent", m_id);
            successCallback();
        }
    };

    m_udpSocket.async_send(
        ::asio::buffer(
            message->getBodyAddr() + bytesAlreadySent
            , message->getBodySize() - bytesAlreadySent
        )
        , ::std::bind_front(lambda, ::std::move(message))
    );
    message = nullptr;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveUdpMessage(
    ::xrn::meta::constraint::doesCallableHasParameters<> auto successCallback
)
{
    // m_udpBufferInLocker.lock();
    this->receiveUdpMessageHeader(successCallback, 0);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveUdpMessageHeader(
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
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{} Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                // m_udpBufferInLocker.unlock();
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_udpBufferIn->getHeaderSize()) {
                XRN_DEBUG(
                    "UDP{} <- Header not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_udpBufferIn->getHeaderSize()
                );
                // m_udpBufferInLocker.unlock();
                return this->receiveUdpMessageHeader(
                    successCallback, bytesAlreadyReceived + length
                );
            }

            m_udpBufferIn->updateBodySize();
            if (!m_udpBufferIn->isBodyEmpty()) {
                XRN_DEBUG("UDP{} <- Header received", m_id);
                // m_udpBufferInLocker.unlock();
                return this->receiveUdpMessageBody(successCallback);
            }

            XRN_DEBUG("UDP{} <- Body is empty", m_id);
            // send the message to the queue and start to receive messages again
            successCallback();
            // m_udpBufferInLocker.unlock();
        }
    };

    XRN_DEBUG(
        "UDP{} <- Request header (size:{})"
        , m_id
        , m_udpBufferIn->getHeaderSize() - bytesAlreadyReceived
    );

    m_udpSocket.async_receive(
        ::asio::buffer(
            m_udpBufferIn->getHeaderAddr() + bytesAlreadyReceived
            , m_udpBufferIn->getHeaderSize() - bytesAlreadyReceived
        )
        , lambda
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveUdpMessageBody(
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

            XRN_DEBUG(
                "UDP{} <- Is body not fully received??? ({} / {})"
                , m_id
                , bytesAlreadyReceived + length
                , m_udpBufferIn->getBodySize()
            );
            // if not everything has been received
            if (bytesAlreadyReceived + length < m_udpBufferIn->getBodySize()) {
                XRN_DEBUG(
                    "UDP{} <- Body not fully received ({} / {})"
                    , m_id
                    , bytesAlreadyReceived + length
                    , m_udpBufferIn->getBodySize()
                );
                // m_udpBufferInLocker.unlock();
                return this->receiveUdpMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            XRN_DEBUG("UDP{} <- Body received", m_id);
            successCallback();
            // m_udpBufferInLocker.unlock();
        }
    };

    XRN_DEBUG(
        "UDP{} <- Request body (size:{})({})"
        , m_id
        , m_udpBufferIn->getBodySize() - bytesAlreadyReceived
        , bytesAlreadyReceived
    );
    m_udpSocket.async_receive(
        ::asio::buffer(
            m_udpBufferIn->getBodyAddr() + bytesAlreadyReceived // move the ptr to the end of the body
            , m_udpBufferIn->getBodySize() - bytesAlreadyReceived
        )
        , lambda
    );
}
