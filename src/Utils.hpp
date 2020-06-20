#ifndef UTILS_HPP
#define UTILS_HPP

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>

namespace utils
{
using Request = boost::beast::http::request<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::string_body>;

void handleSystemError(boost::system::error_code, char const*);

bool isDirectory(const char* path);

Response createJsonOkRequest(const Request&, nlohmann::json&&);

Response createJsonBadRequest(const Request&, boost::string_view);

Response createJsonNotFound(const Request&, boost::string_view);

Response createJsonServerError(const Request&, boost::string_view);

const char* getServerSignature();

const char* getClientAgent();

} // namespace utils

#endif // UTILS_HPP
