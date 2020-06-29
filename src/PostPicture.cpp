#include "PostPicture.hpp"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog-inl.h>
#include <sstream>

void to_json(nlohmann::json& json, const PostPicture& picture)
{
    auto getObjectType = [&picture]() -> char const* {
        if(picture.objectType == PictureObjectType::face)
        {
            return "face";
        }
        return "body";
    };
    auto getData = [&picture]() -> std::string {
        std::stringstream ret;
        namespace iters = boost::archive::iterators;
        using B64EncodeIter =
            iters::base64_from_binary<iters::transform_width<char const*, 6, 8>>;
        std::copy(B64EncodeIter(picture.data),
                  B64EncodeIter(&picture.data[picture.size]),
                  std::ostream_iterator<char>(ret));

        size_t equal_count = (3 - picture.size % 3) % 3;
        for(size_t i = 0; i < equal_count; i++)
        {
            ret.put('=');
        }
        return std::move(ret.str());
    };
    json = nlohmann::json{
        {"camera", picture.cameraId}, {"object", picture.objectId},
        {"type", getObjectType()},    {"timestamp", picture.timestamp},
        {"width", picture.width},     {"height", picture.height},
        {"size", picture.size},       {"data", getData()}};
}

void from_json(const nlohmann::json& json, PostPicture& picture)
{
    std::string typeName;
    std::string encodedData;

    auto parseData = [&picture](const std::string& data) -> void {
        namespace iters = boost::archive::iterators;
        using B64DecodeIter =
            iters::transform_width<iters::binary_from_base64<std::string::const_iterator>, 8, 6>;
        try
        {
            std::copy(B64DecodeIter(data.begin()), B64DecodeIter(data.end()),
                      picture.data);
        }
        catch(const std::exception& err)
        {
            throw std::runtime_error(err.what());
        }
    };

    json.at("camera").get_to(picture.cameraId);
    json.at("width").get_to(picture.width);
    json.at("height").get_to(picture.height);
    json.at("object").get_to(picture.objectId);
    json.at("timestamp").get_to(picture.timestamp);
    json.at("size").get_to(picture.size);
    json.at("type").get_to(typeName);
    picture.objectType =
        (typeName == "face") ? PictureObjectType::face : PictureObjectType::body;
    json.at("data").get_to(encodedData);
    if(picture.data)
    {
        delete[] picture.data;
    }
    picture.data = new char[picture.size];
    parseData(encodedData);
}

bool operator==(const PostPicture& a, const PostPicture& b)
{
    return ((a.height == b.height) && (a.width == b.width) &&
            (a.cameraId == b.cameraId) && (a.timestamp == b.timestamp) &&
            (a.objectId == b.objectId) && (a.objectType == b.objectType) &&
            (a.size == b.size) && (std::memcmp(a.data, b.data, a.size) == 0));
}

bool operator!=(const PostPicture& a, const PostPicture& b)
{
    return !(a == b);
}

PostPicture* createPicture()
{
    auto picture = new PostPicture();
    picture->size = 0;
    picture->data = nullptr;
    picture->width = 0;
    picture->height = 0;
    picture->objectId = 0;
    picture->objectType = PictureObjectType::face;
    picture->timestamp = 0;
    picture->cameraId = 0;
    return picture;
}

void freePicture(PostPicture* pic)
{
    if(pic->data)
    {
        spdlog::info("Free picture data, obj: {}, camera: {}", pic->objectId, pic->cameraId);
        ::free(pic->data);
    }
    free(pic);
}

PostPictureWrapper::PostPictureWrapper() : PostPictureWrapper(createPicture())
{
    spdlog::info("Create new empty picture");
}

PostPicture& PostPictureWrapper::picture()
{
    return *picture_;
}

PostPictureWrapper::PostPictureWrapper(PostPicture* picture)
{
    picture_.reset(picture);
    timestamp_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    spdlog::info("Assign picture data, obj: {}, camera: {}", picture_->objectId,
                 picture_->cameraId);
}

time_t PostPictureWrapper::timestamp() const
{
    return timestamp_;
}
