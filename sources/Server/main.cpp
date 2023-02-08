///////////////////////////////////////////////////////////////////////////
// Precompilled headers
///////////////////////////////////////////////////////////////////////////
#include <pch.hpp>

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#include <xrn/Network/Message.hpp>
#include <xrn/Network/IClient.hpp>
#include <xrn/Network/Connection.hpp>

///////////////////////////////////////////////////////////////////////////
auto main(
    int argc
    , char** argv
) -> int
{
    XRN_FATAL_ASSERT(argc == 2, "Usage: client <host>");
    return EXIT_SUCCESS;

    // ::ServerExample server{ static_cast<::std::uint16_t>(::std::atoi(argv[1])) };
    // if (!server.start()) {
        // return EXIT_FAILURE;
    // }

    // ::std::thread thread{
        // [&server](){
            // ::std::string str;
            // while (server.isRunning()) {
                // ::std::getline(::std::cin, str);
                // if (str == "/q") {
                    // server.stop();
                // }
            // }
        // }
    // };
    // while (server.isRunning()) {
        // server.blockingPullIncommingMessages();
    // }

    // thread.join();

}
