#include "JsonApiRequestHandler.hpp"
#include "Utils.hpp"
#include <nlohmann/json.hpp>
#include <utility>

using boost::beast::http::verb;

JsonApiRequestHandler::JsonApiRequestHandler(JsonApiRequestHandler::HandlerFunction func)
: func{std::move(func)}
{
}

void JsonApiRequestHandler::handleRequest(Request&& req, HttpSession::Queue& send)
{
    if(req.method() != verb::get and req.method() != verb::post)
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
