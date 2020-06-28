#ifndef PICTURE_PREVIEW_HANDLER_HPP
#define PICTURE_PREVIEW_HANDLER_HPP

#include "HttpSession.hpp"
#include "PostPicture.hpp"
#include <boost/uuid/uuid.hpp>
#include <list>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class PicturePreviewHandler : public std::enable_shared_from_this<PicturePreviewHandler>
{
public:
    class PictureView
    {
    public:
        PictureView();
        explicit PictureView(std::string);
        explicit PictureView(const PostPicture&);
        PictureView(const PictureView&) = delete;
        PictureView& operator=(const PictureView&) = delete;
        PictureView(PictureView&&) noexcept = default;
        PictureView& operator=(PictureView&&) noexcept = default;
        ~PictureView() = default;

        const std::string& getData() const;

    private:
        std::string data;
    };

    class Pictures
    {
    public:
        Pictures(std::string, time_t, std::vector<PictureView>&&);

        std::string uuid() const;

        time_t timestamp() const;

        size_t numOfTracks() const;

        std::shared_ptr<PictureView> snap(size_t) const;
        std::shared_ptr<PictureView> track(size_t) const;

        std::vector<std::string> getSnapUrls() const;

        std::vector<std::string> getTrackUrls() const;

        int getSnapMask() const;

    private:
        const size_t snapsLen{5};
        std::string uuid_;
        time_t timestamp_;
        std::vector<std::shared_ptr<PictureView>> snaps;
        std::vector<std::shared_ptr<PictureView>> tracks;
    };

    using Request = boost::beast::http::request<boost::beast::http::string_body>;
    using Response = boost::beast::http::response<boost::beast::http::string_body>;

    explicit PicturePreviewHandler(size_t);

    void createPreview(std::vector<PictureView>&& pictureViews);

    nlohmann::json listPreview(const std::string&, size_t);

    Response visitPreview(const Request&);

    void handleRequest(Request&&, HttpSession::Queue&);

private:
    using PictureIds = std::list<std::string>;
    using PictureBlocks = std::unordered_map<std::string, Pictures>;

    size_t cacheLimit;
    std::mutex mutex;
    PictureIds ids;
    PictureBlocks pictures;
};

#endif // PICTURE_PREVIEW_HANDLER_HPP
