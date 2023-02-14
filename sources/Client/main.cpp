///////////////////////////////////////////////////////////////////////////
// Precompilled headers
///////////////////////////////////////////////////////////////////////////
#include <pch.hpp>

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <Example/Client.hpp>

auto getPort()
    -> ::std::uint16_t
{
    return 6;
}

///////////////////////////////////////////////////////////////////////////
auto main(
    int argc
    , char** argv
) -> int
{
    // ::xrn::network::Message<::example::MessageType> message{
        // ::example::MessageType::message
        // , ::xrn::network::Message<::example::MessageType>::ProtocolType::tcp
        // , ::getPort()
    // };
    // message.push(::getPort());
    // auto port{ message.template pull<::std::uint16_t>() };
    // ::fmt::print("{}\n", port);
    // return 0;
    XRN_FATAL_SASSERT(argc == 3, "Usage: client <host> {}", argc);
    ::example::Client client{
        argv[1],
        static_cast<::std::uint16_t>(::std::atoi(argv[2]))
    };

    ::std::string str;
    while (true) {
        ::std::getline(::std::cin, str);
        if (!client.isRunning()) {
            XRN_DEBUG("Client is disconnected");
            break;
        } else if (str.size()) {
            if (!::std::strncmp(str.c_str(), "/", 1)) {
                switch (*str.substr(1, 2).c_str()) {
                case 'h': client.commandHelp(); break;
                case 'q': goto ExitWhile;
                default: ::fmt::print("System: {}\n", "invalid command"); break;
                }
            } else {
                client.messageServer(str);
            }
        }
    }
ExitWhile:

    client.disconnect();

    return EXIT_SUCCESS;
}
