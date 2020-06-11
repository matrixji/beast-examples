#include "StaticRequestHandler.hpp"

StaticRequestHandler::StaticRequestHandler(boost::beast::string_view documentRoot)
: documentRoot{documentRoot}
{
}

void StaticRequestHandler::handleRequest(HttpSession::Request&& req, HttpSession::Queue& send)
{
    if(req.method() != http::verb::get and req.method() != http::verb::head)
    {
        return send(createBadRequest(req, "Unsupported method."));
    }

    if(req.target().empty() or req.target()[0] != '/' or
       req.target().find("..") != boost::beast::string_view::npos)
    {
        return send(createBadRequest(req, "Illegal request-target"));
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

    boost::beast::error_code error;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, error);

    if(error == boost::system::errc::no_such_file_or_directory)
    {
        return send(createNotFound(req, req.target()));
    }

    if(error)
    {
        return send(createServerError(req, error.message()));
    }

    const auto size = body.size();

    // resp for HEAD
    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, getMimeType(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // resp for get
    http::response<http::file_body> res{
        std::piecewise_construct, std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, getMimeType(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

boost::beast::string_view StaticRequestHandler::getMimeType(boost::beast::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if(pos == boost::beast::string_view::npos)
        {
            return boost::beast::string_view{};
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

std::string StaticRequestHandler::pathConcat(boost::beast::string_view base,
                                             boost::beast::string_view path)
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

http::response<http::string_body>
StaticRequestHandler::createBadRequest(HttpSession::Request& req, boost::beast::string_view why)
{
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
StaticRequestHandler::createNotFound(HttpSession::Request& req, boost::beast::string_view target)
{
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + target.to_string() + "' was not found.";
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
StaticRequestHandler::createServerError(HttpSession::Request& req, boost::beast::string_view what)
{
    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + what.to_string() + "'";
    res.prepare_payload();
    return res;
}