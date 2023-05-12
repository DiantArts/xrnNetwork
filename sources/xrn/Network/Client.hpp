#pragma once

///////////////////////////////////////////////////////////////////////////
// Headers
///////////////////////////////////////////////////////////////////////////
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wnull-dereference"
#elif __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#ifdef __clang__
    #pragma clang diagnostic pop
#elif __GNUC__
    #pragma GCC diagnostic pop
#endif

#include <xrn/Network/Client/Client.hpp>



///////////////////////////////////////////////////////////////////////////
/// \defgroup networkClient Network client module
///
/// Module to communicate to an xrnNetwork server
///
///////////////////////////////////////////////////////////////////////////
