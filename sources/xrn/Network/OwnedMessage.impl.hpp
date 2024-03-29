#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Constructors
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::OwnedMessage<UserEnum>::OwnedMessage(
    ::std::shared_ptr<::xrn::network::Connection<UserEnum>> owner
    , ::std::unique_ptr<::xrn::network::Message<UserEnum>>&& message
) noexcept
    : m_message{ ::std::move(message) }
    , m_owner{ ::std::move(owner) }
{}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Getters
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::OwnedMessage<UserEnum>::getOwner()
    -> ::std::shared_ptr<::xrn::network::Connection<UserEnum>>
{
    return m_owner;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::OwnedMessage<UserEnum>::getMessage()
    -> ::std::unique_ptr<::xrn::network::Message<UserEnum>>&
{
    return m_message;
}
