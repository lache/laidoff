#pragma once

#if defined __cplusplus
extern "C" {;
#endif  /* defined __cplusplus */

#define MAX_PATHQUERY_SMOOTH (2048)

typedef struct _LWPATHQUERY {
	float spos[3];
	float epos[3];
	float smooth_path[MAX_PATHQUERY_SMOOTH * 3];
	int n_smooth_path;
} LWPATHQUERY;

void* load_nav(const char* filename);
void unload_nav(void* n);
void nav_query(void* n, LWPATHQUERY* q);

void set_random_start_end_pos(void* n, LWPATHQUERY* q);

#if defined __cplusplus
}
#endif  /* defined __cplusplus */
