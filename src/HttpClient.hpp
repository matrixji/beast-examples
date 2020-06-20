#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <string>

class HttpClient
{
public:
    HttpClient(std::string, uint16_t, std::string);

    void post(const std::string&, std::string&&);

private:
    std::string host;
    uint16_t port;
    std::string target;
    boost::asio::io_context ioContext{};
    boost::asio::ip::tcp::resolver resolver{ioContext};
    boost::asio::ip::tcp::socket socket{ioContext};
    boost::beast::flat_buffer buffer{};
    boost::beast::http::request<boost::beast::http::string_body> req;
    boost::beast::http::response<boost::beast::http::dynamic_body> resp{};
};

#endif // HTTP_CLIENT_HPP
