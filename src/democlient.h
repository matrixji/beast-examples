#ifndef DEMO_CLIENT_H
#define DEMO_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

    // post structure for snap.
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

    // post snap to server
    // arguments:
    //  snaps: start address of PostPicture[]
    //  numOfSnaps: length of snaps, must be 5
    //  tracks: start address of PostPicture[]
    //  numOfTracks: length of tracks
    //
    // return: 0 for success, others for error.
    int postPictures(const PostPicture* snaps, unsigned int numOfSnaps,
                     const PostPicture* tracks, unsigned int numOfTracks);

#ifdef __cplusplus
};
#endif

#endif // DEMO_CLIENT_H
