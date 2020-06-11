#include "Utils.hpp"
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

void utils::handleSystemError(boost::system::error_code ec, char const* what)
{
    std::string mess = "System error: " + ec.message();
    if(what)
    {
        mess += std::string(". ") + what;
    }
    std::cerr << mess << std::endl;
}

bool utils::isDirectory(const char* path)
{
    using stat = struct stat;
    stat buffer{0};
    return (::stat(path, &buffer) == 0) and S_ISDIR(buffer.st_mode);
}