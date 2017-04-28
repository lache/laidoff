#include <stdlib.h>
#include "armature.h"
#include "file.h"
#include <string.h>

int load_armature(const char* filename, LWARMATURE* ar) {
	size_t s;
	char* c = create_binary_from_file(filename, &s);
	ar->count = *(int*)c;
	ar->mat = (mat4x4*)(c + sizeof(int));
	ar->parent_index = (int*)(c + sizeof(int) + sizeof(mat4x4) * ar->count);
	ar->use_connect = (int*)(c + sizeof(int) + sizeof(mat4x4) * ar->count + sizeof(int) * ar->count);
	ar->d = c;
	return 0;
}

void unload_armature(LWARMATURE* ar) {
	free(ar->d);
	memset(ar, 0, sizeof(LWARMATURE));
}
