#ifndef HTTP_URI_ROUTER_HPP
#define HTTP_URI_ROUTER_HPP

#include "HttpSession.hpp"
#include <functional>
#include <regex>
#include <string>
#include <tuple>
#include <vector>

class HttpUriRouter
{
public:
    using HandlerFunction =
        std::function<void(HttpSession::Request&&, HttpSession::Queue&)>;

    template <typename RequestHandler>
    void registHandler(const std::string& pattern, std::shared_ptr<RequestHandler> handler)
    {
        auto func = [handler](HttpSession::Request&& request, HttpSession::Queue& send) {
            handler->handleRequest(std::move(request), send);
        };
        handlers.emplace_back(std::move(buildHandler(pattern, std::move(func))));
    }

    HandlerFunction resolve(const std::string&);

private:
    using Handler = std::tuple<std::regex, HandlerFunction>;
    using Handlers = std::vector<Handler>;
    Handlers handlers;

    Handler buildHandler(const std::string&, HandlerFunction);
};

#endif // HTTP_URI_ROUTER_HPP
