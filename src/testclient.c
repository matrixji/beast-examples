#include "democlient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    int index = 0;
    if(argc == 1)
    {
        printf("usage: %s <jpeg-file>\n", argv[0]);
        exit(1);
    }
    const unsigned int nums = 5;
    PostPicture pics[nums];
    FILE* fp = NULL;

    fp = fopen(argv[1], "rb");
    if(fp == NULL)
    {
        printf("%s is not a valid file\n", argv[1]);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    for(index = 0; index < nums; ++index)
    {
        pics[index].width = 0;
        pics[index].height = 0;
        pics[index].size = fileSize;
        pics[index].data = malloc(fileSize);
        if(index > 0)
        {
            // NOLINTNEXTLINE
            memcpy(pics[index].data, pics[0].data, fileSize);
        }
        else
        {
            fread(pics[index].data, 1, fileSize, fp);
        }
    }

    int ret = postPictures(pics, nums);

    for(index = 0; index < nums; ++index)
    {
        free(pics[index].data);
    }
    return ret;
}