#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>

class HttpClient
{
public:
    HttpClient(std::string, uint16_t, std::string);

    void post(const std::string&, std::string&);

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
