#ifndef PICTURE_PREVIEW_HANDLER_HPP
#define PICTURE_PREVIEW_HANDLER_HPP

#include "HttpSession.hpp"
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
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
        explicit PictureView(std::string);
        PictureView() = delete;
        PictureView(const PictureView&) = delete;
        PictureView& operator=(const PictureView&) = delete;
        PictureView(PictureView&&) noexcept;
        PictureView& operator=(PictureView&&) noexcept;
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

        std::shared_ptr<PictureView> picture(size_t) const;

    private:
        std::string uuid_;
        time_t timestamp_;
        std::vector<std::shared_ptr<PictureView>> pictures;
    };

    using Response = boost::beast::http::response<boost::beast::http::string_body>;

    explicit PicturePreviewHandler(size_t);

    nlohmann::json createPreview(std::vector<PictureView>&& pictureViews);

    nlohmann::json listPreview(const std::string&, size_t);

    Response visitPreview(const HttpSession::Request&);

    void handleRequest(HttpSession::Request&&, HttpSession::Queue&);

private:
    using PictureIds = std::list<std::string>;
    using PictureBlocks = std::unordered_map<std::string, Pictures>;

    size_t cacheLimit;
    std::mutex mutex;
    PictureIds ids;
    PictureBlocks pictures;
};

#endif // PICTURE_PREVIEW_HANDLER_HPP
