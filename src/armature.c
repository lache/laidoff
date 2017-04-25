#include <stdlib.h>
#include "armature.h"
#include "file.h"

int load_armature(const char* filename, LWARMATURE* ar) {
	size_t s;
	char* c = create_binary_from_file(filename, &s);
	int bone_count = (int)(s / sizeof(mat4x4));
	ar->count = bone_count;
	ar->mat = (mat4x4*)c;
	return 0;
}

void unload_armature(LWARMATURE* ar) {
	ar->count = 0;
	free(ar->mat);
	ar->mat = 0;
}
