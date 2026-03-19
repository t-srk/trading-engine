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

private:
    void do_accept();

    net::io_context&                      io_context_;
    tcp::acceptor                         acceptor_;
    MatchingEngine                        engine_;
    std::vector<std::shared_ptr<Session>> sessions_;
};

} // namespace trading