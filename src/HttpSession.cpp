#include "HttpSession.hpp"
#include "HttpUriRouter.hpp"
#include "Utils.hpp"
#include "WebsocketHandler.hpp"
#include "WebsocketSession.hpp"
#include <spdlog/spdlog-inl.h>
#include <utility>

using boost::asio::bind_executor;
using boost::asio::post;
using boost::asio::ip::tcp;
using boost::system::error_code;
using Request = boost::beast::http::request<boost::beast::http::string_body>;

HttpSession::HttpSession(tcp::socket sock, HttpUriRouter& router,
                         WebsocketHandler& wsHandler)
: wsHandler(wsHandler)
, socket{std::move(sock)}
, strand{socket.get_executor()}
, timer{socket.get_executor().context(), std::chrono::steady_clock::time_point::max()}
, queue{*this}
, router(router)
, peerAddress{socket.remote_endpoint().address().to_string() + ":" +
              std::to_string(socket.remote_endpoint().port())}
{
    spdlog::info("new session from: {}", peerAddress);
}

void HttpSession::run()
{
    if(!strand.running_in_this_thread())
    {
        auto self = shared_from_this();
        return post(bind_executor(strand, [self] { self->run(); }));
    }
    onTimer({});
    doRead();
}

void HttpSession::doRead()
{
    // TODO: try porting timout as parameter
    constexpr int timeout{15};
    timer.expires_after(std::chrono::seconds{timeout});

    // reset parser.
    parser.emplace();
    parser->body_limit(bodyLimit);

    // read header
    // error_code error;
    // http::read_header(socket, header, *parser, error);

    // read a request
    auto self = shared_from_this();
    auto callback = [self](error_code error, size_t readBytes) {
        self->onRead(error, readBytes);
    };
    boost::beast::http::async_read(socket, buffer, *parser,
                                   bind_executor(strand, std::move(callback)));
}

void HttpSession::onTimer(error_code error)
{
    if(error and error != boost::asio::error::operation_aborted)
    {
        return utils::handleSystemError(error, "timer");
    }

    // Check if this has been upgraded to Websocket
    if(timer.expires_at() == (std::chrono::steady_clock::time_point::min)())
    {
        return;
    }

    // Verify that the timer really expired since the deadline may have moved.
    if(timer.expiry() <= std::chrono::steady_clock::now())
    {
        // Closing the socket cancels all outstanding operations. They
        // will complete with asio::error::operation_aborted
        spdlog::info("close session: {}", peerAddress);
        socket.shutdown(tcp::socket::shutdown_both, error);
        socket.close(error);
        return;
    }

    // Wait on the timer
    auto self = shared_from_this();
    timer.async_wait(
        bind_executor(strand, [self](error_code error) { self->onTimer(error); }));
}

void HttpSession::onRead(error_code error, size_t)
{
    // Happens when the timer closes the socket
    if(error == boost::asio::error::operation_aborted)
    {
        return;
    }

    // This means they closed the connection
    if(error == boost::beast::http::error::end_of_stream)
    {
        return doClose();
    }

    if(error)
    {
        return utils::handleSystemError(error, "read");
    }

    // See if it is a WebSocket Upgrade
    if(boost::beast::websocket::is_upgrade(parser->get()))
    {
        // Make timer expire immediately, by setting expiry to time_point::min
        // we can detect the upgrade to websocket in the timer handler
        timer.expires_at((std::chrono::steady_clock::time_point::min)());

        // Create a WebSocket websocket session by transferring the socket
        std::make_shared<WebsocketSession>(std::move(socket), wsHandler)
            ->doAccept(parser->release());
        return;
    }

    // router
    Request request{parser->release()};
    auto uri = request.target().to_string();
    auto handler = router.resolve(uri);
    spdlog::info("session: {}, {} {}", peerAddress, request.method_string().data(), uri);
    handler(std::move(request), queue);

    // If we aren't at the queue limit, try to pipeline another request
    if(!queue.isFull())
    {
        doRead();
    }
}

void HttpSession::onWrite(error_code error, bool close)
{
    // Happens when the timer closes the socket
    if(error == boost::asio::error::operation_aborted)
    {
        return;
    }

    if(error)
    {
        return utils::handleSystemError(error, "write");
    }

    if(close)
    {
        return doClose();
    }

    // Inform the queue that a write completed
    if(queue.onWrite())
    {
        // Read another request
        doRead();
    }
}

void HttpSession::doClose()
{
    error_code error;
    socket.shutdown(tcp::socket::shutdown_both, error);
    socket.close(error);
}

const std::string& HttpSession::getPeerAddress() const
{
    return peerAddress;
}

HttpSession::Queue::Queue(HttpSession& session) : self(session)
{
    workers.reserve(limit);
}

bool HttpSession::Queue::onWrite()
{
    auto const full = isFull();
    workers.erase(workers.begin());
    if(!workers.empty())
    {
        (*workers.front())();
    }
    return full;
}