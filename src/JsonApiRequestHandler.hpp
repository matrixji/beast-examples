#ifndef JSON_API_REQUEST_HANDLER_HPP
#define JSON_API_REQUEST_HANDLER_HPP

#include "HttpUriRouter.hpp"
#include <string>

namespace http = boost::beast::http;

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

    using HandlerFunction = std::function<std::string(HttpSession::Request&)>;

    explicit JsonApiRequestHandler(HandlerFunction);

    void handleRequest(HttpSession::Request&&, HttpSession::Queue&);

private:
    static http::response<http::string_body> createBadRequest(HttpSession::Request&,
                                                              boost::beast::string_view);
    static http::response<http::string_body> createNotFound(HttpSession::Request&,
                                                            boost::beast::string_view);
    static http::response<http::string_body> createServerError(HttpSession::Request&,
                                                               boost::beast::string_view);

    HandlerFunction func;
};

#endif // JSON_API_REQUEST_HANDLER_HPP
