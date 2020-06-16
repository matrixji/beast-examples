#include "Utils.hpp"
#include <boost/beast/version.hpp>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

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

utils::Response utils::createJsonOkRequest(const HttpSession::Request& req, nlohmann::json&& json)
{
    json.emplace("ret", 0);
    Response res{HttpStatus::ok, req.version()};
    res.set(HttpField::server, BOOST_BEAST_VERSION_STRING);
    res.set(HttpField::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

utils::Response utils::createJsonBadRequest(const HttpSession::Request& req, utils::string_view why)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", why},
    };

    Response res{HttpStatus::bad_request, req.version()};
    res.set(HttpField::server, BOOST_BEAST_VERSION_STRING);
    res.set(HttpField::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

utils::Response utils::createJsonNotFound(const HttpSession::Request& req, utils::string_view target)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", std::string(target) + " not found"},
    };

    Response res{HttpStatus::not_found, req.version()};
    res.set(HttpField::server, BOOST_BEAST_VERSION_STRING);
    res.set(HttpField::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

utils::Response utils::createJsonServerError(const HttpSession::Request& req, utils::string_view what)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", what},
    };

    Response res{HttpStatus::internal_server_error, req.version()};
    res.set(HttpField::server, BOOST_BEAST_VERSION_STRING);
    res.set(HttpField::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}