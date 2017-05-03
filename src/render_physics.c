#include "lwcontext.h"
#include "render_physics.h"
#include "render_solid.h"
#include "laidoff.h"
#include "lwlog.h"

void lwc_render_physics(const struct _LWCONTEXT* pLwc) {
	glViewport(0, 0, pLwc->width, pLwc->height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}
