#ifndef HTTP_LISTENER_HPP
#define HTTP_LISTENER_HPP

#include "HttpUriRouter.hpp"
#include <memory>

class HttpListener : public std::enable_shared_from_this<HttpListener>
{
public:
    HttpListener(boost::asio::io_context&, const boost::asio::ip::tcp::endpoint&,
                 std::shared_ptr<std::string const>);

    void run();

    template <typename RequestHandler>
    void registHandler(std::string pattern, std::shared_ptr<RequestHandler> handler)
    {
        router.registHandler(std::move(pattern), handler);
    }

private:
    boost::asio::ip::tcp::socket socket;
    boost::asio::ip::tcp::acceptor acceptor;
    std::shared_ptr<std::string const> documentRoot;
    HttpUriRouter router;

    void doAccept();
    void onAccept(boost::system::error_code);
};

#endif // HTTP_LISTENER_HPP
