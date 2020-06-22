#include "StaticRequestHandler.hpp"
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>

using boost::system::error_code;
using Request = boost::beast::http::request<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::string_body>;

StaticRequestHandler::StaticRequestHandler(boost::string_view documentRoot)
: documentRoot{documentRoot}
{
}

void StaticRequestHandler::handleRequest(Request&& req, HttpSession::Queue& send)
{
    using boost::beast::file_mode;
    using boost::beast::http::field;
    using boost::beast::http::file_body;
    using boost::beast::http::status;
    using boost::beast::http::verb;
    using EmptyBody = boost::beast::http::response<boost::beast::http::empty_body>;
    using FileBody = boost::beast::http::response<boost::beast::http::file_body>;

    if(req.method() != verb::get and req.method() != verb::head)
    {
        return send(req, createBadRequest(req, "Unsupported method."));
    }

    if(req.target().empty() or req.target()[0] != '/' or
       req.target().find("..") != boost::string_view::npos)
    {
        return send(req, createBadRequest(req, "Illegal request-target"));
    }

    auto path = pathConcat(documentRoot, req.target());
    if(req.target().back() == '/')
    {
        path.append("index.html");
    }
    while(utils::isDirectory(path.c_str()))
    {
        path.append("/index.html");
    }

    error_code error;
    file_body::value_type body;

    body.open(path.c_str(), file_mode::scan, error);

    if(error == boost::system::errc::no_such_file_or_directory)
    {
        return send(req, createNotFound(req, req.target()));
    }

    if(error)
    {
        return send(req, createServerError(req, error.message()));
    }

    const auto size = body.size();

    // resp for HEAD
    if(req.method() == verb::head)
    {
        EmptyBody res{status::ok, req.version()};
        res.set(field::server, utils::getServerSignature());
        res.set(field::content_type, getMimeType(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(req, std::move(res));
    }

    // resp for get
    FileBody res{std::piecewise_construct, std::make_tuple(std::move(body)),
                 std::make_tuple(status::ok, req.version())};
    res.set(field::server, utils::getServerSignature());
    res.set(field::content_type, getMimeType(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(req, std::move(res));
}

boost::string_view StaticRequestHandler::getMimeType(boost::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if(pos == boost::string_view::npos)
        {
            return boost::string_view{};
        };
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))
    {
        return "text/html";
    }
    if(iequals(ext, ".html"))
    {
        return "text/html";
    }
    if(iequals(ext, ".css"))
    {
        return "text/css";
    }
    if(iequals(ext, ".txt"))
    {
        return "text/plain";
    }
    if(iequals(ext, ".js"))
    {
        return "application/javascript";
    }
    if(iequals(ext, ".json"))
    {
        return "application/json";
    }
    if(iequals(ext, ".xml"))
    {
        return "application/xml";
    }
    if(iequals(ext, ".png"))
    {
        return "image/png";
    }
    if(iequals(ext, ".jpe"))
    {
        return "image/jpeg";
    }
    if(iequals(ext, ".jpeg"))
    {
        return "image/jpeg";
    }
    if(iequals(ext, ".jpg"))
    {
        return "image/jpeg";
    }
    if(iequals(ext, ".gif"))
    {
        return "image/gif";
    }
    if(iequals(ext, ".bmp"))
    {
        return "image/bmp";
    }
    if(iequals(ext, ".ico"))
    {
        return "image/vnd.microsoft.icon";
    }
    if(iequals(ext, ".tiff"))
    {
        return "image/tiff";
    }
    if(iequals(ext, ".tif"))
    {
        return "image/tiff";
    }
    if(iequals(ext, ".svg"))
    {
        return "image/svg+xml";
    }
    if(iequals(ext, ".svgz"))
    {
        return "image/svg+xml";
    }
    return "application/data";
}

std::string StaticRequestHandler::pathConcat(boost::string_view base, boost::string_view path)
{
    if(base.empty())
    {
        return path.to_string();
    }
    std::string result = base.to_string();
#ifdef WIN32
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
    {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
    for(const auto& c : result)
    {
        if(c == '/')
        {
            c = path_separator;
        }
    }
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
    {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
#endif
    return result;
}

Response StaticRequestHandler::createBadRequest(Request& req, boost::string_view why)
{
    using boost::beast::http::field;
    using boost::beast::http::status;

    Response res{status::bad_request, req.version()};
    res.set(field::server, utils::getServerSignature());
    res.set(field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
}

Response StaticRequestHandler::createNotFound(Request& req, boost::string_view target)
{
    using boost::beast::http::field;
    using boost::beast::http::status;

    Response res{status::not_found, req.version()};
    res.set(field::server, utils::getServerSignature());
    res.set(field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + target.to_string() + "' was not found.";
    res.prepare_payload();
    return res;
}

Response StaticRequestHandler::createServerError(Request& req, boost::string_view what)
{
    using boost::beast::http::field;
    using boost::beast::http::status;

    Response res{status::internal_server_error, req.version()};
    res.set(field::server, utils::getServerSignature());
    res.set(field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + what.to_string() + "'";
    res.prepare_payload();
    return res;
}