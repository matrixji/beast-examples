#ifndef JSON_API_REQUEST_HANDLER_HPP
#define JSON_API_REQUEST_HANDLER_HPP

#include "HttpUriRouter.hpp"
#include <nlohmann/json.hpp>
#include <string>

class JsonApiRequestHandler : public std::enable_shared_from_this<JsonApiRequestHandler>
{
public:
    class NotFound : public std::exception
    {
    public:
        explicit NotFound(const char* mess) : mess{mess}
        {
        }

        const char* what() const noexcept override
        {
            return mess.c_str();
        }

    private:
        std::string mess;
    };

    using HandlerFunction = std::function<nlohmann::json(HttpSession::Request&)>;

    explicit JsonApiRequestHandler(HandlerFunction);

    void handleRequest(HttpSession::Request&&, HttpSession::Queue&);

private:
    HandlerFunction func;
};

#endif // JSON_API_REQUEST_HANDLER_HPP
