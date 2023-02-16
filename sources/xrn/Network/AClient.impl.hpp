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
> ::xrn::network::AClient<UserEnum>::AClient(
    bool isServer // = false
)
    : m_isServer{ isServer }
    , m_idleWork{ m_asioContext }
    , m_asioContextThread{
        // Create the asio thread and its context that will hold every network actions
        [this](){
            m_asioContext.run();
            if (this->isRunning()) {
                XRN_ERROR("Asio context stopped running while still running");
            }
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
    XRN_DEBUG("Destroying...");
    m_asioContext.stop();
    if (m_asioContextThread.joinable()) {
        XRN_DEBUG("Joining asio thread...");
        m_asioContextThread.join();
        XRN_DEBUG("...Asio thread joined");
    }
    if (m_inMessagesThread.joinable()) {
        XRN_DEBUG("Joining messages thread...");
        m_inMessagesThread.join();
        XRN_DEBUG("...Messages thread joined");
    }
    XRN_DEBUG("...Destroyed");
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
    if (this->handleIncommingSystemMessages(message->getOwner(), *message->getMessage())) {
        return;
    }
    this->onReceive(*message->getMessage(), message->getOwner());
    message->getMessage()->resetPointer();
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
    , ::std::unique_ptr<::xrn::network::Message<UserEnum>>&& message
)
{
    m_messagesIn.push_back(::std::make_unique<::xrn::network::OwnedMessage<UserEnum>>(
        connection, ::std::move(message)
    ));
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
> auto ::xrn::network::AClient<UserEnum>::isServer() const
    -> bool
{
    return m_isServer;
}

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

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::AClient<UserEnum>::isRunning() const
    -> bool
{
    return m_isRunning;
}
