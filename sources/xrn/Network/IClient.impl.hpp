#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Rule of 5
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::IClient<UserEnum>::IClient() = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::IClient<UserEnum>::~IClient() = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::IClient<UserEnum>::IClient(
    IClient&& that
) noexcept = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::IClient<UserEnum>::operator=(
    IClient&& that
) noexcept
    -> IClient& = default;



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Events
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::IClient<UserEnum>::onConnect(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> target
) -> bool
{
    XRN_LOG(
        "C{} ({}:{}) Connected"
        , target->getId()
        , target->getAddress()
        , target->getPort()
    );
    return true;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::IClient<UserEnum>::onDisconnect(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> target
)
{
    XRN_LOG(
        "C{} ({}:{}) Disconnected"
        , target->getId()
        , target->getAddress()
        , target->getPort()
    );
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::IClient<UserEnum>::onSend(
    ::xrn::network::Message<UserEnum>& message
    , ::std::shared_ptr<::xrn::network::Connection<UserEnum>> target
) -> bool
{
    XRN_DEBUG("-> C{}: '{}'", target->getId(), message.getAsString());
    return true;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::IClient<UserEnum>::onReceive(
    ::xrn::network::Message<UserEnum>& message
    , ::std::shared_ptr<::xrn::network::Connection<UserEnum>> target
)
{
    XRN_DEBUG("<- C{}: '{}'", target->getId(), message.getAsString());
}
