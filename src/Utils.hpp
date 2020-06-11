#ifndef UTILS_HPP
#define UTILS_HPP

#include <boost/system/error_code.hpp>

namespace utils
{
void handleSystemError(boost::system::error_code, char const*);

bool isDirectory(const char* path);

} // namespace utils

#endif // UTILS_HPP
