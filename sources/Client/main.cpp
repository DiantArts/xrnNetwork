///////////////////////////////////////////////////////////////////////////
// Precompilled headers
///////////////////////////////////////////////////////////////////////////
#include <thread>
#include <pch.hpp>

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <Example/Client.hpp>
#include <xrn/Network/NewMessage.hpp>
#include <Example/MessageType.hpp>

///////////////////////////////////////////////////////////////////////////
auto main(
    int argc
    , char** argv
) -> int
{

    int a;
    double b;
    ::std::string c, d;
    ::std::string e{ "this is a test" }, f{ "this is not a test anymore"};
    ::xrn::network::NewMessage<::example::MessageType> message;
    // ::fmt::print("Size: {}\n", message.getSize());
    // ::fmt::print("Header size: {}\n", message.getHeaderSize());
    // ::fmt::print("Body size: {}\n", message.getBodySize());

    message << 5 << 3.4 << e << ::std::move(e);
    message >> a >> b >> c >> d;
    ::fmt::print("a: {}\nb: {}\nc: {}\nd: {}\n", a, b, c, d);

    // ::fmt::print("Size: {}\n", message.getSize());
    // ::fmt::print("Header size: {}\n", message.getHeaderSize());
    // ::fmt::print("Body size: {}\n", message.getBodySize());
    return 0;

    XRN_FATAL_SASSERT(argc == 3, "Usage: client <host> {}", argc);
    ::example::Client client{
        argv[1],
        static_cast<::std::uint16_t>(::std::atoi(argv[2]))
    };

    ::std::string str;
    ::std::getline(::std::cin, str);
    while (true) {
        // ::std::getline(::std::cin, str);
        // if (!client.isRunning()) {
            // XRN_DEBUG("Client is disconnected");
            // break;
        // } else if (str.size()) {
            // if (!::std::strncmp(str.c_str(), "/", 1)) {
                // switch (*str.substr(1, 2).c_str()) {
                // case 'q': goto ExitWhile;
                // default: ::fmt::print("System: {}\n", "invalid command"); break;
                // }
            // } else {
                // client.messageServer(str);
            // }
        // }

        // auto message{ ::std::make_unique<::xrn::network::Message<::example::MessageType>>(
            // ::example::MessageType::message, client.getConnectionId(), "yes"
        // ) };
        // XRN_DEBUG("{} -> '{}'", message->template pull<::xrn::Id>(), message->template pull<::std::string>());
        // message->resetPullPosition();
        // client.udpSendToServer(::std::move(message));
        ::std::this_thread::sleep_for(100ms);
        client.messageServer("yes");
    }
// ExitWhile:

    return EXIT_SUCCESS;
}
