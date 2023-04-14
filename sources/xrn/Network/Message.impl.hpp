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
> ::xrn::network::Message<UserEnum>::Message() noexcept = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::Message<UserEnum>::Message(
    Message::SystemType messageType
) noexcept
{
    m_header.messageType = static_cast<decltype(Message::Header::messageType)>(messageType);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::Message<UserEnum>::Message(
    UserEnum messageType
) noexcept
{
    m_header.messageType = static_cast<decltype(Message::Header::messageType)>(messageType);
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
> ::xrn::network::Message<UserEnum>::~Message()
{
    XRN_DEBUG("Message deleted");
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::Message<UserEnum>::Message(
    const Message& that
) noexcept
{
    // manual copy because copy constructor does not work
    ::std::memcpy(
        m_message.data()
        , that.m_message.data()
        , sizeof(*m_message.data()) * m_message.size()
    );
    XRN_DEBUG("Message copy constructed");
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::operator=(
    const Message& that
) noexcept
    -> Message&
{
    // manual copy because copy constructor does not work
    ::std::memcpy(
        m_message.data()
        , that.m_message.data()
        , sizeof(*m_message.data()) * m_message.size()
    );
    XRN_DEBUG("Message copy assigned");
    return *this;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> ::xrn::network::Message<UserEnum>::Message(
    Message&& that
) noexcept
{
    // manual copy because copy constructor does not work
    ::std::memmove(
        m_message.data()
        , that.m_message.data()
        , sizeof(*m_message.data()) * m_message.size()
    );
    XRN_DEBUG("Message move constructed");
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::operator=(
    Message&& that
) noexcept
    -> Message&
{
    // manual copy because copy constructor does not work
    ::std::memmove(
        m_message.data()
        , that.m_message.data()
        , sizeof(*m_message.data()) * m_message.size()
    );
    XRN_DEBUG("Message move assigned");
    return *this;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Body execution
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Message<UserEnum>::push(
    auto& data
)
{
    XRN_ASSERT(
        sizeof(data) + m_header.bodySize < Message::maxSize
        , "Cannot push futher than Message::maxSize ({}) ({} + {} < {})"
        , m_header.bodySize + sizeof(data)
        , sizeof(data)
        , m_header.bodySize
        , Message::maxSize
    );
    ::std::memcpy(m_message.data() + this->getSize(), &data, sizeof(data));
    m_header.bodySize += static_cast<::std::uint16_t>(sizeof(data));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Message<UserEnum>::push(
    auto&& data
)
{
    XRN_ASSERT(
        sizeof(data) + m_header.bodySize < Message::maxSize
        , "Cannot push futher than Message::maxSize ({}) ({} + {} < {})"
        , m_header.bodySize + sizeof(data)
        , sizeof(data)
        , m_header.bodySize
        , Message::maxSize
    );
    ::std::memmove(m_message.data() + this->getSize(), &data, sizeof(data));
    m_header.bodySize += static_cast<::std::uint16_t>(sizeof(data));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Message<UserEnum>::pushCopy(
    auto* ptr
    , ::std::size_t size
)
{
    XRN_ASSERT(
        m_header.bodySize * (size * sizeof(*ptr)) < Message::maxSize
        , "Cannot push futher than Message::maxSize ({}) ({} + ({} * {}) < {})"
        , m_header.bodySize + size * sizeof(*ptr)
        , m_header.bodySize
        , size
        , sizeof(*ptr)
        , Message::maxSize
    );
    ::std::memcpy(m_message.data() + this->getSize(), ptr, size * sizeof(*ptr));
    m_header.bodySize += static_cast<::std::uint16_t>(size * sizeof(*ptr));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Message<UserEnum>::pushCopy(
    auto* ptr
    , Message::SizeType size
)
{
    XRN_ASSERT(
        m_header.bodySize * (size * sizeof(*ptr)) < Message::maxSize
        , "Cannot push futher than Message::maxSize ({}) ({} + ({} * {}) < {})"
        , m_header.bodySize + size * sizeof(*ptr)
        , m_header.bodySize
        , size
        , sizeof(*ptr)
        , Message::maxSize
    );
    ::std::memcpy(m_message.data() + this->getSize(), ptr, size * sizeof(*ptr));
    m_header.bodySize += static_cast<::std::uint16_t>(size * sizeof(*ptr));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Message<UserEnum>::pushMove(
    auto* ptr
    , ::std::size_t size
)
{
    XRN_ASSERT(
        m_header.bodySize * (size * sizeof(*ptr)) < Message::maxSize
        , "Cannot push futher than Message::maxSize ({}) ({} + ({} * {}) < {})"
        , m_header.bodySize + size * sizeof(*ptr)
        , m_header.bodySize
        , size
        , sizeof(*ptr)
        , Message::maxSize
    );
    ::std::memmove(m_message.data() + this->getSize(), ptr, size * sizeof(*ptr));
    m_header.bodySize += static_cast<::std::uint16_t>(size * sizeof(*ptr));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> void ::xrn::network::Message<UserEnum>::pushMove(
    auto* ptr
    , Message::SizeType size
)
{
    XRN_ASSERT(
        m_header.bodySize * (size * sizeof(*ptr)) < Message::maxSize
        , "Cannot push futher than Message::maxSize ({}) ({} + ({} * {}) < {})"
        , m_header.bodySize + size * sizeof(*ptr)
        , m_header.bodySize
        , size
        , sizeof(*ptr)
        , Message::maxSize
    );
    ::std::memmove(m_message.data() + this->getSize(), ptr, size * sizeof(*ptr));
    m_header.bodySize += static_cast<::std::uint16_t>(size * sizeof(*ptr));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> template <
    typename T
> auto ::xrn::network::Message<UserEnum>::pull()
    -> T&
{
    XRN_ASSERT(
        m_pullPointer + sizeof(T) <= this->getSize()
        , "Cannot pull further than bodySize ({}) ({} + {} <= {})"
        , m_pullPointer + sizeof(T) - this->getHeaderSize()
        , m_pullPointer
        , sizeof(T)
        , this->getSize()
    );
    auto& data = *::std::bit_cast<T*>(m_message.data() + m_pullPointer);
    m_pullPointer += sizeof(T);
    return data;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> template <
    typename T
> auto ::xrn::network::Message<UserEnum>::pull(
    ::std::size_t size
) -> T*
{
    XRN_ASSERT(
        m_pullPointer + (size * sizeof(T)) <= this->getSize()
        , "Cannot pull further than bodySize ({}) ({} + ({} * {}) <= {})"
        , m_pullPointer + (size * sizeof(T)) - this->getHeaderSize()
        , m_pullPointer
        , size
        , sizeof(T)
        , this->getSize()
    );
    auto* ptr = ::std::bit_cast<T*>(m_message.data() + m_pullPointer);
    m_pullPointer += (size * sizeof(T));
    return ptr;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> template <
    typename T
> auto ::xrn::network::Message<UserEnum>::pull(
    Message::SizeType size
) -> T*
{
    XRN_ASSERT(
        m_pullPointer + (size * sizeof(T)) <= this->getSize()
        , "Cannot pull further than bodySize ({}) ({} + ({} * {}) <= {})"
        , m_pullPointer + size * sizeof(T) - this->getHeaderSize()
        , m_pullPointer
        , size
        , sizeof(T)
        , this->getSize()
    );
    auto* ptr = ::std::bit_cast<T*>(m_message.data() + m_pullPointer);
    m_pullPointer += size * sizeof(T);
    return ptr;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::resetPullPosition()
{
    m_pullPointer = this->getHeaderSize();
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
> auto ::xrn::network::Message<UserEnum>::getHeader() const
    -> const Message<UserEnum>::Header&
{
    return m_header;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::getAddr()
    -> ::std::byte*
{
    return m_message.data();
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::getSize() const
    -> ::std::size_t
{
    return Message::getHeaderSize() + m_header.bodySize;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> constexpr auto ::xrn::network::Message<UserEnum>::getHeaderSize()
    -> ::std::size_t
{
    return sizeof(Message::Header);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::getBodySize() const
    -> ::std::size_t
{
    return m_header.bodySize;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::getAsString() const
    -> ::std::string
{
    return { ::std::bit_cast<char*>(m_message.data()), m_header.bodySize };
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::getType() const
    -> UserEnum
{
    return static_cast<UserEnum>(m_header.messageType);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::getTypeAsSystemType() const
    -> Message<UserEnum>::SystemType
{
    return static_cast<Message::SystemType>(m_header.messageType);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto ::xrn::network::Message<UserEnum>::getTypeAsInt() const
    -> ::std::uint16_t
{
    return m_header.messageType;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Overload declaration: int
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator<<(::xrn::network::Message<UserEnum>& message, int data)
    -> ::xrn::network::Message<UserEnum>&
{
    message.push(data);
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator>>(::xrn::network::Message<UserEnum>& message, int& data)
    -> ::xrn::network::Message<UserEnum>&
{
    data = message.template pull<int>();
    return message;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Overload declaration: ::xrn::Id
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator<<(::xrn::network::Message<UserEnum>& message, ::xrn::Id data)
    -> ::xrn::network::Message<UserEnum>&
{
    message.push(data);
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator>>(::xrn::network::Message<UserEnum>& message, ::xrn::Id& data)
    -> ::xrn::network::Message<UserEnum>&
{
    data = message.template pull<::xrn::Id>();
    return message;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Overload declaration: float
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator<<(::xrn::network::Message<UserEnum>& message, float data)
    -> ::xrn::network::Message<UserEnum>&
{
    message.push(data);
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator>>(::xrn::network::Message<UserEnum>& message, float& data)
    -> ::xrn::network::Message<UserEnum>&
{
    data = message.template pull<float>();
    return message;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Overload declaration: double
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator<<(::xrn::network::Message<UserEnum>& message, double data)
    -> ::xrn::network::Message<UserEnum>&
{
    message.push(data);
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator>>(::xrn::network::Message<UserEnum>& message, double& data)
    -> ::xrn::network::Message<UserEnum>&
{
    data = message.template pull<double>();
    return message;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Overload declaration: string
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator<<(::xrn::network::Message<UserEnum>& message, const ::std::string& str)
    -> ::xrn::network::Message<UserEnum>&
{
    using SizeType = typename ::xrn::network::Message<UserEnum>::SizeType;
    message.template push<SizeType>(static_cast<SizeType>(str.size()));
    message.pushCopy(str.data(), str.size());
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator<<(::xrn::network::Message<UserEnum>& message, ::std::string&& str)
    -> ::xrn::network::Message<UserEnum>&
{
    using SizeType = typename ::xrn::network::Message<UserEnum>::SizeType;
    message.template push<SizeType>(static_cast<SizeType>(str.size()));
    message.pushMove(str.data(), str.size());
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator<<(::xrn::network::Message<UserEnum>& message, const ::std::string_view& str)
    -> ::xrn::network::Message<UserEnum>&
{
    using SizeType = typename ::xrn::network::Message<UserEnum>::SizeType;
    message.template push<SizeType>(static_cast<SizeType>(str.size()));
    message.pushCopy(str.data(), str.size());
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator<<(::xrn::network::Message<UserEnum>& message, const char* str)
    -> ::xrn::network::Message<UserEnum>&
{
    message << ::std::string{ str };
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator>>(::xrn::network::Message<UserEnum>& message, ::std::string& str)
    -> ::xrn::network::Message<UserEnum>&
{
    using SizeType = typename ::xrn::network::Message<UserEnum>::SizeType;
    str.resize(static_cast<::std::size_t>(message.template pull<SizeType>()));
    str.assign(message.template pull<char>(str.size()), str.size());
    return message;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum UserEnum
> auto operator>>(::xrn::network::Message<UserEnum>& message, ::std::string_view& str)
    -> ::xrn::network::Message<UserEnum>&
{
    using SizeType = typename ::xrn::network::Message<UserEnum>::SizeType;
    auto size{ static_cast<::std::size_t>(message.template pull<SizeType>()) };
    auto* ptr{ message.template pull<char>(size) };
    str = ::std::string_view{ ptr, size };
    return message;
}
