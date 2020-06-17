#ifndef DEMO_CLIENT_H
#define DEMO_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

    // post structure for picture.
    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct PostPicture
    {
        // image width, optional.
        unsigned int width;

        // image height. optional
        unsigned int height;

        // image size, 0 to identify no image
        unsigned int size;

        // image data pointer. nullptr is no image
        char* data;
    } PostPicture;

    // post picture to server
    // arguments:
    //  pictures: start address of PostPicture[]
    //  length: length of PostPicture[], must be 5
    //
    // return: 0 for success, others for error.
    int postPictures(const PostPicture* pictures, unsigned int length);

#ifdef __cplusplus
};
#endif

#endif // DEMO_CLIENT_H
