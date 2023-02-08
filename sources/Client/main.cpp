///////////////////////////////////////////////////////////////////////////
// Precompilled headers
///////////////////////////////////////////////////////////////////////////
#include <pch.hpp>

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/Client/Client.hpp>

///////////////////////////////////////////////////////////////////////////
auto main(
    int argc
    , char** argv
) -> int
{
    enum MessageType { start, stop, last };

    XRN_FATAL_ASSERT(argc == 3, "Usage: client <host> {}", argc);
    ::xrn::network::client::Client<MessageType> client;

    // ::ClientExample client;
    // XRN_FATAL_ASSERT(
        // client.connectToServer(argv[1], ::std::atoi(argv[2]),
        // "Could not connect the following server: {}:{}", argv[1], ::std::atoi(argv[2])
    // );

    // ::std::thread inMessagesThread{
        // [&client](){
            // ::std::size_t i{ 0 };
            // while (client.isConnectedToServer()) {
                // client.blockingPullIncommingMessages();
            // }
        // }
    // };

    // ::std::string str;
    // while (true) {
        // ::std::getline(::std::cin, str);
        // if (!client.isConnected()) {
            // break;
        // } else if (str.size()) {
            // if (!::std::strncmp(str.c_str(), "/", 1)) {
                // switch (*str.substr(1, 2).c_str()) {
                // case 'h': client.commandHelp(); break;
                // case 'q': client.disconnect(); goto ExitWhile;
                // case 'u': client.messageUdpServer(str.substr(3)); break;
                // case 'n': client.rename(str.substr(3)); break;
                // case 'c': client.displayConnectedClients(); break;
                // default: ::std::cerr << "[ERROR:SYSTEM] invalid command.\n";
                // }
            // } else {
                // client.messageTcpServer(str);
            // }
        // }
    // }

    // client.getIncommingMessages().notify();
    // inMessagesThread.join();

    return EXIT_SUCCESS;
}
