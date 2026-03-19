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
                    [this](const Trade& t) { broadcast_trade(t); }
                );

                sessions_.push_back(session);
                session->start();
            }
            do_accept();
        }
    );
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