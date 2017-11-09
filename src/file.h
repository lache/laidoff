#pragma once

#ifdef __cplusplus
extern "C" {;
#endif

typedef struct _LWUNIQUEID LWUNIQUEID;

char * create_string_from_file(const char * filename);
void release_string(char * d);
char* create_binary_from_file(const char* filename, size_t* size);
void release_binary(char * d);
int get_cached_user_id(const char* path_prefix, LWUNIQUEID* id);
int save_cached_user_id(const char* path_prefix, const LWUNIQUEID* id);
#ifdef __cplusplus
};
#endif
