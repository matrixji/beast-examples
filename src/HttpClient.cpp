#include "HttpClient.hpp"
#include <boost/asio/connect.hpp>
#include <boost/beast/version.hpp>
#include <iostream>

constexpr auto httpVersion{11};

HttpClient::HttpClient(std::string host, uint16_t port, std::string target)
: host{std::move(host)}
, port{port}
, target{std::move(target)}
, req{boost::beast::http::verb::post, target, httpVersion}
{
}

void HttpClient::post(const std::string& mime, std::string&& data)
{
    auto const result = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket, result.begin(), result.end());
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, target, httpVersion};
    req.set(boost::beast::http::field::host, host);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(boost::beast::http::field::content_type, mime);
    req.content_length(data.size());
    req.body().swap(data);
    std::cout << (data.length()) << std::endl;
    boost::beast::http::write(socket, req);
    boost::beast::http::read(socket, buffer, resp);
    boost::system::error_code error;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
}
