///////////////////////////////////////////////////////////////////////////
// Precompilled headers
///////////////////////////////////////////////////////////////////////////
#include <thread>
#include <pch.hpp>

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <Example/Client.hpp>
#include <Example/MessageType.hpp>

///////////////////////////////////////////////////////////////////////////
auto main(
    int argc
    , char** argv
) -> int
{
    XRN_FATAL_SASSERT(argc == 3, "Usage: client <host> {}", argc);
    ::example::Client client{
        argv[1],
        static_cast<::std::uint16_t>(::std::atoi(argv[2]))
    };

    ::std::string str;
    ::std::getline(::std::cin, str);
    while (true) {
        ::std::getline(::std::cin, str);
        if (!client.isRunning()) {
            XRN_DEBUG("Client is disconnected");
            break;
        } else if (str.size()) {
            if (!::std::strncmp(str.c_str(), "/", 1)) {
                switch (*str.substr(1, 2).c_str()) {
                case 'q': goto ExitWhile;
                default: ::fmt::print("System: {}\n", "invalid command"); break;
                }
            } else {
                client.messageServer(str);
            }
        }

        // auto message{ ::std::make_unique<::xrn::network::Message<::example::MessageType>>(::example::MessageType::message) };
        // *message << client.getConnectionId() << "yes";
        // XRN_DEBUG("{} -> '{}'", message->template pull<::xrn::Id>(), message->template pull<::std::string>());
        // message->resetPullPosition();
        // client.udpSendToServer(::std::move(message));
        // ::std::this_thread::sleep_for(100ms);
        // client.messageServer("yes");
    }
ExitWhile:

    return EXIT_SUCCESS;
}
