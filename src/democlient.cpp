#include "democlient.hpp"
#include "HttpClient.hpp"
#include "PostPicture.hpp"
#include <nlohmann/json.hpp>
#include <string>

DemoClient::DemoClient(std::string server, uint16_t port)
: server{std::move(server)}, port(port)
{
}

void DemoClient::post(std::string path, const nlohmann::json& json)
{
    std::string uri{std::move(path)};
    HttpClient client{server, port, uri};
    client.post("application/json", json.dump());
}

int postPictures(const PostPicture* picture)
{
    nlohmann::json jsonData = *picture;
    DemoClient client{"127.0.0.1", 8088};
    try
    {
        client.post("/api/v1/realtime/preview/pictures", jsonData);
        return 0;
    }
    catch(...)
    {
        return 1;
    }
}