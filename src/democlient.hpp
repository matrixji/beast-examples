#ifndef DEMO_CLIENT_HPP
#define DEMO_CLIENT_HPP

#include "democlient.h"
#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <string>

class DemoClient
{
public:
    DemoClient(std::string, uint16_t);
    void post(std::string, const nlohmann::json& json);

private:
    std::string server;
    uint16_t port;
};

#endif // DEMO_CLIENT_HPP
