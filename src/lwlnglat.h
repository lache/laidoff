#pragma once

#define MAX_VISIBILITY_ENTRY_COUNT (32)
#define MAX_VISIBILITY_ENTRY_NAME_LENGTH (32)
static char visibility[MAX_VISIBILITY_ENTRY_COUNT][MAX_VISIBILITY_ENTRY_NAME_LENGTH];

#define LNGLAT_RES_WIDTH (172824)
#define LNGLAT_RES_HEIGHT (86412)
const static float sea_render_scale = 300.0f;
// extant decribed in R-tree pixel(cell) unit
#define LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS (32.0f)
#define LNGLAT_SEA_PING_EXTENT_IN_DEGREES ((180.0f/LNGLAT_RES_HEIGHT)*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS)

static float cell_x_to_lng(int x) {
    return -180.0f + x / (LNGLAT_RES_WIDTH / 2.0f) * 180.0f;
}

static float cell_y_to_lat(int x) {
    return 90.0f - x / (LNGLAT_RES_HEIGHT / 2.0f) * 90.0f;
}

static float cell_fx_to_lng(float fx) {
    return -180.0f + fx / (LNGLAT_RES_WIDTH / 2.0f) * 180.0f;
}

static float cell_fy_to_lat(float fx) {
    return 90.0f - fx / (LNGLAT_RES_HEIGHT / 2.0f) * 90.0f;
}

static float lng_to_render_coords(float lng, const LWTTLLNGLAT* center) {
    return (lng - center->lng) * sea_render_scale;
}

static float lat_to_render_coords(float lat, const LWTTLLNGLAT* center) {
    return (lat - center->lat) * sea_render_scale;
}

static float cell_x_to_render_coords(int x, const LWTTLLNGLAT* center) {
    return lng_to_render_coords(cell_x_to_lng(x), center);
}

static float cell_y_to_render_coords(int y, const LWTTLLNGLAT* center) {
    return lat_to_render_coords(cell_y_to_lat(y), center);
}

static float cell_fx_to_render_coords(float fx, const LWTTLLNGLAT* center) {
    return (cell_fx_to_lng(fx) - center->lng) * sea_render_scale;
}

static float cell_fy_to_render_coords(float fy, const LWTTLLNGLAT* center) {
    return (cell_fy_to_lat(fy) - center->lat) * sea_render_scale;
}
