#pragma once

#ifdef __cplusplus
extern "C" {;
#endif
char * create_string_from_file(const char * filename);

void release_string(char * d);

char* create_binary_from_file(const char* filename, size_t* size);

void release_binary(char * d);

#ifdef __cplusplus
};
#endif
