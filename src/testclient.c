#include "democlient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t readFromFile(const char* filepath, char** payload)
{
    FILE* fp = NULL;
    fp = fopen(filepath, "rb");
    if(fp == NULL)
    {
        printf("%s is not a valid file\n", filepath);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(fileSize <= 0)
    {
        fclose(fp);
        return 0;
    }

    *payload = malloc(fileSize);
    fread(*payload, 1, fileSize, fp);
    fclose(fp);
    return fileSize;
}

int main(int argc, char* argv[])
{
    int index = 0;
    if(argc < 6)
    {
        printf("usage: %s <face:body> <jpeg-file> <camera-id> <person-id> "
               "<timestamp>\n",
               argv[0]);
        exit(1);
    }
    PostPicture pic;
    pic.size = readFromFile(argv[2], &pic.data);
    pic.width = 0;
    pic.height = 0;
    pic.cameraId = atoi(argv[3]);
    pic.objectId = atoi(argv[4]);
    pic.timestamp = atoll(argv[5]);
    pic.objectType = (strcmp(argv[1], "face") == 0) ? face : body;
    int ret = postPictures(&pic);
    free(pic.data);
    return ret;
}