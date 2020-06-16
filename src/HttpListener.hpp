#ifndef HTTP_LISTENER_HPP
#define HTTP_LISTENER_HPP

#include "HttpUriRouter.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>

class HttpListener : public std::enable_shared_from_this<HttpListener>
{
    using io_context = boost::asio::io_context;
    using tcp = boost::asio::ip::tcp;
    using error_code = boost::system::error_code;

public:
    HttpListener(io_context&, const tcp::endpoint&, std::shared_ptr<std::string const>);

    void run();

    template <typename RequestHandler>
    void registHandler(std::string pattern, std::shared_ptr<RequestHandler> handler)
    {
        router.registHandler(std::move(pattern), handler);
    }

private:
    tcp::socket socket;
    tcp::acceptor acceptor;
    std::shared_ptr<std::string const> documentRoot;
    HttpUriRouter router;

    void doAccept();
    void onAccept(error_code);
};

#endif // HTTP_LISTENER_HPP
