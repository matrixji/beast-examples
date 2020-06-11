#ifndef WEBSOCKET_SESSION_HPP
#define WEBSOCKET_SESSION_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <memory>

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
public:
    explicit WebSocketSession(boost::asio::ip::tcp::socket);

private:
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::asio::steady_timer timer;
    boost::beast::multi_buffer buffer;
    char pingState = 0;
};

#endif // WEBSOCKET_SESSION_HPP
