#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <string>
#include "matching_engine.h"
#include "session.h"

namespace net = boost::asio;
using     tcp = net::ip::tcp;

namespace trading {

class Server {
public:
    Server(net::io_context& io_context, uint16_t port);
    void broadcast_trade(const Trade& trade);
    void broadcast_book_update(const std::string& instrument);

    // Called by Session after a successful login.
    // If user_id is already active, the old session receives session_replaced and is closed.
    void register_session(const std::string& user_id, std::shared_ptr<Session> session);

    // Called by Session on disconnect to clean up the active_users_ entry.
    void deregister_session(const std::string& user_id, Session* raw);

    // Send a portfolio_update message to one specific user (not a broadcast).
    // Looks up the user's active session and delivers the serialized portfolio.
    void send_portfolio_update(const std::string& user_id);

    // Shared mutex protecting engine_, sessions_, and active_users_.
    // Sessions lock this before touching the matching engine.
    std::mutex& mutex() { return mutex_; }

private:
    void do_accept();

    net::io_context&                      io_context_;
    tcp::acceptor                         acceptor_;
    MatchingEngine                        engine_;
    std::vector<std::shared_ptr<Session>> sessions_;

    // user_id → their current authenticated session (weak to avoid ownership cycles)
    std::unordered_map<std::string, std::weak_ptr<Session>> active_users_;

    mutable std::mutex mutex_;
};

} // namespace trading
