#include "HttpListener.hpp"
#include "HttpSession.hpp"
#include "Utils.hpp"
#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>

HttpListener::HttpListener(boost::asio::io_context& ioContext,
                           const boost::asio::ip::tcp::endpoint& endpoint,
                           std::shared_ptr<const std::string> documentRoot)
: socket{ioContext}, acceptor{ioContext}, documentRoot{std::move(documentRoot)}
{
    using boost::asio::socket_base;
    boost::system::error_code error;

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
    acceptor.async_accept(socket, [self = shared_from_this()](boost::system::error_code error) {
        self->onAccept(error);
    });
}

void HttpListener::onAccept(boost::system::error_code error)
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
