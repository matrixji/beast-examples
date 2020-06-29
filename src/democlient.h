#ifndef DEMO_CLIENT_H
#define DEMO_CLIENT_H

#include <stdint.h> // NOLINT(modernize-deprecated-headers)

#ifdef __cplusplus
extern "C"
{
#endif

    // NOLINTNEXTLINE(modernize-use-using)
    typedef enum PictureObjectType
    {
        body = 1,
        face = 2
    } PictureObjectType;

    // post structure for snap.
    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct PostPicture
    {
        // camera id
        unsigned int cameraId;

        // object id
        unsigned int objectId;

        // objectType
        PictureObjectType objectType;

        // 64bit timestamp in milliseconds
        uint64_t timestamp;

        // image width, optional(could be 0 for unknown).
        unsigned int width;

        // image height. optional(could be 0 for unknown)
        unsigned int height;

        // image size, 0 to identify no image
        unsigned int size;

        // image data pointer. nullptr is no image
        char* data;
    } PostPicture;

    // post snap to server
    // arguments:
    //  picture: the picture to post
    // return: 0 for success, others for error.
    int postPictures(const PostPicture* picture);

#ifdef __cplusplus
};
#endif

#endif // DEMO_CLIENT_H
