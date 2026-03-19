#include "server.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <boost/beast/core.hpp>
namespace beast = boost::beast;

using json = nlohmann::json;

namespace trading {

Server::Server(net::io_context& io_context, uint16_t port)
    : io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
{
    engine_.add_instrument("BTC-USD");
    engine_.add_instrument("ETH-USD");
    engine_.add_instrument("product_1.0");

    std::cout << "[server] listening on port " << port << "\n";
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::cout << "[server] client connected: "
                          << socket.remote_endpoint() << "\n";

                auto session = std::make_shared<Session>(
                    std::move(socket),
                    io_context_,
                    engine_,
                    *this,
                    [this](const Trade& t) { broadcast_trade(t); }
                );

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    sessions_.push_back(session);
                }
                session->start();
            }
            do_accept();
        }
    );
}

void Server::register_session(const std::string& user_id,
                               std::shared_ptr<Session> session) {
    std::shared_ptr<Session> old_session;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = active_users_.find(user_id);
        if (it != active_users_.end()) {
            old_session = it->second.lock();
        }
        active_users_[user_id] = session;
        std::cout << "[server] login: " << user_id << "\n";
    }
    // Deliver and close the old session OUTSIDE the lock to avoid deadlock
    // (deliver() itself posts to the session's strand, so this is safe).
    if (old_session) {
        old_session->deliver(json{{"event","error"},{"reason","session_replaced"}}.dump() + "\n");
        old_session->force_close();
    }
}

void Server::deregister_session(const std::string& user_id, Session* raw) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_users_.find(user_id);
    if (it == active_users_.end()) return;

    auto sp = it->second.lock();
    // Only erase if this is still the same session (not a newer replacement)
    if (!sp || sp.get() == raw) {
        active_users_.erase(it);
        std::cout << "[server] logout: " << user_id << "\n";
    }
}

void Server::send_book_snapshot(std::shared_ptr<Session> session) {
    std::vector<std::string> msgs;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& instr : engine_.instruments()) {
            const auto& book = engine_.get_book(instr);

            json bids_arr = json::array();
            int count = 0;
            for (const auto& kv : book.get_bids()) {
                if (count >= 20) break;
                uint32_t qty = kv.second.total_quantity();
                if (qty == 0) continue;
                bids_arr.push_back({{"price", kv.first}, {"quantity", qty}});
                ++count;
            }

            json asks_arr = json::array();
            count = 0;
            for (const auto& kv : book.get_asks()) {
                if (count >= 20) break;
                uint32_t qty = kv.second.total_quantity();
                if (qty == 0) continue;
                asks_arr.push_back({{"price", kv.first}, {"quantity", qty}});
                ++count;
            }

            msgs.push_back(json{
                {"event",      "snapshot"},
                {"instrument", instr},
                {"bids",       bids_arr},
                {"asks",       asks_arr}
            }.dump() + "\n");
        }
    }
    for (const auto& m : msgs) session->deliver(m);
}

void Server::broadcast_book_update(const std::string& instrument) {
    std::string serialized;
    std::vector<std::shared_ptr<Session>> live;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!engine_.has_instrument(instrument)) return;

        const auto& book = engine_.get_book(instrument);

        // Bids: sorted highest → lowest, take top 20 non-empty levels
        json bids_arr = json::array();
        int count = 0;
        for (const auto& kv : book.get_bids()) {
            if (count >= 20) break;
            uint32_t qty = kv.second.total_quantity();
            if (qty == 0) continue;
            bids_arr.push_back({{"price", kv.first}, {"quantity", qty}});
            ++count;
        }

        // Asks: sorted lowest → highest, take top 20 non-empty levels
        json asks_arr = json::array();
        count = 0;
        for (const auto& kv : book.get_asks()) {
            if (count >= 20) break;
            uint32_t qty = kv.second.total_quantity();
            if (qty == 0) continue;
            asks_arr.push_back({{"price", kv.first}, {"quantity", qty}});
            ++count;
        }

        json msg = {
            {"event",      "book_update"},
            {"instrument", instrument},
            {"bids",       bids_arr},
            {"asks",       asks_arr}
        };
        serialized = msg.dump() + "\n";

        // Prune dead sessions and snapshot the live ones
        sessions_.erase(
            std::remove_if(sessions_.begin(), sessions_.end(),
                [](const std::shared_ptr<Session>& s) { return !s->socket_alive(); }),
            sessions_.end()
        );
        live = sessions_;
    }
    // Deliver outside the lock — deliver() posts to each session's strand
    for (auto& s : live) s->deliver(serialized);
}

