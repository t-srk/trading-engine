#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include "matching_engine.h"
#include "session.h"

namespace trading {

using boost::asio::ip::tcp;

// ── Server ────────────────────────────────────────────────────────────────────
// Listens for incoming TCP connections on a port.
// For each new connection, creates a Session.
// Broadcasts trades to all connected sessions.

class Server {
public:
    Server(boost::asio::io_context& io_context, uint16_t port);

    // Broadcast a trade notification to every connected client
    void broadcast_trade(const Trade& trade);

private:
    void do_accept();

    boost::asio::io_context& io_context_;
    tcp::acceptor            acceptor_;
    MatchingEngine           engine_;

    // All active sessions — one per connected client
    std::vector<std::shared_ptr<Session>> sessions_;
};

} // namespace trading