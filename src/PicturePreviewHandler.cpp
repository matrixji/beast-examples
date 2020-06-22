#include "PicturePreviewHandler.hpp"
#include "Utils.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <ctime>

PicturePreviewHandler::Pictures::Pictures(std::string uuid, time_t timestamp,
                                          std::vector<PictureView>&& pvs)
: uuid_{std::move(uuid)}, timestamp_{timestamp}
{
    size_t index = 0;
    for(auto& pv : pvs)
    {
        if(index < snapsLen)
        {
            snaps.emplace_back(std::make_shared<PictureView>(std::move(pv)));
        }
        else
        {
            tracks.emplace_back(std::make_shared<PictureView>(std::move(pv)));
        }
        ++index;
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
PicturePreviewHandler::Pictures::snap(size_t index) const
{
    try
    {
        return snaps.at(index);
    }
    catch(std::out_of_range&)
    {
        return std::make_shared<PictureView>(std::string{});
    }
}

std::shared_ptr<PicturePreviewHandler::PictureView>
PicturePreviewHandler::Pictures::track(size_t index) const
{
    try
    {
        return tracks.at(index);
    }
    catch(std::out_of_range&)
    {
        return std::make_shared<PictureView>(std::string{});
    }
}
size_t PicturePreviewHandler::Pictures::numOfTracks() const
{
    return tracks.size();
}

int PicturePreviewHandler::Pictures::getSnapMask() const
{
    int mask = 0;
    int currMask = 1;
    constexpr unsigned int numOfPics{5};
    for(size_t i = 0; i < numOfPics; i++)
    {
        auto pic = snap(i);
        if(pic->getData().size() > 0)
        {
            mask |= currMask;
        }
        currMask <<= 1;
    }
    return mask;
}
std::vector<std::string> PicturePreviewHandler::Pictures::getTrackUrls() const
{
    std::vector<std::string> ret;
    for(size_t i = 0; i < tracks.size(); ++i)
    {
        ret.emplace_back(std::string("/api/v1/realtime/preview/picture/") +
                         uuid_ + "/tracks/" + std::to_string(i));
    }
    return ret;
}
std::vector<std::string> PicturePreviewHandler::Pictures::getSnapUrls() const
{
    std::vector<std::string> ret;
    for(size_t i = 0; i < snaps.size(); ++i)
    {
        if(snaps.at(i)->getData().size() > 0)
        {
            ret.emplace_back(std::string("/api/v1/realtime/preview/picture/") +
                             uuid_ + "/snaps/" + std::to_string(i));
        }
    }
    return ret;
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
    json = nlohmann::json{
        {"uuid", pvs.uuid()},
        {"timestamp", pvs.timestamp()},
        {"snaps", {{"mask", pvs.getSnapMask()}, {"pictures", pvs.getSnapUrls()}}},
        {"tracks", {{"num", pvs.numOfTracks()}, {"pictures", pvs.getTrackUrls()}}}};
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

PicturePreviewHandler::Response PicturePreviewHandler::visitPreview(const Request& req)
{
    using boost::beast::http::field;
    using boost::beast::http::status;

    auto getPicture = [this](const std::string& uuid, const std::string& name, size_t index) {
        std::unique_lock<std::mutex> lock(mutex);
        if(name == "snaps")
        {
            return pictures.at(uuid).snap(index);
        }
        else if(name == "tracks")
        {
            return pictures.at(uuid).track(index);
        }
        throw std::out_of_range({});
    };

    std::vector<std::string> paths;
    std::string uri = req.target().to_string();
    boost::split(paths, uri, boost::is_any_of("/"), boost::token_compress_on);
    size_t index{0};
    boost::conversion::try_lexical_convert(paths.at(paths.size() - 1), index);
    std::string& name = paths.at(paths.size() - 2);
    std::string& uuid = paths.at(paths.size() - 3);

    try
    {
        auto picture = getPicture(uuid, name, index);
        Response res{status::ok, req.version()};
        res.set(field::server, utils::getServerSignature());
        res.set(field::content_type, "image/jpeg");
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

void PicturePreviewHandler::handleRequest(Request&& req, HttpSession::Queue& send)
{
    if(req.method() != boost::beast::http::verb::get)
    {
        return send(req, utils::createJsonBadRequest(req, "Unsupported method."));
    }
    try
    {
        return send(req, visitPreview(req));
    }
    catch(const std::exception& ex)
    {
        return send(req, utils::createJsonServerError(req, ex.what()));
    }
}
