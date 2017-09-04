#include "lwbutton.h"
#include "lwmacro.h"
#include "lwlog.h"

void lwbutton_append(LWBUTTONLIST* button_list, const char* id, float x, float y, float w, float h) {
	if (button_list->button_count >= ARRAY_SIZE(button_list->button)) {
		LOGE(LWLOGPOS "ARRAY_SIZE(button_list->button) exceeded");
		return;
	}
	LWBUTTON* b = &button_list->button[button_list->button_count];
	if (strlen(id) > ARRAY_SIZE(b->id) - 1) {
		LOGE(LWLOGPOS "ARRAY_SIZE(b->id) exceeded");
		return;
	}
	strcpy(b->id, id);
	b->x = x;
	b->y = y;
	b->w = w;
	b->h = h;
	button_list->button_count++;
}

int lwbutton_press(const LWBUTTONLIST* button_list, float x, float y) {
	if (button_list->button_count >= ARRAY_SIZE(button_list->button)) {
		LOGE(LWLOGPOS "ARRAY_SIZE(button_list->button) exceeded");
		return -1;
	}
	for (int i = 0; i < button_list->button_count; i++) {
		const LWBUTTON* b = &button_list->button[i];
		if (b->x <= x && x <= b->x + b->w && b->y - b->h <= y && y <= b->y) {
			return i;
		}
	}
	return -1;
}

const char* lwbutton_id(const LWBUTTONLIST* button_list, int idx) {
	if (idx >= ARRAY_SIZE(button_list->button)) {
		LOGE(LWLOGPOS "ARRAY_SIZE(button_list->button) exceeded");
		return 0;
	}
	return button_list->button[idx].id;
}
