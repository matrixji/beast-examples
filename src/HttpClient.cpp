#include "HttpClient.hpp"
#include "Utils.hpp"
#include <boost/asio/connect.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <iostream>

using boost::asio::ip::tcp;
using boost::beast::http::field;
using boost::beast::http::verb;
using boost::system::error_code;

constexpr auto httpVersion{11};

HttpClient::HttpClient(std::string host, uint16_t port, std::string target)
: host{std::move(host)}, port{port}, target{std::move(target)}, req{verb::post, target, httpVersion}
{
}

void HttpClient::post(const std::string& mime, std::string&& data)
{
    auto const result = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket, result.begin(), result.end());
    req.set(field::host, host);
    req.set(field::user_agent, utils::getClientAgent());
    req.set(field::content_type, mime);
    req.content_length(data.size());
    req.body().swap(data);
    std::cout << (data.length()) << std::endl;
    boost::beast::http::write(socket, req);
    boost::beast::http::read(socket, buffer, resp);
    error_code error;
    socket.shutdown(tcp::socket::shutdown_both, error);
}
