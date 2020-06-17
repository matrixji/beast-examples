#ifndef DEMOCLIENT_H
#define DEMOCLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

    // post structure for picture.
    struct PostPicture
    {
        // image width, optional.
        size_t width;

        // image height. optional
        size_t height;

        // image size, 0 to identify no image
        size_t size;

        // image data pointer. nullptr is no image
        char* data;
    };

    // post picture to server
    // arguments:
    //  pictures: start address of PostPicture[]
    void postPictures(const PostPicture* pictures);

#ifndef __cplusplus
};
#endif

#endif // DEMOCLIENT_H
