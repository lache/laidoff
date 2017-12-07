#pragma once

#include "lwatlasenum.h"

#define LW_UI_IDENTIFIER_LENGTH (32)
#define LW_UI_BUTTON_LIST_SIZE (64)

typedef struct _LWBUTTON {
    char id[LW_UI_IDENTIFIER_LENGTH];
    float x, y, w, h;
    LW_ATLAS_ENUM lae;
    LW_ATLAS_ENUM lae_alpha;
    float ui_alpha;
} LWBUTTON;

typedef struct _LWBUTTONLIST {
    LWBUTTON button[LW_UI_BUTTON_LIST_SIZE];
    int button_count;
} LWBUTTONLIST;

LWBUTTON* lwbutton_append(LWBUTTONLIST* button_list, const char* id, float x, float y, float w, float h);
LWBUTTON* lwbutton_lae_append(LWBUTTONLIST* button_list, const char* id, float x, float y, float w, float h,
                              LW_ATLAS_ENUM lae, LW_ATLAS_ENUM lae_alpha, float ui_alpha);
int lwbutton_press(const LWBUTTONLIST* button_list, float x, float y);
const char* lwbutton_id(const LWBUTTONLIST* button_list, int idx);
