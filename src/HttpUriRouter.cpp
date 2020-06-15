#include "HttpUriRouter.hpp"
HttpUriRouter::HandlerFunction HttpUriRouter::resolve(const std::string& path)
{
    for(const auto& handler : handlers)
    {
        const auto& regex = std::get<0>(handler);
        if(std::regex_match(path, regex))
        {
            return std::get<1>(handler);
        }
    }
    return std::get<1>(handlers.back());
}

HttpUriRouter::Handler HttpUriRouter::buildHandler(const std::string& pattern,
                                                   HttpUriRouter::HandlerFunction function)
{
    std::regex regex{pattern, std::regex_constants::ECMAScript};
    return Handler{std::move(regex), std::move(function)};
}
