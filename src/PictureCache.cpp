#include "PictureCache.hpp"
#include <algorithm>

PictureCache::PictureCache(std::shared_ptr<PicturePreviewHandler> handler)
: handler{std::move(handler)}
{
}

void PictureCache::cleanup()
{
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    auto isExpired = [now](const PostPictureWrapper& picture) {
        return now - picture.timestamp() > timeLimit;
    };

    auto removePictureExpired = [&isExpired](std::vector<PostPictureWrapper>& pictures) {
        pictures.erase(std::remove_if(pictures.begin(), pictures.end(), isExpired),
                       pictures.end());
    };

    // remove expired in lines
    auto removePictureExpiredInLines = [removePictureExpired](BodyPictureCacheLine& lines) {
        std::for_each(lines.begin(), lines.end(), removePictureExpired);
    };

    auto allLinesEmpty = [](const BodyPictureCacheLine& lines) -> bool {
        return std::all_of(lines.begin(), lines.end(),
                           [](const std::vector<PostPictureWrapper>& pictures) {
                               return pictures.empty();
                           });
    };

    auto removePictures = [removePictureExpiredInLines, allLinesEmpty, isExpired, this]() {
        // remove expired for all object-id
        std::for_each(bodyCaches.begin(), bodyCaches.end(),
                      [removePictureExpiredInLines](BodyPictureCacheCollection::value_type& pair) {
                          removePictureExpiredInLines(pair.second);
                      });
        // remove pair if all lines empty
        auto iterBodyCaches = bodyCaches.begin();
        while(iterBodyCaches != bodyCaches.end())
        {
            if(allLinesEmpty(iterBodyCaches->second))
            {
                iterBodyCaches = bodyCaches.erase(iterBodyCaches);
            }
            else
            {
                ++iterBodyCaches;
            }
        }

        // remove expired for face
        auto iterFaceCaches = faceCaches.begin();
        while(iterFaceCaches != faceCaches.end())
        {
            if(isExpired(iterFaceCaches->second))
            {
                iterFaceCaches = faceCaches.erase(iterFaceCaches);
            }
            else
            {
                ++iterFaceCaches;
            }
        }
    };

    std::unique_lock<std::mutex> lock(mutexForCaches);
    removePictures();
}

void PictureCache::addPicture(PostPictureWrapper&& picture)
{
    auto objectId = picture.picture().objectId;
    auto objectType = picture.picture().objectType;
    cleanup();
    // add new pic
    {
        std::unique_lock<std::mutex> lock(mutexForCaches);
        if(objectType == PictureObjectType::face)
        {
            // add face cache
            faceCaches[objectId] = std::move(picture);
        }
        else
        {
            // add body cache
            auto lines = bodyCaches.emplace(objectId, BodyPictureCacheLine{}).first;
            lines->second.at(picture.picture().cameraId).emplace_back(std::move(picture));
        }
    }
    process();
}

void PictureCache::process()
{
    using Pvs = std::vector<PicturePreviewHandler::PictureView>;

    auto hasEmpty = [](const BodyPictureCacheLine& lines) {
        return std::any_of(lines.begin(), lines.end(),
                           [](const std::vector<PostPictureWrapper>& pics) {
                               return pics.empty();
                           });
    };

    auto tryPostWithObject = [hasEmpty, this](const unsigned int objectId, Pvs& pvs) {
        auto& lines = bodyCaches.at(objectId);
        if(!hasEmpty(lines))
        {
            for(auto& line : lines)
            {
                if(line.size() >= 6)
                {
                    pvs.emplace_back(faceCaches.at(objectId).picture());
                    pvs.emplace_back(bodyCaches.at(objectId).at(0).at(0).picture());
                    pvs.emplace_back(bodyCaches.at(objectId).at(1).at(0).picture());
                    pvs.emplace_back(bodyCaches.at(objectId).at(2).at(0).picture());
                    pvs.emplace_back(bodyCaches.at(objectId).at(3).at(0).picture());
                    pvs.emplace_back(line.at(0).picture());
                    pvs.emplace_back(line.at(1).picture());
                    pvs.emplace_back(line.at(2).picture());
                    pvs.emplace_back(line.at(3).picture());
                    pvs.emplace_back(line.at(4).picture());
                    pvs.emplace_back(line.at(5).picture());
                }
            }
        }
        if(!pvs.empty())
        {
            faceCaches.erase(objectId);
            bodyCaches.erase(objectId);
        }
    };

    Pvs pvs;
    {
        std::unique_lock<std::mutex> lock(mutexForCaches);
        std::vector<unsigned int> ids;
        for(const auto& pair : faceCaches)
        {
            if(bodyCaches.find(pair.first) != bodyCaches.end())
            {
                ids.emplace_back(pair.first);
            }
        }
        for(const auto id : ids)
        {
            tryPostWithObject(id, pvs);
        }
    }
    if(not pvs.empty())
    {
        handler->createPreview(std::move(pvs));
    }
}
