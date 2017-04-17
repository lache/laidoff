#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <errno.h>
#include "file.h"

#define  LOG_TAG    "And9"

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

typedef struct _NativeAsset
{
    unsigned long asset_hash;
    int start_offset;
    int length;
} NativeAsset;

static NativeAsset nativeAssetArray[512];
static int nativeAssetLength;
static char apkPath[2048];

unsigned long hash(unsigned char *str);

void set_apk_path(const char* apk_path)
{
    strcpy(apkPath, apk_path);
}

void release_string(const char* d)
{
    free((void*)d);
}

char* create_asset_file(const char* filename, size_t* size, int binary)
{
    unsigned long asset_hash = hash(filename);

    for (int i = 0; i < nativeAssetLength; i++)
    {
        if (nativeAssetArray[i].asset_hash == asset_hash)
        {
            FILE* f = fopen(apkPath, binary ? "rb" : "r");

            char* p = malloc(nativeAssetArray[i].length + (binary ? 0 : 1));

            int fseekResult = fseek(f, nativeAssetArray[i].start_offset, SEEK_SET);

            int freadResult = fread(p, nativeAssetArray[i].length, 1, f);

            if (!binary)
            {
                p[nativeAssetArray[i].length] = '\0'; // null termination for string data needed
            }

            LOGI("create_asset_file: %s size: %d, fseekResult: %d, errno: %d, binary: %d, test: %s",
                 filename,
                 nativeAssetArray[i].length,
                 fseekResult,
                 errno,
                 binary,
                 p
            );

            fclose(f);
            *size = nativeAssetArray[i].length;
            return p;
        }
    }

    *size = 0;
    return 0;
}

char* create_binary_from_file(const char* filename, size_t* size)
{
    return create_asset_file(filename, size, 1);
}

char* create_string_from_file(const char* filename)
{
    size_t size;
    return create_asset_file(filename, &size, 0);
}

void release_binary(char* d)
{
    free((void*)d);
}

void register_asset(const char* asset_path, int start_offset, int length) {

    nativeAssetArray[nativeAssetLength].asset_hash = hash((unsigned char *)asset_path);
    nativeAssetArray[nativeAssetLength].start_offset = start_offset;
    nativeAssetArray[nativeAssetLength].length = length;

    LOGI("Asset #%d (hash=%lu) registered: %s, start_offset=%d, length=%d",
         nativeAssetLength, nativeAssetArray[nativeAssetLength].asset_hash, asset_path, start_offset, length);

    nativeAssetLength++;
}
