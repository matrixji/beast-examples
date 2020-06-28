#ifndef DATA_SERIALIZE_HPP
#define DATA_SERIALIZE_HPP

#include "democlient.h"
#include <nlohmann/json_fwd.hpp>

void to_json(nlohmann::json&, const PostPicture&);

void from_json(const nlohmann::json&, PostPicture&);

bool operator==(const PostPicture& a, const PostPicture& b);

bool operator!=(const PostPicture& a, const PostPicture& b);

PostPicture* createPicture();

void freePicture(PostPicture*);

using Picture = std::unique_ptr<PostPicture, decltype(freePicture)*>;

class PostPictureWrapper
{
public:
    PostPictureWrapper();

    explicit PostPictureWrapper(PostPicture*);

    PostPictureWrapper(const PostPictureWrapper&) = delete;

    PostPictureWrapper(PostPictureWrapper&&) = default;

    PostPictureWrapper& operator=(const PostPictureWrapper&) = delete;

    PostPictureWrapper& operator=(PostPictureWrapper&&) = default;

    ~PostPictureWrapper() = default;

    PostPicture& picture();

    time_t timestamp() const;

private:
    Picture picture_{nullptr, freePicture};
    time_t timestamp_;
};

#endif // DATA_SERIALIZE_HPP
