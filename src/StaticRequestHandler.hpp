#ifndef STATIC_REQUEST_HANDLER_HPP
#define STATIC_REQUEST_HANDLER_HPP

#include "HttpSession.hpp"
#include "Utils.hpp"
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>

namespace http = boost::beast::http;

class StaticRequestHandler : public std::enable_shared_from_this<StaticRequestHandler>
{

public:
    explicit StaticRequestHandler(boost::beast::string_view);

    void handleRequest(HttpSession::Request&&, HttpSession::Queue&);

private:
    boost::beast::string_view documentRoot;

    static boost::beast::string_view getMimeType(boost::beast::string_view);

    static std::string pathConcat(boost::beast::string_view, boost::beast::string_view);

    static http::response<http::string_body> createBadRequest(HttpSession::Request&,
                                                              boost::beast::string_view);

    static http::response<http::string_body> createNotFound(HttpSession::Request&,
                                                            boost::beast::string_view);

    static http::response<http::string_body> createServerError(HttpSession::Request&,
                                                               boost::beast::string_view);
};

#endif // STATIC_REQUEST_HANDLER_HPP
