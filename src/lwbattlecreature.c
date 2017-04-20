#include "lwbattlecreature.h"
#include "lwmacro.h"
#include "lwcontext.h"

void update_player(const struct _LWCONTEXT* pLwc, int slot_index, LWBATTLECREATURE* player) {
	player->shake_duration = (float)LWMAX(0, player->shake_duration - (float)pLwc->delta_time);
}
