#include "WebsocketSession.hpp"
#include "WebsocketHandler.hpp"
#include <boost/asio/bind_executor.hpp>
#include <spdlog/spdlog-inl.h>

using boost::asio::ip::tcp;
using boost::beast::websocket::frame_type;
using boost::system::error_code;
using Request = boost::beast::http::request<boost::beast::http::string_body>;

WebsocketSession::WebsocketSession(boost::asio::ip::tcp::socket socket, WebsocketHandler& handler)
: handler(handler)
, ws{std::move(socket)}
, strand{ws.get_executor()}
, timer{ws.get_executor().context(), std::chrono::steady_clock::time_point::max()}
{
}

void WebsocketSession::doAccept(const Request& req)
{
    ws.control_callback([this](frame_type kind, boost::string_view payload) {
        onControlCallback(kind, payload);
    });

    onTimer({});

    timer.expires_after(std::chrono::seconds{timeout});
    auto self = shared_from_this();

    // accept
    ws.async_accept(req, bind_executor(strand, [self](error_code error) {
                        self->onAccept(error);
                    }));
}

void WebsocketSession::onAccept(error_code error)
{
    if(error == boost::asio::error::operation_aborted)
    {
        return;
    }
    else if(error)
    {
        return utils::handleSystemError(error, "ws accept");
    }

    auto self = shared_from_this();
    // start a detach proc for processing incoming request.
    proc = std::thread([self]() { self->processProc(); });
    proc.detach();

    doRead();
}

void WebsocketSession::onTimer(error_code error)
{
    if(error and error != boost::asio::error::operation_aborted)
    {
        return utils::handleSystemError(error, "timer");
    }

    if(timer.expiry() <= std::chrono::steady_clock::now())
    {
        if(ws.is_open() and pingState == 0)
        {
            pingState = 1;
            auto self = shared_from_this();
            timer.expires_after(std::chrono::seconds{timeout});
            ws.async_ping({}, bind_executor(strand, [self](error_code error) {
                              self->onPing(error);
                          }));
        }
        else
        {
            ws.next_layer().shutdown(tcp::socket::shutdown_both, error);
            ws.next_layer().close(error);
            return;
        }
    }

    // wait timer
    auto self = shared_from_this();
    timer.async_wait(
        bind_executor(strand, [self](error_code error) { self->onTimer(error); }));
}

void WebsocketSession::activity()
{
    pingState = 0;
    timer.expires_after(std::chrono::seconds{timeout});
}

void WebsocketSession::onPing(error_code error)
{
    if(error == boost::asio::error::operation_aborted)
    {
        return;
    }
    if(error)
    {
        return utils::handleSystemError(error, "ws ping");
    }

    if(pingState == 1)
    {
        pingState = 2;
    }
}

void WebsocketSession::onControlCallback(frame_type, boost::string_view)
{
    // nothing but activity
    activity();
}

void WebsocketSession::doRead()
{
    auto self = shared_from_this();
    auto handler = bind_executor(strand, [self](error_code error, size_t size) {
        self->onRead(error, size);
    });
    ws.async_read(buffer, handler);
}

void WebsocketSession::onRead(error_code error, size_t)
{
    if(error == boost::asio::error::operation_aborted)
    {
        return;
    }
    if(error == boost::beast::websocket::error::closed)
    {
        return;
    }
    if(error)
    {
        utils::handleSystemError(error, "ws read");
    }

    activity();

    // enqueue message
    const auto& bufferData = buffer.data();
    std::string msg(boost::asio::buffer_cast<char const*>(boost::beast::buffers_front(bufferData)),
                    boost::asio::buffer_size(bufferData));
    buffer.consume(buffer.size());

    {
        std::unique_lock<std::mutex> lock(mutexForQueue);
        queue.emplace_back(std::move(msg));
        conditionForQueue.notify_one();
    }
    doRead();
}

void WebsocketSession::write(std::string msg)
{
    auto self = shared_from_this();
    auto handler = bind_executor(strand, [self](error_code error, size_t size) {
        self->onWrite(error, size);
    });
    ws.async_write(boost::asio::buffer(msg), handler);
}

void WebsocketSession::onWrite(error_code error, size_t)
{
    if(error == boost::asio::error::operation_aborted)
    {
        return;
    }
    else if(error)
    {
        return utils::handleSystemError(error, "ws write");
    }
}

void WebsocketSession::processProc()
{
    auto self = shared_from_this();
    auto pickMsg = [this](std::string& msg) {
        std::unique_lock<std::mutex> lock(mutexForQueue);
        if(queue.empty())
        {
            // wait next msg
            conditionForQueue.wait(lock);
        }

        // pop the message
        msg.swap(queue.front());
        queue.pop_front();
    };

    while(ws.is_open())
    {
        std::string msg;
        pickMsg(msg);
        if(msg.size())
        {
            try
            {
                auto json = nlohmann::json::parse(msg);
            }
            catch(nlohmann::json::parse_error& parseError)
            {
                // TODO:
            };
        }
    }
}
