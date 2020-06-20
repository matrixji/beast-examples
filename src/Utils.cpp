#include "Utils.hpp"
#include <boost/beast/http/parser.hpp>
#include <iostream>
#include <sys/stat.h>

using boost::beast::http::field;
using boost::beast::http::status;
using boost::system::error_code;
using Request = boost::beast::http::request<boost::beast::http::string_body>;
using RequestParser = boost::beast::http::request_parser<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::string_body>;

void utils::handleSystemError(error_code ec, char const* what)
{
    std::string mess = "System error: " + ec.message();
    if(what)
    {
        mess += std::string(". ") + what;
    }
    std::cerr << mess << std::endl;
}

bool utils::isDirectory(const char* path)
{
    using stat = struct stat;
    stat buffer{};
    return (::stat(path, &buffer) == 0) and S_ISDIR(buffer.st_mode);
}

Response utils::createJsonOkRequest(const Request& req, nlohmann::json&& json)
{
    json.emplace("ret", 0);
    Response res{status::ok, req.version()};
    res.set(field::server, getServerSignature());
    res.set(field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

Response utils::createJsonBadRequest(const Request& req, boost::string_view why)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", why},
    };

    Response res{status::bad_request, req.version()};
    res.set(field::server, getServerSignature());
    res.set(field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

Response utils::createJsonNotFound(const Request& req, boost::string_view target)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", std::string(target) + " not found"},
    };

    Response res{status::not_found, req.version()};
    res.set(field::server, getServerSignature());
    res.set(field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

Response utils::createJsonServerError(const Request& req, boost::string_view what)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", what},
    };

    Response res{status::internal_server_error, req.version()};
    res.set(field::server, getServerSignature());
    res.set(field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

const char* utils::getServerSignature()
{
    return "UNIS-WEB-SERVER/1.0";
}

const char* utils::getClientAgent()
{
    return "UNIS-WEB-CLIENT/1.0";
}