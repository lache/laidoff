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
    // Original center x coordinate (screen coordinate)
    float origin_x;
    // Original center y coordinate (screen coordinate)
    float origin_y;
} LWDIRPAD;

void reset_dir_pad_position(LWDIRPAD* dir_pad);
int lw_get_normalized_dir_pad_input(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad, float *dx, float *dy, float *dlen);
void get_right_dir_pad_original_center(const float aspect_ratio, float *x, float *y);
void get_left_dir_pad_original_center(const float aspect_ratio, float *x, float *y);
float get_dir_pad_size_radius();
int dir_pad_press(LWDIRPAD* dir_pad, float x, float y, int pointer_id,
                  float dir_pad_center_x, float dir_pad_center_y, float sr);
void dir_pad_move(LWDIRPAD* dir_pad, float x, float y, int pointer_id,
                  float dir_pad_center_x, float dir_pad_center_y, float sr);
int dir_pad_release(LWDIRPAD* dir_pad, int pointer_id);

void render_dir_pad(const LWCONTEXT* pLwc, float x, float y);
void render_dir_pad_with_start(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad);
