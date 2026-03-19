#include "server.h"
#include <iostream>

namespace trading {

// ── Constructor ───────────────────────────────────────────────────────────────
Server::Server(boost::asio::io_context& io_context, uint16_t port)
    : io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
{
    // Register tradeable instruments
    engine_.add_instrument("BTC-USD");
    engine_.add_instrument("ETH-USD");

    std::cout << "[server] listening on port " << port << "\n";
    do_accept();
}

// ── do_accept ─────────────────────────────────────────────────────────────────
// Async loop: wait for a connection, create a Session, then wait again.
void Server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::cout << "[server] client connected: "
                          << socket.remote_endpoint() << "\n";

                // Create the session, passing a callback for trade broadcasts
                auto session = std::make_shared<Session>(
                    std::move(socket),
                    engine_,
                    [this](const Trade& t) { broadcast_trade(t); }
                );

                sessions_.push_back(session);
                session->start();
            } else {
                std::cout << "[server] accept error: " << ec.message() << "\n";
            }

            // Always queue the next accept — keep listening
            do_accept();
        }
    );
}

// ── broadcast_trade ───────────────────────────────────────────────────────────
// Send a trade notification to every connected client
void Server::broadcast_trade(const Trade& trade) {
    TradeMsg msg;
    msg.event        = "trade";
    msg.trade_id     = trade.trade_id;
    msg.instrument   = trade.instrument;
    msg.price        = trade.price;
    msg.quantity     = trade.quantity;
    msg.buyer_id     = trade.buyer_id;
    msg.seller_id    = trade.seller_id;
    msg.buy_order_id  = trade.buy_order_id;
    msg.sell_order_id = trade.sell_order_id;

    std::string serialized = protocol::serialize(msg);

    // Remove dead sessions and broadcast to live ones
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