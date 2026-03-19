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
                    engine_,
                    *this,
                    [this](const Trade& t) { broadcast_trade(t); }
                );

                sessions_.push_back(session);
                session->start();
            }
            do_accept();
        }
    );
}

void Server::register_session(const std::string& user_id,
                               std::shared_ptr<Session> session) {
    auto it = active_users_.find(user_id);
    if (it != active_users_.end()) {
        if (auto old = it->second.lock()) {
            // Evict the previous session — deliver notice then close TCP
            old->deliver(json{{"event","error"},{"reason","session_replaced"}}.dump() + "\n");
            old->force_close();
        }
    }
    active_users_[user_id] = session;
    std::cout << "[server] login: " << user_id << "\n";
}

void Server::deregister_session(const std::string& user_id, Session* raw) {
    auto it = active_users_.find(user_id);
    if (it == active_users_.end()) return;

    auto sp = it->second.lock();
    // Only erase if this is still the same session (not a newer replacement)
    if (!sp || sp.get() == raw) {
        active_users_.erase(it);
        std::cout << "[server] logout: " << user_id << "\n";
    }
}

void Server::broadcast_book_update(const std::string& instrument) {
    if (!engine_.has_instrument(instrument)) return;

    const auto& book = engine_.get_book(instrument);

    // Bids: already sorted highest → lowest by std::greater, take top 20
    json bids_arr = json::array();
    int count = 0;
    for (const auto& kv : book.get_bids()) {
        if (count++ >= 20) break;
        bids_arr.push_back({{"price", kv.first}, {"quantity", kv.second.total_quantity()}});
    }

    // Asks: already sorted lowest → highest, take top 20
    json asks_arr = json::array();
    count = 0;
    for (const auto& kv : book.get_asks()) {
        if (count++ >= 20) break;
        asks_arr.push_back({{"price", kv.first}, {"quantity", kv.second.total_quantity()}});
    }

    json msg = {
        {"event",      "book_update"},
        {"instrument", instrument},
        {"bids",       bids_arr},
        {"asks",       asks_arr}
    };
    std::string serialized = msg.dump() + "\n";

    sessions_.erase(
        std::remove_if(sessions_.begin(), sessions_.end(),
            [](const std::shared_ptr<Session>& s) { return !s->socket_alive(); }),
        sessions_.end()
    );

    for (auto& session : sessions_) {
        session->deliver(serialized);
    }
}

void Server::broadcast_trade(const Trade& trade) {
    json msg = {
        {"event",        "trade"},
        {"trade_id",     trade.trade_id},
        {"instrument",   trade.instrument},
        {"price",        trade.price},
        {"quantity",     trade.quantity},
        {"buyer_id",     trade.buyer_id},
        {"seller_id",    trade.seller_id},
        {"buy_order_id",  trade.buy_order_id},
        {"sell_order_id", trade.sell_order_id}
    };

    std::string serialized = msg.dump() + "\n";

    // Clean up dead sessions
    sessions_.erase(
        std::remove_if(sessions_.begin(), sessions_.end(),
            [](const std::shared_ptr<Session>& s) {
                return !s->socket_alive();
            }),
        sessions_.end()
    );

    for (auto& session : sessions_) {
        session->deliver(serialized);
    }
}

} // namespace trading