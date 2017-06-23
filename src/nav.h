#pragma once

#if defined __cplusplus
extern "C" {;
#endif  /* defined __cplusplus */

#define MAX_PATHQUERY_SMOOTH (2048)

typedef struct _LWPATHQUERY {
	// Start position
	float spos[3];
	// End position
	float epos[3];
	// Smooth path points
	float smooth_path[MAX_PATHQUERY_SMOOTH * 3];
	// Total smooth path point count
	int n_smooth_path;
	// Abstract time for test player movement
	float path_t;
} LWPATHQUERY;

void* load_nav(const char* filename);
void unload_nav(void* n);
void nav_query(void* n, LWPATHQUERY* q);

void set_random_start_end_pos(void* n, LWPATHQUERY* q);

#if defined __cplusplus
}
#endif  /* defined __cplusplus */
