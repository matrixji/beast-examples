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
        return send(utils::createJsonBadRequest(req, "Unsupported method."));
    }

    try
    {
        return send(utils::createJsonOkRequest(req, func(req)));
    }
    catch(const JsonApiRequestHandler::NotFound&)
    {
        return send(utils::createJsonNotFound(req, req.target()));
    }
    catch(const std::exception& ex)
    {
        return send(utils::createJsonServerError(req, ex.what()));
    }
}
