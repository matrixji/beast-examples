#include "HttpListener.hpp"
#include "HttpSession.hpp"
#include "Utils.hpp"
#include <boost/asio/dispatch.hpp>

using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::system::error_code;

HttpListener::HttpListener(io_context& ioContext, const tcp::endpoint& endpoint,
                           std::shared_ptr<const std::string> documentRoot)
: socket{ioContext}, acceptor{ioContext}, documentRoot{std::move(documentRoot)}
{
    using boost::asio::socket_base;
    error_code error;

    acceptor.open(endpoint.protocol(), error);
    if(error)
    {
        utils::handleSystemError(error, "Acceptor open");
        return;
    }

    acceptor.set_option(socket_base::reuse_address{true}, error);
    if(error)
    {
        utils::handleSystemError(error, "Acceptor set_option");
        return;
    }

    acceptor.bind(endpoint, error);
    if(error)
    {
        utils::handleSystemError(error, "Acceptor bind");
        return;
    }

    acceptor.listen(socket_base::max_listen_connections, error);
    if(error)
    {
        utils::handleSystemError(error, "Acceptor listen");
        return;
    }
}
void HttpListener::run()
{
    if(acceptor.is_open())
    {
        auto self{shared_from_this()};
        boost::asio::dispatch(acceptor.get_executor(),
                              [self, this] { doAccept(); });
    }
}
void HttpListener::doAccept()
{
    auto self = shared_from_this();
    acceptor.async_accept(socket,
                          [self](error_code error) { self->onAccept(error); });
}

void HttpListener::onAccept(error_code error)
{
    if(error)
    {
        utils::handleSystemError(error, "accept");
    }
    else
    {
        auto session = std::make_shared<HttpSession>(std::move(socket), router);
        session->run();
    }
    doAccept();
}
