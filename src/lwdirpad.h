#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWDIRPAD {
    // 1 if dir pad is dragged, 0 if otherwise
    int dragging;
    // dir_pad_dragging pointer index
    int pointer_id;
    // Current dir pad x coordinate (screen coordinate)
    float x;
    // Current dir pad y coordinate (screen coordinate)
    float y;
    // Touch start x coordinate (screen coordinate)
    float start_x;
    // Touch start y coordinate (screen coordinate)
    float start_y;
    // Initial origin x coordinate (screen coordinate)
    float origin_x;
    // Initial origin y coordinate (screen coordinate)
    float origin_y;
    // Touch area width (screen coordinate)
    float size_x;
    // Touch area height (screen coordinate)
    float size_y;
} LWDIRPAD;

void render_dir_pad(const LWCONTEXT* pLwc, float x, float y);
void render_dir_pad_with_start(const LWCONTEXT* pLwc, float x, float y, float start_x, float start_y, int dragging);