void Server::set_halted(bool h) {
    std::string serialized;
    std::vector<std::shared_ptr<Session>> live;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        halted_ = h;
        serialized = json{{"event","engine_status"},{"halted",halted_}}.dump() + "\n";
        sessions_.erase(
            std::remove_if(sessions_.begin(), sessions_.end(),
                [](const std::shared_ptr<Session>& s){ return !s->socket_alive(); }),
            sessions_.end());
        live = sessions_;
    }
    for (auto& s : live) s->deliver(serialized);
}

void Server::send_all_portfolios(const std::string& admin_user_id) {
    std::string serialized;
    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        json users_arr = json::array();
        for (const auto& [uid, portfolio] : engine_.all_portfolios()) {
            json positions_arr = json::array();
            for (const auto& [instr, pos] : portfolio.positions()) {
                double mark = engine_.last_traded_price(pos.instrument);
                double upnl = pos.qty * (mark - pos.avg_cost);
                positions_arr.push_back({
                    {"instrument",    pos.instrument},
                    {"qty",           pos.qty},
                    {"avg_cost",      pos.avg_cost},
                    {"realized_pnl",  pos.realized_pnl},
                    {"unrealized_pnl", upnl},
                    {"last_price",    mark}
                });
            }
            users_arr.push_back({{"user_id", uid}, {"positions", positions_arr}});
        }

        serialized = json{{"event","all_portfolios"},{"users",users_arr}}.dump() + "\n";

        auto it = active_users_.find(admin_user_id);
        if (it != active_users_.end()) session = it->second.lock();
    }
    if (session) session->deliver(serialized);
}

void Server::broadcast_orders_cleared() {
    std::string serialized = json{{"event","orders_cleared"}}.dump() + "\n";
    std::vector<std::shared_ptr<Session>> live;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(
            std::remove_if(sessions_.begin(), sessions_.end(),
                [](const std::shared_ptr<Session>& s){ return !s->socket_alive(); }),
            sessions_.end());
        live = sessions_;
    }
    for (auto& s : live) s->deliver(serialized);
}

void Server::broadcast_cleared_portfolios() {
    std::string serialized =
        json{{"event","portfolio_update"},{"positions",json::array()}}.dump() + "\n";
    std::vector<std::shared_ptr<Session>> live;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [uid, weak] : active_users_) {
            if (auto s = weak.lock()) live.push_back(s);
        }
    }
    for (auto& s : live) s->deliver(serialized);
}

void Server::broadcast_portfolio_mark_updates(const std::string& instrument) {
    std::vector<std::string> users;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [uid, portfolio] : engine_.all_portfolios()) {
            if (portfolio.positions().count(instrument))
                users.push_back(uid);
        }
    }
    for (const auto& uid : users)
        send_portfolio_update(uid);
}

void Server::send_portfolio_update(const std::string& user_id) {
    std::string serialized;
    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto& portfolio = engine_.get_portfolio(user_id);

        json positions_arr = json::array();
        for (const auto& [instr, pos] : portfolio.positions()) {
            double mark = engine_.last_traded_price(pos.instrument);
            double upnl = pos.qty * (mark - pos.avg_cost);
            positions_arr.push_back({
                {"instrument",    pos.instrument},
                {"qty",           pos.qty},
                {"avg_cost",      pos.avg_cost},
                {"realized_pnl",  pos.realized_pnl},
                {"unrealized_pnl", upnl},
                {"last_price",    mark}
            });
        }

        json msg = {
            {"event",     "portfolio_update"},
            {"positions", positions_arr}
        };
        serialized = msg.dump() + "\n";

        auto it = active_users_.find(user_id);
        if (it != active_users_.end()) {
            session = it->second.lock();
        }
    }
    // Deliver outside the lock — deliver() posts to the session's strand
    if (session) session->deliver(serialized);
}

void Server::broadcast_trade(const Trade& trade) {
    json msg = {
        {"event",         "trade"},
        {"trade_id",      trade.trade_id},
        {"instrument",    trade.instrument},
        {"price",         trade.price},
        {"quantity",      trade.quantity},
        {"buyer_id",      trade.buyer_id},
        {"seller_id",     trade.seller_id},
        {"buy_order_id",  trade.buy_order_id},
        {"sell_order_id", trade.sell_order_id}
    };

    std::string serialized = msg.dump() + "\n";
    std::vector<std::shared_ptr<Session>> live;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(
            std::remove_if(sessions_.begin(), sessions_.end(),
                [](const std::shared_ptr<Session>& s) { return !s->socket_alive(); }),
            sessions_.end()
        );
        live = sessions_;
    }
    for (auto& s : live) s->deliver(serialized);
}

} // namespace trading
