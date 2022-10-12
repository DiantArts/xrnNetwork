///////////////////////////////////////////////////////////////////////////
// Precompilled headers
///////////////////////////////////////////////////////////////////////////
#include <pch.hpp>

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/Message.hpp>

///////////////////////////////////////////////////////////////////////////
auto main()
    -> int
{
    enum MessageType { start, stop, last };

    {
        ::xrn::network::Message<MessageType> m{ MessageType::start, "hello", "you"s, "are"sv };
        m.push<short, short, short>(2, 5, 3);
        auto str1{ m.pull<char*>() };
        auto str2{ m.pull<char*>() };
        auto str3{ m.pull<char*>() };
        auto& i1{ m.pull<short>() };
        auto& i2{ m.pull<short>() };
        auto& i3{ m.pull<short>() };
        ::fmt::print("{} {} {} {} {} {}\n", str1, str2, str3, i1, i2, i3);
    }

    {
        ::xrn::network::Message<MessageType> m{ MessageType::start, "hello", "you"s, "are"sv };
        ::std::vector<short> vec{ 5, 3 };
        m.push(::std::move(vec));
        auto str1{ m.pull<char*>() };
        auto str2{ m.pull<char*>() };
        auto str3{ m.pull<char*>() };
        auto v1{ m.pull<short*>() };
        ::fmt::print("{} {} {} {} {} {}\n", str1, str2, str3, v1.size(), v1[0], v1[1]);
    }
    return EXIT_SUCCESS;
}
