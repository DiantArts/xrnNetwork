#include <pch.hpp>
#include <catch2/catch.hpp>

#include <xrn/Network/Message.hpp>

TEST_CASE(" xrnNetwork :: Message::basicTest01")
{
    enum MessageType { start, stop, last };
    ::xrn::network::Message<MessageType> m{ MessageType::start, "hello", "you"s, "are"sv };
    m.push<short, int, float, short>(2, 5, 3.7f, 9);
    auto str1{ m.pull<char*>() };
    auto str2{ m.pull<char*>() };
    auto str3{ m.pull<char*>() };
    auto& s1{ m.pull<short>() };
    auto& i1{ m.pull<int>() };
    auto& f1{ m.pull<float>() };
    auto& s2{ m.pull<short>() };

    REQUIRE(str1 == "hello");
    REQUIRE(str2 == "you");
    REQUIRE(str3 == "are");
    REQUIRE(s1 == 2);
    REQUIRE(i1 == 5);
    REQUIRE(f1 == 3.7f);
    REQUIRE(s2 == 9);
}

TEST_CASE(" xrnNetwork :: Message::basicTest02")
{
    enum MessageType { start, stop, last };
    ::xrn::network::Message<MessageType> m{ MessageType::start, "hello", "you"s, "are"sv, 5, 3.7f };
    auto str1{ m.pull<char*>() };
    auto str2{ m.pull<char*>() };
    auto str3{ m.pull<char*>() };
    auto& i1{ m.pull<int>() };
    auto& f1{ m.pull<float>() };

    REQUIRE(str1 == "hello");
    REQUIRE(str2 == "you");
    REQUIRE(str3 == "are");
    REQUIRE(i1 == 5);
    REQUIRE(f1 == 3.7f);
}

TEST_CASE(" xrnNetwork :: Message::basicTest03")
{
    enum MessageType { start, stop, last };
    ::xrn::network::Message<MessageType> m{ MessageType::start };
    m.push("hello");
    m.push("can");
    m.push("someone");
    m.push("hear");
    m.push("me");
    m.push("I");
    m.push("need");
    m.push("help");
    m.push("please");
    m.push("HELLOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");

    auto str1{ m.pull<char*>() };
    auto str2{ m.pull<::std::string>() };
    auto str3{ m.pull<::std::vector<char>>() };
    auto str4{ m.pull<::std::span<char>>() };
    auto str5{ m.pull<char*>() };
    auto str6{ m.pull<char*>() };
    auto str7{ m.pull<char*>() };
    auto str8{ m.pull<char*>() };
    auto str9{ m.pull<char*>() };
    auto str10{ m.pull<char*>() };

    REQUIRE(str1 == "hello");
    REQUIRE(str2 == "can");
    REQUIRE(str3 == "someone");
    REQUIRE(str4 == "hear");
    REQUIRE(str5 == "me");
    REQUIRE(str6 == "I");
    REQUIRE(str7 == "need");
    REQUIRE(str8 == "help");
    REQUIRE(str9 == "please");
    REQUIRE(str10 == "HELLOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
}
