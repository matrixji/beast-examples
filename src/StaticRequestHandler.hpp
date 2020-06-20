#ifndef STATIC_REQUEST_HANDLER_HPP
#define STATIC_REQUEST_HANDLER_HPP

#include "HttpSession.hpp"
#include "Utils.hpp"
#include <boost/filesystem.hpp>

class StaticRequestHandler : public std::enable_shared_from_this<StaticRequestHandler>
{
public:
    using Request = boost::beast::http::request<boost::beast::http::string_body>;
    using Response = boost::beast::http::response<boost::beast::http::string_body>;
    explicit StaticRequestHandler(boost::string_view);
    void handleRequest(Request&&, HttpSession::Queue&);

private:
    boost::string_view documentRoot;

    static boost::string_view getMimeType(boost::string_view);
    static std::string pathConcat(boost::string_view, boost::string_view);
    static Response createBadRequest(Request&, boost::string_view);
    static Response createNotFound(Request&, boost::string_view);
    static Response createServerError(Request&, boost::string_view);
};

#endif // STATIC_REQUEST_HANDLER_HPP
