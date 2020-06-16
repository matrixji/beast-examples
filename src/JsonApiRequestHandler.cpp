#include "JsonApiRequestHandler.hpp"
#include "Utils.hpp"
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <utility>

JsonApiRequestHandler::JsonApiRequestHandler(JsonApiRequestHandler::HandlerFunction func)
: func{std::move(func)}
{
}

void JsonApiRequestHandler::handleRequest(HttpSession::Request&& req, HttpSession::Queue& send)
{
    using HttpVerb = boost::beast::http::verb;
    if(req.method() != HttpVerb::get and req.method() != HttpVerb::post)
    {
        return send(std::move(utils::createJsonBadRequest(req, "Unsupported method.")));
    }

    try
    {
        return send(std::move(utils::createJsonOkRequest(req, std::move(func(req)))));
    }
    catch(const JsonApiRequestHandler::NotFound&)
    {
        return send(std::move(utils::createJsonNotFound(req, req.target())));
    }
    catch(const std::exception& ex)
    {
        return send(std::move(utils::createJsonServerError(req, ex.what())));
    }
}
