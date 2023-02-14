#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Constructors
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> ::xrn::network::Message<T>::Message(
    Message::ProtocolType protocolType // = Message::ProtocolType::undefined
) noexcept
    : m_header{
        .bodySize = 0
        , .messageType = 0
    }
    , m_index{ 0 }
    , m_protocolType{ protocolType }
{}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> ::xrn::network::Message<T>::Message(
    Message::SystemType messageType
    , Message::ProtocolType protocolType // = Message::ProtocolType::undefined
) noexcept
    : m_header{
        .bodySize = 0
        , .messageType = static_cast<decltype(Message::Header::messageType)>(messageType)
    }
    , m_index{ 0 }
    , m_protocolType{ protocolType }
{}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> ::xrn::network::Message<T>::Message(
    Message::UserType messageType
    , Message::ProtocolType protocolType // = Message::ProtocolType::undefined
) noexcept
    : m_header{
        .bodySize = 0
        , .messageType = static_cast<decltype(Message::Header::messageType)>(messageType)
    }
    , m_index{ 0 }
    , m_protocolType{ protocolType }
{}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> ::xrn::network::Message<T>::Message(
    Message::SystemType messageType
    , Message::ProtocolType protocolType
    , auto&&... args
) noexcept
    : m_header{
        .bodySize = (Message::getSizeOfArg(::std::forward<decltype(args)>(args)) + ...)
        , .messageType = static_cast<decltype(Message::Header::messageType)>(messageType)
    }
    , m_body{ m_header.bodySize }
    , m_index{ 0 }
    , m_protocolType{ protocolType }
{
    this->pushMemory(0, ::std::forward<decltype(args)>(args)...);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> ::xrn::network::Message<T>::Message(
    Message::UserType messageType
    , Message::ProtocolType protocolType
    , auto&&... args
) noexcept
    : m_header{
        .bodySize = (Message::getSizeOfArg(::std::forward<decltype(args)>(args)) + ...)
        , .messageType = static_cast<decltype(Message::Header::messageType)>(messageType)
    }, m_body{ m_header.bodySize }
    , m_index{ 0 }
    , m_protocolType{ protocolType }
{
    this->pushMemory(0, ::std::forward<decltype(args)>(args)...);
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Rule of 5
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> ::xrn::network::Message<T>::~Message()
{
    XRN_DEBUG("Message deleted");
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> inline ::xrn::network::Message<T>::Message(
    const Message& that
) noexcept = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> inline auto ::xrn::network::Message<T>::operator=(
    const Message& that
) noexcept
    -> Message& = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> inline ::xrn::network::Message<T>::Message(
    Message&& that
) noexcept = default;

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> inline auto ::xrn::network::Message<T>::operator=(
    Message&& that
) noexcept
    -> Message& = default;



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Push
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::push(
    auto&&... args
)
{
    const auto size{ (Message::getSizeOfArg(::std::forward<decltype(args)>(args)) + ...) };
    const auto oldSize{ m_body.size() };
    m_body.resize(oldSize + size);
    m_header.bodySize = static_cast<decltype(m_header.bodySize)>(m_body.size());
    pushMemory(oldSize, ::std::forward<decltype(args)>(args)...);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::push(
    ::xrn::meta::constraint::isContiguousContainer auto&&... args
)
{
    const auto size{ (args.size() + ...) };
    const auto oldSize{ m_body.size() };
    m_body.resize(oldSize + size);
    m_header.bodySize = m_body.size();
    pushMemory(oldSize, ::std::forward<decltype(args)>(args)...);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::pushAll(
    auto&&... args
)
{
    (this->push(::std::forward<decltype(args)>(args)), ...);
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Pull
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> template <
    typename RawDataType
> auto ::xrn::network::Message<T>::pull()
    -> ::std::remove_cvref_t<RawDataType>&
{
    using DataType = ::std::remove_cvref_t<RawDataType>;
    auto& ref{ *::std::bit_cast<DataType*>(&m_body[m_index]) };
    m_index += sizeof(ref);
    return ref;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> template <
    ::xrn::meta::constraint::isPointer RawDataType
> requires (
    !::xrn::meta::constraint::sameAs<RawDataType, char>
) auto ::xrn::network::Message<T>::pull()
    -> ::std::span<::std::remove_cvref_t<::std::remove_pointer_t<RawDataType>>>
{
    using DataType = ::std::remove_cvref_t<::std::remove_pointer_t<RawDataType>>;
    Message::SizeType size{ *::std::bit_cast<Message::SizeType*>(&m_body[m_index]) };
    m_index += sizeof(size);
    DataType* ptr{ ::std::bit_cast<DataType*>(&m_body[m_index]) };
    m_index += sizeof(DataType) * size;
    return { ptr, static_cast<::std::size_t>(size) };
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> template <
    ::xrn::meta::constraint::isMemoryStr RawDataType
> auto ::xrn::network::Message<T>::pull()
    -> ::std::string
{
    Message::SizeType size{ *::std::bit_cast<Message::SizeType*>(&m_body[m_index]) };
    m_index += sizeof(size);
    char* ptr{ ::std::bit_cast<char*>(&m_body[m_index]) };
    m_index += sizeof(char) * size;
    return ::std::string{ ptr, static_cast<::std::size_t>(size) };
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Size
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::updateBodySize()
{
    m_body.resize(m_header.bodySize);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::resize(
    ::std::size_t newSize
)
{
    m_body.resize(newSize);
    m_header.bodySize = newSize;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Getters
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getBodyAddr()
    -> ::std::byte*
{
    return static_cast<::std::byte*>(m_body.data());
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getHeader() const
    -> const Message<T>::Header&
{
    return m_header;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getHeader()
    -> Message<T>::Header&
{
    return m_header;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getHeaderAddr()
    -> ::std::byte*
{
    return ::std::bit_cast<::std::byte*>(&m_header);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getBody() const
    -> const ::std::vector<::std::byte>&
{
    return m_body;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getBody()
    -> ::std::vector<::std::byte>&
{
    return m_body;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::isBodyEmpty() const
    -> bool
{
    return m_body.empty();
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getAsString() const
    -> ::std::string
{
    return { ::std::bit_cast<char*>(m_body.data()), m_body.size() };
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getHeaderSize() const
    -> ::std::size_t
{
    return sizeof(Message::Header);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getBodySize() const
    -> ::std::size_t
{
    return m_header.bodySize;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getType() const
    -> T
{
    return static_cast<T>(m_header.messageType);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getTypeAsSystemType() const
    -> Message<T>::SystemType
{
    return static_cast<Message::SystemType>(m_header.messageType);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getTypeAsInt() const
    -> ::std::uint16_t
{
    return m_header.messageType;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> auto ::xrn::network::Message<T>::getProtocolType() const
    -> Message::ProtocolType
{
    return m_protocolType;
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::setProtocolType(
    Message::ProtocolType protocolType
)
{
    m_protocolType = protocolType;
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Helpers size
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> constexpr auto ::xrn::network::Message<T>::getSizeOfArg(
    auto&& arg
) -> decltype(Message::Header::bodySize)
{
    return sizeof(arg);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> constexpr auto ::xrn::network::Message<T>::getSizeOfArg(
    const char *const arg
) -> decltype(Message::Header::bodySize)
{
    return sizeof(Message::SizeType) + (sizeof(char) * ::std::strlen(arg));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> constexpr auto ::xrn::network::Message<T>::getSizeOfArg(
    ::xrn::meta::constraint::isContiguousContainer auto&& arg
) -> decltype(Message::Header::bodySize)
{
    return static_cast<decltype(Message::Header::bodySize)>(sizeof(Message::SizeType)) +
        static_cast<decltype(Message::Header::bodySize)>(sizeof(arg.at(0)) * arg.size());
}



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Helpers push
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::pushMemory(
    ::std::size_t index
    , auto&& arg
    , auto&&... args
)
{
    const auto size{ Message::getSizeOfArg(arg) };
    this->pushSingleMemory(index, ::std::forward<decltype(arg)>(arg));
    if constexpr (sizeof...(args)) {
        pushMemory(index + size, ::std::forward<decltype(args)>(args)...);
    }
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::pushSingleMemory(
    ::std::size_t index
    , auto& arg
)
{
    ::std::memcpy(m_body.data() + index, &arg, sizeof(arg));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::pushSingleMemory(
    ::std::size_t index
    , auto&& arg
)
{
    ::std::memmove(m_body.data() + index, &arg, sizeof(arg));
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::pushSingleMemory(
    ::std::size_t index
    , const char *const arg
)
{
    auto size{ static_cast<Message::SizeType>(::std::strlen(arg)) };
    ::std::memcpy(m_body.data() + index, &size, sizeof(Message::SizeType));
    ::std::memcpy(m_body.data() + index + sizeof(Message::SizeType), arg, size);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::pushSingleMemory(
    ::std::size_t index
    , char *const arg
)
{
    auto size{ static_cast<Message::SizeType>(::std::strlen(arg)) };
    ::std::memcpy(m_body.data() + index, &size, sizeof(Message::SizeType));
    ::std::memmove(m_body.data() + index + sizeof(Message::SizeType), arg, size);
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::pushSingleMemory(
    ::std::size_t index
    , ::xrn::meta::constraint::isContiguousContainer auto& arg
)
{
    auto size{ static_cast<Message::SizeType>(arg.size()) };
    ::std::memmove(m_body.data() + index, &size, sizeof(Message::SizeType));
    ::std::memcpy(m_body.data() + index + sizeof(Message::SizeType), arg.data(), arg.size());
}

///////////////////////////////////////////////////////////////////////////
template <
    ::xrn::network::detail::constraint::isValidEnum T
> void ::xrn::network::Message<T>::pushSingleMemory(
    ::std::size_t index
    , ::xrn::meta::constraint::isContiguousContainer auto&& arg
)
{
    auto size{ static_cast<Message::SizeType>(arg.size()) };
    ::std::memmove(m_body.data() + index, &size, sizeof(Message::SizeType));
    ::std::memmove(m_body.data() + index + sizeof(Message::SizeType), arg.data(), arg.size());
}
