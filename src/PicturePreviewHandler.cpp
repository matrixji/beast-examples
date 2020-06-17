
#include "PicturePreviewHandler.hpp"
#include "Utils.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/version.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <ctime>

PicturePreviewHandler::Pictures::Pictures(std::string uuid, time_t timestamp,
                                          std::vector<PictureView>&& pvs)
: uuid_{std::move(uuid)}, timestamp_{timestamp}
{
    for(auto& pv : pvs)
    {
        pictures.emplace_back(std::make_shared<PictureView>(std::move(pv)));
    }
}

inline std::string PicturePreviewHandler::Pictures::uuid() const
{
    return uuid_;
}

inline time_t PicturePreviewHandler::Pictures::timestamp() const
{
    return timestamp_;
}

std::shared_ptr<PicturePreviewHandler::PictureView>
PicturePreviewHandler::Pictures::picture(size_t index) const
{
    try
    {
        return pictures.at(index);
    }
    catch(std::out_of_range&)
    {
        return std::make_shared<PictureView>(std::string{});
    }
}

PicturePreviewHandler::PictureView::PictureView(std::string data)
: data{std::move(data)}
{
}

PicturePreviewHandler::PictureView::PictureView(PicturePreviewHandler::PictureView&& pv) noexcept
: data{std::move(pv.data)}
{
}

PicturePreviewHandler::PictureView&
PicturePreviewHandler::PictureView::operator=(PicturePreviewHandler::PictureView&& pv) noexcept
{
    if(this != &pv)
    {
        data.swap(pv.data);
    }
    return *this;
}

const std::string& PicturePreviewHandler::PictureView::getData() const
{
    return data;
}

PicturePreviewHandler::PicturePreviewHandler(size_t cacheLimit)
: cacheLimit{cacheLimit}
{
}

nlohmann::json PicturePreviewHandler::createPreview(std::vector<PictureView>&& pictureViews)
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    time_t now = std::time(nullptr);
    auto uuidStr = boost::uuids::to_string(uuid);

    // with lock
    {
        std::unique_lock<std::mutex> lock(mutex);
        pictures.emplace(uuidStr, Pictures(uuidStr, now, std::move(pictureViews)));
        ids.emplace_front(std::move(uuidStr));
        while(ids.size() > cacheLimit)
        {
            pictures.erase(ids.back());
            ids.pop_back();
        }
    }
    return nlohmann::json{};
}

void to_json(nlohmann::json& json, const PicturePreviewHandler::Pictures& pvs)
{
    auto getMask = [&pvs]() -> int {
        int mask = 0;
        int currMask = 1;
        constexpr unsigned int numOfPics{5};
        for(size_t i = 0; i < numOfPics; i++)
        {
            auto pic = pvs.picture(i);
            if(pic->getData().size() > 0)
            {
                mask |= currMask;
            }
            currMask <<= 1;
        }
        return mask;
    };
    json = nlohmann::json{
        {"uuid", pvs.uuid()}, {"timestamp", pvs.timestamp()}, {"mask", getMask()}};
}

nlohmann::json PicturePreviewHandler::listPreview(const std::string& prev, size_t limit)
{
    std::vector<Pictures> pics{};
    {
        std::unique_lock<std::mutex> lock(mutex);
        size_t n = 0;
        for(const auto& uuidStr : ids)
        {
            if(uuidStr == prev or (limit > 0 and n >= limit))
            {
                break;
            }
            pics.emplace_back(pictures.at(uuidStr));
            ++n;
        }
    }
    return nlohmann::json{{"pictures", pics}};
}

PicturePreviewHandler::Response PicturePreviewHandler::visitPreview(const HttpSession::Request& req)
{
    auto getPicture = [this](const std::string& uuid, size_t index) {
        std::unique_lock<std::mutex> lock(mutex);
        return pictures.at(uuid).picture(index);
    };

    std::vector<std::string> paths;
    std::string uri = req.target().to_string();
    boost::split(paths, uri, boost::is_any_of("/"), boost::token_compress_on);
    size_t index{0};
    boost::conversion::try_lexical_convert(paths.at(paths.size() - 1), index);
    std::string& uuid = paths.at(paths.size() - 2);

    try
    {
        auto picture = getPicture(uuid, index);
        Response res{utils::HttpStatus::ok, req.version()};
        res.set(utils::HttpField::server, BOOST_BEAST_VERSION_STRING);
        res.set(utils::HttpField::content_type, "image/jpeg");
        res.keep_alive(req.keep_alive());
        res.body() = picture->getData();
        res.prepare_payload();
        return res;
    }
    catch(std::out_of_range&)
    {
        return utils::createJsonNotFound(req, uri);
    }
}

void PicturePreviewHandler::handleRequest(HttpSession::Request&& req, HttpSession::Queue& send)
{
    using HttpVerb = boost::beast::http::verb;
    if(req.method() != HttpVerb::get)
    {
        return send(utils::createJsonBadRequest(req, "Unsupported method."));
    }
    try
    {
        return send(visitPreview(req));
    }
    catch(const std::exception& ex)
    {
        return send(utils::createJsonServerError(req, ex.what()));
    }
}
