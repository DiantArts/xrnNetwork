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
    const ::std::string& host
    , ::std::uint16_t port
    , ::xrn::network::AClient<UserEnum>& owner
)
    : m_owner{ owner }
    , m_tcpSocket{ m_owner.getAsioContext() }
    , m_tcpBufferIn{ ::xrn::network::Message<UserEnum>::ProtocolType::tcp }
    , m_udpSocket{ m_owner.getAsioContext() }
    , m_udpBufferIn{ ::xrn::network::Message<UserEnum>::ProtocolType::udp }
{}

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
{}



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
    this->disconnect();
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
> inline auto xrn::network::Connection<UserEnum>::operator=(
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
> void xrn::network::Connection<UserEnum>::disconnect()
{
    if (m_udpSocket.is_open()) {
        m_isSendingAllowed = false;
        m_udpSocket.cancel();
        m_udpSocket.close();
        XRN_DEBUG("C{}: closed", m_id);
    }
    m_owner.notifyIncommingMessageQueue();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::isConnected() const
    -> bool
{
    return m_udpSocket.is_open();
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
> void xrn::network::Connection<UserEnum>::connectToClient(
    const ::std::string& host
    , const ::std::uint16_t port
)
{
    if (m_tcpSocket.is_open()) {
        if (m_owner.onConnect(this->shared_from_this())) {
            this->identification();
        }
    } else {
        XRN_THROW("TCP{}: Invalid socket", m_id);
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::connectToServer(
    const ::std::string& host
    , const ::std::uint16_t port
)
{
    XRN_DEBUG("TCP{}: request targeting {}:{}", m_id, host, port);
    m_socket.async_connect(
        ::asio::ip::tcp::endpoint{ ::asio::ip::address::from_string(host), port }
        , [this](
            const ::std::error_code& errCode
        ) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("TCP{}: Connection canceled", m_id);
                } else {
                    XRN_ERROR(
                        "TCP{}: Failed to connect to {}:{}: {}"
                        , m_id
                        , host
                        , port
                        , errCode.message()
                    );
                }
                return;
            }

            XRN_DEBUG("TCP{}: Connection accepted", m_id);
            if (!m_owner.onConnect(this->shared_from_this())) {
                this->disconnect();
            } else {
                this->identification();
            }
        }
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::tcpSend(
    const ::xrn::network::Message<UserEnum>& message
)
{
    ::std::scoped_lock locker{ m_tcpMessagesOut.getMutex() };
    auto needsToStartSending{ !this->hasSendingTcpMessagesAwaiting() };
    m_tcpMessagesOut.push_back(message);
    if (m_isSendingAllowed && needsToStartSending) {
        this->sendAwaitingTcpMessages();
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::tcpSend(
    ::xrn::network::Message<UserEnum>&& message
)
{
    ::std::scoped_lock locker{ m_tcpMessagesOut.getMutex() };
    auto needsToStartSending{ !this->hasSendingTcpMessagesAwaiting() };
    m_tcpMessagesOut.push_back(::std::move(message));
    if (m_isSendingAllowed && needsToStartSending) {
        this->sendAwaitingTcpMessages();
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::hasSendingTcpMessagesAwaiting() const
    -> bool
{
    return !m_tcpMessagesOut.empty();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::sendAwaitingTcpMessages()
{
    this->sendTcpQueueMessage([this](){ XRN_DEBUG("TCP{}: message sent", m_id); });
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::startReceivingTcpMessage()
{
    this->receiveTcpMessage([this](){ XRN_DEBUG("TCP{}: message sent", m_id); });
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// UDP
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
template <
    ::detail::constraint::isEnum UserMessageType
> void xrn::network::Connection<UserEnum>::setupUdp(
{
    // send udp informations
    this->sendTcpMessage([this](){  });
    this->sendMessage<[](...){}>(::network::Message<UserMessageType>{
        ::network::Message<UserMessageType>::SystemType::udpInformations,
        m_connection->udp.getPort()
    });

    // receive
    this->receiveMessage<[](::std::shared_ptr<::network::Connection<UserMessageType>> connection){
        if (
            connection->tcp.m_bufferIn.getTypeAsSystemType() !=
            ::network::Message<UserMessageType>::SystemType::udpInformations
        ) {
            ::std::cerr << "[ERROR:TCP:" << connection->getId() << "] Failed to setup Udp, "
                << "unexpected message received: " << connection->tcp.m_bufferIn.getTypeAsInt() << ".\n";
            connection->disconnect();
        } else {
            connection->udp.target(
                connection->tcp.getAddress(),
                connection->tcp.m_bufferIn.template pull<::std::uint16_t>()
            );
            connection->tcp.m_isSendingAllowed = true;
            connection->m_owner.onConnectionValidated(connection);
            connection->tcp.m_blocker.notify_all();
        }
    }>();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::udpTarget(
    const ::std::string& host
    , const ::std::uint16_t port
)
{
    m_udpSocket.async_connect(
        ::asio::ip::udp::endpoint{ ::asio::ip::address::from_string(host), port }
        , [=](const ::std::error_code& errCode) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("UDP{}: Target canceled", m_id);
                } else {
                    XRN_ERROR("UDP{}: Failed to target {}:{}", m_id, host, port);
                }
                this->disconnect();
                return;
            }

            XRN_LOG("UDP{}: targeting {}:{}", m_id, host, port);
            m_isSendingAllowed = true;
            auto locker{ m_udpMessagesOut.lock() };
            this->sendAwaitingUdpMessages();
        }
    );
    XRN_DEBUG("UDP{}: request targeting {}:{}", m_id, host, port);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::udpSend(
    const ::xrn::network::Message<UserEnum>& message
)
{
    m_udpMessagesOut.push_back(message);
    auto locker{ m_udpMessagesOut.lock() };
    if (m_udpMessagesOut.lockFreeCount() == 1) {
        this->sendAwaitingUdpMessages();
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::udpSend(
    ::xrn::network::Message<UserEnum>&& message
)
{
    m_udpMessagesOut.push_back(::std::move(message));
    auto locker{ m_udpMessagesOut.lock() };
    if (m_udpMessagesOut.lockFreeCount() == 1) {
        this->sendAwaitingUdpMessages();
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::hasSendingUdpMessagesAwaiting() const
    -> bool
{
    return !m_udpMessagesOut.empty();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::sendAwaitingUdpMessages()
{
    if (m_isSendingAllowed) {
        ++m_numberOfSendingInstances;
        this->sendUdpQueueMessage([this](){ --m_numberOfSendingInstances; });
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::startReceivingUdpMessage()
{
    this->receiveUdpMessage([this](){ XRN_DEBUG("UDP{}: message sent", m_id); });
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
> auto xrn::network::Connection<UserEnum>::getId() const
    -> ::xrn::Id
{
    return m_id;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::getPort() const
    -> ::std::uint16_t
{
    return m_tcpSocket.local_endpoint().port();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::getAddress() const
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
    m_owner->pushIncommingMessage(this->shared_from_this(), buffer);
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
    auto successCallback
)
{
    if (!m_isSendingAllowed) {
        XRN_ERROR(
            "TCP{}: {}"
            , m_id
            , "Messages are already being sent or the connection cannot send messages"
        );
        return;
    }
    this->sendTcpMessageHeader(m_tcpMessagesOut.pop_front(), successCallback)
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendTcpQueueMessage(
    auto successCallback
)
{
    this->sendTcpMessageHeader(m_tcpMessagesOut.pop_front(), successCallback)
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpMessage(
    const ::xrn::network::Message<UserEnum>& message
    , auto successCallback
)
{
    if (!m_isSendingAllowed) {
        XRN_ERROR(
            "TCP{}: {}"
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
> void ::xrn::network::Connection<UserEnum>::forceSendTcpMessage(
    const ::xrn::network::Message<UserEnum>& message
    , auto successCallback
)
{
    this->sendTcpMessageHeader(::std::move(message), successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpMessageHeader(
    ::xrn::network::Message<UserEnum>&& message
    , auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the header and call the sending of the body
    auto lambda{
        [this, message = ::std::move(message), bytesAlreadySent, successCallback](
            const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("TCP{}: Send canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("TCP{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{}: Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been sent
            if (bytesAlreadySent + length < message.front().getHeaderSize()) {
                return this->sendTcpMessageHeader(
                    ::std::move(message)
                    , successCallbac
                    , bytesAlreadySent + length
                );
            }

            // send body if present
            if (message.front().getBodySize()) {
                return this->sendTcpMessageBody(
                    ::std::move(message)
                    , successCallback
                );
            }

            successCallback();
        }
    };

    m_tcpSocket.async_send(
        ::asio::buffer(
            &message.getHeader() + bytesAlreadySent
            , message.getHeaderSize() - bytesAlreadySent
        )
        , ::std::bind(
            lambda
            , ::std::placeholders::_1
            , ::std::placeholders::_2
        )
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendTcpMessageBody(
    ::xrn::network::Message<UserEnum>&& message
    , auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the body
    auto lambda{
        [this, message = ::std::move(message), bytesAlreadySent, successCallback](
            const ::std::error_code& errCode
            , const ::std::size_t length
        ) {
            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("TCP{}: Send canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("TCP{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{}: Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            successCallback();
        }
    };

    m_tcpSocket.async_send(
        ::asio::buffer(
            &message.getBody() + bytesAlreadySent
            , message.getBodySize() - bytesAlreadySent
        )
        , ::std::bind(
            lambda
            , ::std::placeholders::_1
            , ::std::placeholders::_2
        )
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::receiveTcpMessage(
    auto successCallback
)
{
    this->receiveTcpMessageHeader(0, successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveTcpMessageHeader(
    auto successCallback
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
                    XRN_ERROR("TCP{}: Receive canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("TCP{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{}: Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_tcpMessagesOut.front().getHeaderSize()) {
                return this->receiveTcpMessageHeader(successCallback, bytesAlreadyReceived + length);
            }

            if (!m_tcpBufferIn.isBodyEmpty()) {
                m_tcpBufferIn.updateBodySize();
                return this->receiveTcpMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner(m_tcpBufferIn);
            this->startReceivingTcpMessage();
            successCallback();
        }
    };

    m_tcpSocket.async_receive(
        ::asio::buffer(
            &m_tcpBufferIn.getHeader() + bytesAlreadyReceived
            , m_tcpBufferIn.getHeaderSize() - bytesAlreadyReceived
        )
        , ::std::bind(
            lambda
            , ::std::placeholders::_1
            , ::std::placeholders::_2
        )
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveTcpMessageBody(
    auto successCallback
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
                    XRN_ERROR("TCP{}: Receive canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("TCP{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("TCP{}: Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_tcpMessagesOut.front().getBodySize()) {
                return this->receiveTcpMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner(m_tcpBufferIn);
            this->startReceivingTcpMessage();
            successCallback();
        }
    };

    m_tcpSocket.async_receive(
        ::asio::buffer(
            &m_tcpBufferIn.getAddr() + bytesAlreadyReceived
            , m_tcpBufferIn.getBodySize() - bytesAlreadyReceived
        )
        , ::std::bind(
            lambda
            , ::std::placeholders::_1
            , ::std::placeholders::_2
        )
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
    auto successCallback
)
{
    if (!m_isSendingAllowed) {
        XRN_ERROR(
            "UDP{}: {}"
            , m_id
            , "Messages are already being sent or the connection cannot send messages"
        );
        return;
    }
    this->sendUdpMessageHeader(m_udpMessagesOut.pop_front(), successCallback)
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendUdpQueueMessage(
    auto successCallback
)
{
    this->sendUdpMessageHeader(m_udpMessagesOut.pop_front(), successCallback)
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendUdpMessage(
    const ::xrn::network::Message<UserEnum>& message
    , auto successCallback
)
{
    if (!m_isSendingAllowed) {
        XRN_ERROR(
            "UDP{}: {}"
            , m_id
            , "Messages are already being sent or the connection cannot send messages"
        );
        return;
    }
    this->sendUdpMessageHeader(::std::move(message), successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::forceSendUdpMessage(
    const ::xrn::network::Message<UserEnum>& message
    , auto successCallback
)
{
    this->sendUdpMessageHeader(::std::move(message), successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendUdpMessageHeader(
    ::xrn::network::Message<UserEnum>&& message
    , auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the header and call the sending of the body
    auto lambda{
        [this, message = ::std::move(message), bytesAlreadySent, successCallback](
            const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("UDP{}: Send canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("UDP{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{}: Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been sent
            if (bytesAlreadySent + length < message.front().getHeaderSize()) {
                return this->sendUdpMessageHeader(
                    ::std::move(message)
                    , successCallbac
                    , bytesAlreadySent + length
                );
            }

            // send body if present
            if (message.front().getBodySize()) {
                return this->sendUdpMessageBody(
                    ::std::move(message)
                    , successCallback
                );
            }

            successCallback();
        }
    };

    m_udpSocket.async_send(
        ::asio::buffer(
            &message.getHeader() + bytesAlreadySent
            , message.getHeaderSize() - bytesAlreadySent
        )
        , ::std::bind(
            lambda
            , ::std::placeholders::_1
            , ::std::placeholders::_2
        )
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendUdpMessageBody(
    ::xrn::network::Message<UserEnum>&& message
    , auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the body
    auto lambda{
        [this, message = ::std::move(message), bytesAlreadySent, successCallback](
            const ::std::error_code& errCode
            , const ::std::size_t length
        ) {
            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("UDP{}: Send canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("UDP{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{}: Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            successCallback();
        }
    };

    m_udpSocket.async_send(
        ::asio::buffer(
            &message.getBody() + bytesAlreadySent
            , message.getBodySize() - bytesAlreadySent
        )
        , ::std::bind(
            lambda
            , ::std::placeholders::_1
            , ::std::placeholders::_2
        )
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::receiveUdpMessage(
    auto successCallback
)
{
    this->receiveUdpMessageHeader(0, successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveUdpMessageHeader(
    auto successCallback
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
                    XRN_ERROR("UDP{}: Receive canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("UDP{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{}: Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_udpMessagesOut.front().getHeaderSize()) {
                return this->receiveUdpMessageHeader(successCallback, bytesAlreadyReceived + length);
            }

            if (!m_udpBufferIn.isBodyEmpty()) {
                m_udpBufferIn.updateBodySize();
                return this->receiveUdpMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner(m_udpBufferIn);
            this->startReceivingUdpMessage();
            successCallback();
        }
    };

    m_udpSocket.async_receive(
        ::asio::buffer(
            &m_udpBufferIn.getHeader() + bytesAlreadyReceived
            , m_udpBufferIn.getHeaderSize() - bytesAlreadyReceived
        )
        , ::std::bind(
            lambda
            , ::std::placeholders::_1
            , ::std::placeholders::_2
        )
    );
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveUdpMessageBody(
    auto successCallback
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
                    XRN_ERROR("UDP{}: Receive canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("UDP{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("UDP{}: Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_udpMessagesOut.front().getBodySize()) {
                return this->receiveUdpMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner(m_udpBufferIn);
            this->startReceivingUdpMessage();
            successCallback();
        }
    };

    m_udpSocket.async_receive(
        ::asio::buffer(
            &m_udpBufferIn.getAddr() + bytesAlreadyReceived
            , m_udpBufferIn.getBodySize() - bytesAlreadyReceived
        )
        , ::std::bind(
            lambda
            , ::std::placeholders::_1
            , ::std::placeholders::_2
        )
    );
}
