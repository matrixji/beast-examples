#ifndef STATIC_REQUEST_HANDLER_HPP
#define STATIC_REQUEST_HANDLER_HPP

#include "HttpSession.hpp"
#include "Utils.hpp"
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>

class StaticRequestHandler : public std::enable_shared_from_this<StaticRequestHandler>
{
public:
    explicit StaticRequestHandler(boost::beast::string_view);
    void handleRequest(HttpSession::Request&&, HttpSession::Queue&);

private:
    using Response = boost::beast::http::response<boost::beast::http::string_body>;
    using HttpStatus = boost::beast::http::status;
    using HttpField = boost::beast::http::field;
    using HttpVerb = boost::beast::http::verb;

    boost::beast::string_view documentRoot;

    static boost::beast::string_view getMimeType(boost::beast::string_view);
    static std::string pathConcat(boost::beast::string_view, boost::beast::string_view);
    static Response createBadRequest(HttpSession::Request&, boost::beast::string_view);
    static Response createNotFound(HttpSession::Request&, boost::beast::string_view);
    static Response createServerError(HttpSession::Request&, boost::beast::string_view);
};

#endif // STATIC_REQUEST_HANDLER_HPP
