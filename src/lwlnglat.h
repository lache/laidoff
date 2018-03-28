#pragma once

#define MAX_VISIBILITY_ENTRY_COUNT (32)
#define MAX_VISIBILITY_ENTRY_NAME_LENGTH (32)
static char visibility[MAX_VISIBILITY_ENTRY_COUNT][MAX_VISIBILITY_ENTRY_NAME_LENGTH];

const static short res_width = (1 << 14); // 16384;
const static short res_height = (1 << 13); // 8192;

static float cell_x_to_lng(short x) {
    return -180.0f + x / (res_width / 2.0f) * 180.0f;
}

static float cell_y_to_lat(short x) {
    return 90.0f - x / (res_height / 2.0f) * 90.0f;
}

static float cell_fx_to_lng(float fx) {
    return -180.0f + fx / (res_width / 2.0f) * 180.0f;
}

static float cell_fy_to_lat(float fx) {
    return 90.0f - fx / (res_height / 2.0f) * 90.0f;
}

static const float render_scale = 100.0f;

static float cell_x_to_render_coords(short x, const LWTTLLNGLAT* center) {
    return (cell_x_to_lng(x) - center->lng) * render_scale;
}

static float cell_y_to_render_coords(short y, const LWTTLLNGLAT* center) {
    return (cell_y_to_lat(y) - center->lat) * render_scale;
}

static float cell_fx_to_render_coords(float fx, const LWTTLLNGLAT* center) {
    return (cell_fx_to_lng(fx) - center->lng) * render_scale;
}

static float cell_fy_to_render_coords(float fy, const LWTTLLNGLAT* center) {
    return (cell_fy_to_lat(fy) - center->lat) * render_scale;
}
