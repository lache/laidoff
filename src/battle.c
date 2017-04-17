#include "lwcontext.h"
#include "lwmacro.h"

void update_battle(struct _LWCONTEXT* pLwc) {

	const float screen_aspect_ratio = (float)pLwc->width / pLwc->height;

	mat4x4_perspective(pLwc->battle_proj, (float)(LWDEG2RAD(pLwc->battle_fov_deg) / screen_aspect_ratio), screen_aspect_ratio, 0.1f, 1000.0f);

	vec3 eye = { 0, -5.0f, 0.9f };
	vec3 center = { pLwc->battle_cam_center_x, 0, 0.3f };
	vec3 up = { 0, 0, 1 };
	mat4x4_look_at(pLwc->battle_view, eye, center, up);

	pLwc->command_in_progress_anim.t = (float)LWMAX(0, pLwc->command_in_progress_anim.t - (float)pLwc->delta_time);
}
