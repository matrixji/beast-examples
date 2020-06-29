
#ifndef PICTURE_CACHE_HPP
#define PICTURE_CACHE_HPP

#include "PicturePreviewHandler.hpp"
#include "PostPicture.hpp"
#include <memory>

class PictureCache : public std::enable_shared_from_this<PictureCache>
{
public:
    explicit PictureCache(std::shared_ptr<PicturePreviewHandler>);

    void addPicture(PostPictureWrapper&&);

private:
    const static int timeLimit = 10;
    const static int cacheLines = 4;
    using BodyPictureCacheLine = std::array<std::vector<PostPictureWrapper>, cacheLines>;
    using BodyPictureCacheCollection =
        std::unordered_map<unsigned int, BodyPictureCacheLine>;
    using FacePictureCacheCollection = std::unordered_map<unsigned int, PostPictureWrapper>;
    std::shared_ptr<PicturePreviewHandler> handler;
    std::mutex mutexForCaches;
    BodyPictureCacheCollection bodyCaches;
    FacePictureCacheCollection faceCaches;

    void process();

    void cleanup();
};

#endif // PICTURE_CACHE_HPP
