#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <string>
#include <chrono>
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

    // Send a snapshot of every instrument's order book to a single session only.
    // Called on new connection so the client immediately sees the live book
    // without broadcasting stale noise to all existing clients.
    void send_book_snapshot(std::shared_ptr<Session> session);

    // Called by Session after a successful login.
    // If user_id is already active, the old session receives session_replaced and is closed.
    void register_session(const std::string& user_id, std::shared_ptr<Session> session);

    // Called by Session on disconnect to clean up the active_users_ entry.
    void deregister_session(const std::string& user_id, Session* raw);

    // Send a portfolio_update message to one specific user (not a broadcast).
    // Looks up the user's active session and delivers the serialized portfolio.
    void send_portfolio_update(const std::string& user_id);

    // Push a portfolio_update to every active user who holds a position in
    // instrument, reflecting the latest mark price. Called after every trade
    // so all holders see updated unrealized PnL immediately.
    void broadcast_portfolio_mark_updates(const std::string& instrument);

    // ── Admin operations ──────────────────────────────────────────────────────

    // Flip the engine halt flag and broadcast engine_status to all sessions.
    void set_halted(bool h);

    // Read the halt flag — must be called while holding mutex().
    bool is_halted() const { return halted_; }

    // Send all users' portfolios to the requesting admin session only.
    void send_all_portfolios(const std::string& admin_user_id);

    // After clear_all_portfolios(), push an empty portfolio_update to every
    // active session so their UI resets immediately.
    void broadcast_cleared_portfolios();

    // After admin clear_orders, tell every client to wipe their resting order highlights.
    void broadcast_orders_cleared();

    // Broadcast leaderboard (name + total PnL) to all connected sessions.
    void broadcast_leaderboard();

    // Route a tomato throw from from_id to target_id.
    // Enforces a 10-second per-thrower cooldown; delivers tomato_hit to the target.
    void route_tomato(const std::string& from_id,
                      const std::string& target_id,
                      std::shared_ptr<Session> from_session);

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

    // user_id → last time they threw a tomato (for 10s cooldown enforcement)
    std::unordered_map<std::string,
        std::chrono::steady_clock::time_point> tomato_cooldowns_;

    mutable std::mutex mutex_;
    bool               halted_ = false;
};

} // namespace trading
