#ifndef UTILS_HPP
#define UTILS_HPP

#include "HttpSession.hpp"
#include <boost/beast/core/string.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>

namespace utils
{
using boost::beast::string_view;
using boost::system::error_code;
using Response = boost::beast::http::response<boost::beast::http::string_body>;
using HttpVerb = boost::beast::http::verb;
using HttpStatus = boost::beast::http::status;
using HttpField = boost::beast::http::field;

void handleSystemError(error_code, char const*);

bool isDirectory(const char* path);

Response createJsonOkRequest(const HttpSession::Request&, nlohmann::json&&);

Response createJsonBadRequest(const HttpSession::Request&, string_view);

Response createJsonNotFound(const HttpSession::Request&, string_view);

Response createJsonServerError(const HttpSession::Request&, string_view);

} // namespace utils

#endif // UTILS_HPP
