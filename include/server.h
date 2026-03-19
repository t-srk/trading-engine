#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include "matching_engine.h"
#include "session.h"

namespace net = boost::asio;
using     tcp = net::ip::tcp;

namespace trading {

class Server {
public:
    Server(net::io_context& io_context, uint16_t port);
    void broadcast_trade(const Trade& trade);

    // Serialize the current order book for `instrument` and push it to all
    // live clients. Called after every submit and cancel so every client
    // sees an up-to-date view regardless of who placed the order.
    void broadcast_book_update(const std::string& instrument);

private:
    void do_accept();

    net::io_context&                      io_context_;
    tcp::acceptor                         acceptor_;
    MatchingEngine                        engine_;
    std::vector<std::shared_ptr<Session>> sessions_;
};

} // namespace trading