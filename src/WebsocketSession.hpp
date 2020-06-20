#ifndef WEBSOCKET_SESSION_HPP
#define WEBSOCKET_SESSION_HPP

#include "Utils.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/websocket.hpp>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
public:
    using Request = boost::beast::http::request<boost::beast::http::string_body>;
    explicit WebsocketSession(boost::asio::ip::tcp::socket);

    void doAccept(const Request&);

    void onAccept(boost::system::error_code);

    void onTimer(boost::system::error_code);

    // called to indicate activity from the remote peer
    void activity();

    void onPing(boost::system::error_code);

    void onControlCallback(boost::beast::websocket::frame_type, boost::string_view);

    void doRead();

    void onRead(boost::system::error_code, size_t);

    void write(std::string);

    void onWrite(boost::system::error_code, size_t);

private:
    const int defaultTimeout{15};
    using MsgQueue = std::list<std::string>;

    void processProc();

    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::asio::steady_timer timer;
    boost::beast::multi_buffer buffer;
    char pingState = 0;
    int timeout{defaultTimeout};

    MsgQueue queue{};
    std::mutex mutexForQueue{};
    std::thread proc;
    std::condition_variable conditionForQueue{};
};

#endif // WEBSOCKET_SESSION_HPP
