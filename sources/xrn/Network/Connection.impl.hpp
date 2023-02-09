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
    , m_socket{ m_owner.getAsioContext() }
{
    m_socket.open(::asio::ip::udp::v4());
    m_socket.bind(::asio::ip::udp::endpoint(::asio::ip::udp::v4(), 0));
    this->target(host, port);
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
// async (asio thread) - connection
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::target(
    const ::std::string& host
    , const ::std::uint16_t port
)
{
    m_socket.async_connect(
        ::asio::ip::udp::endpoint{ ::asio::ip::address::from_string(host), port }
        , [=](const ::std::error_code& errCode) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("C{}: Operation canceled", m_id);
                } else {
                    XRN_ERROR("C{}: Failed to target {}:{}", m_id, host, port);
                }
                this->disconnect();
                return;
            }

            XRN_LOG("C{}: targeting {}:{}", m_id, host, port);
            m_isSendAllowed = true;
            if (this->hasSendingMessagesAwaiting()) {
                this->sendAwaitingMessages();
            }
        }
    );
    XRN_DEBUG("C{}: request targeting {}:{}", m_id, host, port);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::disconnect()
{
    if (m_socket.is_open()) {
        m_isSendAllowed = false;
        m_socket.cancel();
        m_socket.close();
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
    return m_socket.is_open();
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// async (asio thread) - outgoing messages
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::send(
    const ::xrn::network::Message<UserEnum>& message
)
{
    ::asio::post(m_owner.getAsioContext(), ::std::bind_front(
        [this](
            ::xrn::network::Message<UserEnum> message
        ) {
            auto needsToStartSending{ !this->hasSendingMessagesAwaiting() };
            m_messagesOut.push_back(::std::move(message));
            if (m_isSendAllowed && needsToStartSending) {
                this->sendAwaitingMessages();
            }
        }
        , message
    ));;
    XRN_DEBUG("C{}: request send", m_id);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::send(
    ::xrn::network::Message<UserEnum>&& message
)
{
    ::asio::post(m_owner.getAsioContext(), ::std::bind_front(
        [this](
            ::xrn::network::Message<UserEnum> message
        ) {
            auto needsToStartSending{ !this->hasSendingMessagesAwaiting() };
            m_messagesOut.push_back(::std::move(message));
            if (m_isSendAllowed && needsToStartSending) {
                this->sendAwaitingMessages();
            }
        }
        , ::std::move(message)
    ));;
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::hasSendingMessagesAwaiting() const
    -> bool
{
    return !m_messagesOut.empty();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::sendAwaitingMessages()
{
    this->sendMessage([this](){ XRN_DEBUG("C{}: message sent", m_id); });
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// async (asio thread) - incomming messages
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::startReceivingMessage()
{
    this->receiveMessage([this](){ XRN_DEBUG("C{}: message sent", m_id); });
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// helpers
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
    return m_socket.local_endpoint().port();
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::getAddress() const
    -> ::std::string
{
    return m_socket.local_endpoint().address().to_string();
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// async (asio thread) - outgoing messages
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendMessage(
    auto successCallback
)
{
    if (!m_isSendAllowed) {
        XRN_ERROR(
            "C{}: {}"
            , m_id
            , "Messages are already being sent or the connection cannot send messages"
        );
    }
    m_isSendAllowed = false;
    this->sendMessageHeader(successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::sendMessageHeader(
    auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the header and call the sending of the body
    auto lambda{
        [this, bytesAlreadySent, successCallback](
            const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("C{}: Operation canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("C{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("C{}: Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been sent
            if (bytesAlreadySent + length < m_messagesOut.front().getHeaderSize()) {
                return this->sendMessageHeader(successCallback, bytesAlreadySent + length);
            }

            // send body if present
            if (m_messagesOut.front().getBodySize()) {
                return this->sendMessageBody(successCallback);
            }

            // clear message since already sent
            m_messagesOut.remove_front();

            // send next message if awaiting
            if (this->hasSendingMessagesAwaiting()) {
                return this->sendAwaitingMessages();
            } else {
                m_isSendAllowed = true;
                successCallback();
            }
        }
    };

    m_socket.async_send(
        ::asio::buffer(
            &m_messagesOut.front().getHeader() + bytesAlreadySent
            , m_messagesOut.front().getHeaderSize() - bytesAlreadySent
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
> void ::xrn::network::Connection<UserEnum>::sendMessageBody(
    auto successCallback
    , ::std::size_t bytesAlreadySent // = 0
)
{
    // lambda called by asio to send the body
    auto lambda{
        [this, bytesAlreadySent, successCallback](
            const ::std::error_code& errCode
            , const ::std::size_t length
        ) {

            // error handling
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("C{}: Operation canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("C{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("C{}: Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been sent
            if (bytesAlreadySent + length < m_messagesOut.front().getBodySize()) {
                return this->sendMessageHeader(successCallback, bytesAlreadySent + length);
            }

            // clear message since already sent
            m_messagesOut.remove_front();

            // send next message if awaiting
            if (this->hasSendingMessagesAwaiting()) {
                return this->sendAwaitingMessages();
            } else {
                m_isSendAllowed = true;
                successCallback();
            }
        }
    };

    m_socket.async_send(
        ::asio::buffer(
            &m_messagesOut.front().getBody() + bytesAlreadySent
            , m_messagesOut.front().getBodySize() - bytesAlreadySent
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
// async (asio thread) - incomming messages
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::receiveMessage(
    auto successCallback
)
{
    this->receiveQueueMessageHeader(0, successCallback);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Connection<UserEnum>::receiveMessageHeader(
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
                    XRN_ERROR("C{}: Operation canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("C{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("C{}: Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_messagesOut.front().getHeaderSize()) {
                return this->receiveQueueMessageHeader(successCallback, bytesAlreadyReceived + length);
            }

            if (!m_bufferIn.isBodyEmpty()) {
                m_bufferIn.updateBodySize();
                return this->receiveMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner();
            this->startReceivingMessage();
            successCallback();
        }
    };

    m_socket.async_receive(
        ::asio::buffer(
            &m_bufferIn.getHeader() + bytesAlreadyReceived
            , m_bufferIn.getHeaderSize() - bytesAlreadyReceived
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
> void ::xrn::network::Connection<UserEnum>::receiveMessageBody(
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
                    XRN_ERROR("C{}: Operation canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("C{}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("C{}: Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_messagesOut.front().getBodySize()) {
                return this->receiveMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            // send the message to the queue and start to receive messages again
            this->transferInMessageToOwner();
            this->startReceivingMessage();
            successCallback();
        }
    };

    m_socket.async_receive(
        ::asio::buffer(
            &m_bufferIn.getAddr() + bytesAlreadyReceived
            , m_bufferIn.getBodySize() - bytesAlreadyReceived
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
> void ::xrn::network::Connection<UserEnum>::transferInMessageToOwner()
{
    m_owner->pushIncommingMessage(this->shared_from_this(), m_bufferIn);
}
