#include <stdio.h>
#include "lwlog.h"
#include "lwuniqueid.h"
#include "lwmacro.h"

void concat_path(char* path, const char* path1, const char* path2) {
	if (path1) {
		strcat(path, path1);
	}
	if (strlen(path) > 0) {
		if (strncmp(&path[strlen(path) - 1], PATH_SEPARATOR, 1) != 0) {
			strcat(path, PATH_SEPARATOR);
		}
	}
	strcat(path, path2);
}

int get_cached_user_id(const char* path_prefix, LWUNIQUEID* id) {
	FILE* f;
	char path[1024] = { 0, };
	concat_path(path, path_prefix, LW_USER_ID_CACHE_FILE);
	f = fopen(path, "rb");
	if (f == 0) {
		// no cached user id exists
		LOGI("No cached user id exists");
		return -1;
	}
	fseek(f, 0, SEEK_END);
	const int f_size = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	const int expected_size = sizeof(unsigned int) * 4;
	if (f_size != expected_size) {
		// no cached user id exists
		LOGI("Invalid cached user id size: %d (%d expected)", f_size, expected_size);
		return -2;
	}
	fread((void*)id, expected_size, 1, f);
	fclose(f);
	return 0;
}

int save_cached_user_id(const char* path_prefix, const LWUNIQUEID* id) {
	FILE* f;
	char path[1024] = { 0, };
	concat_path(path, path_prefix, LW_USER_ID_CACHE_FILE);
	f = fopen(path, "wb");
	if (f == 0) {
		// no cached user id exists
		LOGE("Cannot open user id file cache for writing...");
		return -1;
	}
	const int expected_size = sizeof(unsigned int) * 4;
	size_t elements_written = fwrite((const void*)id, expected_size, 1, f);
	if (elements_written != 1) {
		// no cached user id exists
		LOGE("fwrite bytes written failed!");
		return -2;
	}
	fclose(f);
	return 0;
}
