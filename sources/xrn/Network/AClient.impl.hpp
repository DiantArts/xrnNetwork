#pragma once

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
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::AClient<UserEnum>::AClient()
    : m_idleWork{ m_asioContext }
    , m_asioContextThread{
        // Create the asio thread and its context that will hold every network actions
        [this](){
            m_asioContext.run();
        }
    } , m_inMessagesThread{
        [this](){
            do { // wait that the connection is setup then check if is Running
                this->blockingPullIncommingMessages();
            } while (this->isRunning());
        }
    }
{}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Rule of 5
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::AClient<UserEnum>::~AClient()
{
    m_asioContext.stop();
    if (m_asioContextThread.joinable()) {
        m_asioContextThread.join();
    }
    if (m_inMessagesThread.joinable()) {
        m_inMessagesThread.join();
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::AClient<UserEnum>::AClient(
    AClient&& that
) noexcept = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::AClient<UserEnum>::operator=(
    AClient&& that
) noexcept
    -> AClient& = default;



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Incomming messages
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::AClient<UserEnum>::pullIncommingMessage()
{
    auto message{ m_messagesIn.pop_front() };
    if (!this->handleIncommingSystemMessages(message.getOwner(), message)) {
        this->onReceive(message, message.getOwner());
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::AClient<UserEnum>::pullIncommingMessages()
{
    while (!m_messagesIn.empty()) {
        this->pullIncommingMessage();
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::AClient<UserEnum>::blockingPullIncommingMessages()
{
    m_messagesIn.wait();
    this->pullIncommingMessages();
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::AClient<UserEnum>::notifyIncommingMessageQueue()
{
    m_messagesIn.notify();
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::AClient<UserEnum>::pushIncommingMessage(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> connection
    , ::xrn::network::Message<UserEnum> message
)
{
    m_messagesIn.emplace_back(connection, ::std::move(message));
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Getters
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::AClient<UserEnum>::getAsioContext() const
    -> const ::asio::io_context&
{
    return m_asioContext;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::AClient<UserEnum>::getAsioContext()
    -> ::asio::io_context&
{
    return m_asioContext;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::AClient<UserEnum>::getThreadContext()
    -> ::std::thread&
{
    return m_asioContextThread;
}
