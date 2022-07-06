// #define NDEBUG
#define PRINT_DEBUG
#include <pch.hpp>
#include <Logger.hpp>

struct K {
    void run()
    {
        // ::xrn::test(false, "lol");
        ::xrn::test(true, "Message");
        ::xrn::test(false, ::xrn::Logger::Level::none, "Message");
        ::xrn::test(false, ::xrn::Logger::Level::note, "Message");
        ::xrn::test(false, ::xrn::Logger::Level::info, "Message");
        ::xrn::test(false, ::xrn::Logger::Level::trace, "Message");
        ::xrn::test(false, ::xrn::Logger::Level::debug, "Message");
        ::xrn::test(false, ::xrn::Logger::Level::warning, "Message");
        ::xrn::test(false, ::xrn::Logger::Level::error, "Message");
    }
};

int main()
{
    K{}.run();
    return 0;
}
