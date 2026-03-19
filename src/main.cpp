#include <iostream>
#include <boost/asio.hpp>
#include "server.h"

int main() {
    try {
        boost::asio::io_context io_context;
        trading::Server server(io_context, 9000);

        std::cout << "[main] server running. Press Ctrl+C to stop.\n";
        io_context.run();   // blocks here, running the event loop

    } catch (const std::exception& e) {
        std::cerr << "[main] fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}