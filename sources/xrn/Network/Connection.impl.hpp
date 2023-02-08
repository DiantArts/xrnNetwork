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
    ::asio::io_context& asioContext
    , const ::std::string& host
    , ::std::uint16_t port
)
    : m_socket{ asioContext }
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
    this->disconect();
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
        , [&](const ::std::error_code& errCode) {
            if (errCode) {
                if (errCode == ::asio::error::operation_aborted) {
                    XRN_ERROR("Connection {}: Operation canceled", m_id);
                } else {
                    XRN_ERROR("Connection {}: Failed to target {}:{}", m_id, host, port);
                }
                this->disconect();
                return;
            }

            m_isSendAllowed = true;
            if (this->hasSendingMessagesAwaiting()) {
                this->sendAwaitingMessages();
            }
        }
    );
    XRN_DEBUG("Connection {}: targeting {}:{}", m_id, host, port);
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::disconect()
{
    if (m_socket.is_open()) {
        m_isSendAllowed = false;
        m_socket.cancel();
        m_socket.close();
        XRN_DEBUG("Connection {}: closed", m_id);
    }
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::isOpen() const
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
    ::asio::post(this->getAsioContext(), ::std::bind_front(
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
}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void xrn::network::Connection<UserEnum>::send(
    ::xrn::network::Message<UserEnum>&& message
)
{
    ::asio::post(this->getAsioContext(), ::std::bind_front(
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
    this->sendMessage([this](){});
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
    this->receiveMessage([this](){});
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
> auto xrn::network::Connection<UserEnum>::getPort() const
    -> ::std::uint16_t
{}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::getAddress() const
    -> ::std::string
{}

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto xrn::network::Connection<UserEnum>::getAsioContext()
    -> ::asio::io_context&
{
    return *m_socket.get_executor().target<::asio::io_context>();
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
            "Connection {}: {}"
            , m_id
            , "Messages are already being sent or the connection cannot send messages"
        );
    }
    m_isSendAllowed = false;
    this->sendMessageHeader(0, successCallback);
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
                    XRN_ERROR("Connection {}: Operation canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("Connection {}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("Connection {}: Send header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been sent
            if (bytesAlreadySent + length < m_messagesOut.front().getHeaderSize()) {
                return this->sendMessageHeader(successCallback, bytesAlreadySent + length);
            }

            // send body if present
            if (!m_messagesOut.front().isBodyEmpty()) {
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
            m_messagesOut.front().getHeaderAddr() + bytesAlreadySent
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
                    XRN_ERROR("Connection {}: Operation canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("Connection {}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("Connection {}: Send header failed: {}", m_id, errCode.message());
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
            m_messagesOut.front().getBodyAddr() + bytesAlreadySent
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
                    XRN_ERROR("Connection {}: Operation canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("Connection {}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("Connection {}: Receive header failed: {}", m_id, errCode.message());
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
            this->transferBufferToInQueue();
            this->startReceivingMessage();
            successCallback();
        }
    };

    m_socket.async_receive(
        ::asio::buffer(
            m_bufferIn.getHeaderAddr() + bytesAlreadyReceived
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
                    XRN_ERROR("Connection {}: Operation canceled", m_id);
                } else if (errCode == ::asio::error::eof) {
                    XRN_ERROR("Connection {}: Connection closed, aborting message sending", m_id);
                    this->disconnect();
                } else {
                    XRN_ERROR("Connection {}: Receive header failed: {}", m_id, errCode.message());
                    this->disconnect();
                }
                return;
            }

            // if not everything has been received
            if (bytesAlreadyReceived + length < m_messagesOut.front().getBodySize()) {
                return this->receiveMessageBody(successCallback, bytesAlreadyReceived + length);
            }

            // send the message to the queue and start to receive messages again
            this->transferBufferToInQueue();
            this->startReceivingMessage();
            successCallback();
        }
    };

    m_socket.async_receive(
        ::asio::buffer(
            m_bufferIn.getBodyAddr() + bytesAlreadyReceived
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
> void ::xrn::network::Connection<UserEnum>::transferBufferToInQueue()
{
    // this->pushIncommingMessage(
        // ::xrn::network::OwnedMessage<UserEnum>{ m_bufferIn, this->shared_from_this() }
    // );
}
