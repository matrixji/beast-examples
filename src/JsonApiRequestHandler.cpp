#include "JsonApiRequestHandler.hpp"
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <utility>

JsonApiRequestHandler::JsonApiRequestHandler(JsonApiRequestHandler::HandlerFunction func)
: func{std::move(func)}
{
}

void JsonApiRequestHandler::handleRequest(HttpSession::Request&& req, HttpSession::Queue& send)
{
    if(req.method() != http::verb::get and req.method() != http::verb::post)
    {
        return send(createBadRequest(req, "Unsupported method."));
    }

    try
    {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = func(req);
        res.prepare_payload();
        send(std::move(res));
    }
    catch(const JsonApiRequestHandler::NotFound&)
    {
        send(std::move(createNotFound(req, req.target())));
    }
    catch(const std::exception& ex)
    {
        send(std::move(createServerError(req, ex.what())));
    }
}

//
// json error response
//
// {
//   "ret": <non-0>,
//   "error": "error message"
// }
http::response<http::string_body>
JsonApiRequestHandler::createBadRequest(HttpSession::Request& req, boost::beast::string_view why)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", why},
    };

    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
JsonApiRequestHandler::createServerError(HttpSession::Request& req, boost::beast::string_view what)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", what},
    };

    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
JsonApiRequestHandler::createNotFound(HttpSession::Request& req, boost::beast::string_view target)
{
    nlohmann::json json = {
        {"ret", 1},
        {"error", std::string(target) + " not found"},
    };

    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = json.dump();
    res.prepare_payload();
    return res;
}
