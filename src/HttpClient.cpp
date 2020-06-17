#include "HttpClient.hpp"
#include <boost/asio/connect.hpp>
#include <boost/beast/version.hpp>

HttpClient::HttpClient(std::string host, uint16_t port, std::string target)
: host{std::move(host)}, port{port}, target{std::move(target)}
{}

void HttpClient::post(const std::string& mime, std::string&  data)
{
    auto const result = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket, result.begin(), result.end());
    boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::post, target};
    req.set(boost::beast::http::field::host, host);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
}
