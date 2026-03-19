#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include "server.h"

int main() {
    try {
        boost::asio::io_context io_context;

        // Keep io_context::run() alive even when there is momentarily no work
        auto work = boost::asio::make_work_guard(io_context);

        trading::Server server(io_context, 9000);

        // Graceful shutdown on Ctrl+C / SIGTERM
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](boost::system::error_code /*ec*/, int sig) {
            std::cout << "\n[main] signal " << sig << " — shutting down\n";
            work.reset();        // allow run() to drain and return
            io_context.stop();   // cancel all pending async ops
        });

        // Spin up 2–4 threads (never fewer than 2, never more than 4)
        const unsigned n = std::clamp(std::thread::hardware_concurrency(), 2u, 4u);
        std::vector<std::thread> pool;
        pool.reserve(n);
        for (unsigned i = 0; i < n; ++i)
            pool.emplace_back([&io_context] { io_context.run(); });

        std::cout << "[main] server running on " << n << " threads. Ctrl+C to stop.\n";

        for (auto& t : pool) t.join();

    } catch (const std::exception& e) {
        std::cerr << "[main] fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
