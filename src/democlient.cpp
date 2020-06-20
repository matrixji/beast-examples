#include "democlient.h"
#include "HttpClient.hpp"
#include <string>

int postPictures(const PostPicture* snaps, unsigned int numOfSnaps,
                 const PostPicture* tracks, unsigned int numOfTracks)
{
    constexpr unsigned int numOfSnapsLimit{5};
    if(numOfSnaps != numOfSnapsLimit)
    {
        return 1;
    }

    auto getTotalSize = [snaps, numOfSnaps, tracks, numOfTracks]() -> size_t {
        size_t total{0};
        for(unsigned int i = 0; i < numOfSnaps; i++)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            total += snaps[i].size + sizeof(uint32_t);
        }
        for(unsigned int i = 0; i < numOfTracks; i++)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            total += tracks[i].size + sizeof(uint32_t);
        }
        return total;
    };

    auto fillData = [snaps, numOfSnaps, tracks, numOfTracks](std::string& data) {
        auto pc = &data.at(0);
        for(unsigned int i = 0; i < numOfSnaps; i++)
        {
            uint32_t length = snaps[i].size; // NOLINT
            *(pc++) = static_cast<uint8_t>((length & 0xff000000) >> 24); // NOLINT
            *(pc++) = static_cast<uint8_t>((length & 0x00ff0000) >> 16); // NOLINT
            *(pc++) = static_cast<uint8_t>((length & 0x0000ff00) >> 8); // NOLINT
            *(pc++) = static_cast<uint8_t>(length & 0x000000ff);  // NOLINT
            std::copy(snaps[i].data, snaps[i].data + length, pc); // NOLINT
            std::advance(pc, length);
        }
        for(unsigned int i = 0; i < numOfTracks; i++)
        {
            uint32_t length = tracks[i].size; // NOLINT
            *(pc++) = static_cast<uint8_t>((length & 0xff000000) >> 24); // NOLINT
            *(pc++) = static_cast<uint8_t>((length & 0x00ff0000) >> 16); // NOLINT
            *(pc++) = static_cast<uint8_t>((length & 0x0000ff00) >> 8); // NOLINT
            *(pc++) = static_cast<uint8_t>(length & 0x000000ff);    // NOLINT
            std::copy(tracks[i].data, tracks[i].data + length, pc); // NOLINT
            std::advance(pc, length);
        }
    };

    std::string server{"127.0.0.1"};
    constexpr uint16_t port{8088};
    std::string uri{"/api/v1/realtime/preview/pictures"};
    size_t dataLength = getTotalSize();
    std::string data;
    data.resize(dataLength);
    fillData(data);

    try
    {
        HttpClient client{server, port, uri};
        client.post("application/binary", std::move(data));
    }
    catch(std::exception&)
    {
        return 1;
    }
    return 0;
}