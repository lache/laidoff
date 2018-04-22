#pragma once

#define MAX_VISIBILITY_ENTRY_COUNT (32)
#define MAX_VISIBILITY_ENTRY_NAME_LENGTH (32)
static char visibility[MAX_VISIBILITY_ENTRY_COUNT][MAX_VISIBILITY_ENTRY_NAME_LENGTH];

#define LNGLAT_RES_WIDTH (172824)
#define LNGLAT_RES_HEIGHT (86412)
const static float sea_render_scale = 256.0f;
// extant decribed in R-tree pixel(cell) unit
#define LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS (32.0f)
#define LNGLAT_SEA_PING_EXTENT_IN_DEGREES ((180.0f/LNGLAT_RES_HEIGHT)*LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS)

static float cell_fx_to_lng(float fx) {
    return -180.0f + fx / (LNGLAT_RES_WIDTH / 2.0f) * 180.0f;
}

static float cell_fy_to_lat(float fy) {
    return 90.0f - fy / (LNGLAT_RES_HEIGHT / 2.0f) * 90.0f;
}

static float cell_x_to_lng(int x) {
    return cell_fx_to_lng((float)x);
}

static float cell_y_to_lat(int y) {
    return cell_fy_to_lat((float)y);
}

static float lng_to_render_coords(float lng, const LWTTLLNGLAT* center, int view_scale) {
    return (lng - center->lng) * sea_render_scale / view_scale;
}

static float lat_to_render_coords(float lat, const LWTTLLNGLAT* center, int view_scale) {
    return (lat - center->lat) * sea_render_scale / view_scale;
}

static float cell_x_to_render_coords(int x, const LWTTLLNGLAT* center, int view_scale) {
    return lng_to_render_coords(cell_x_to_lng(x), center, view_scale);
}

static float cell_y_to_render_coords(int y, const LWTTLLNGLAT* center, int view_scale) {
    return lat_to_render_coords(cell_y_to_lat(y), center, view_scale);
}

static float cell_fx_to_render_coords(float fx, const LWTTLLNGLAT* center, int view_scale) {
    return (cell_fx_to_lng(fx) - center->lng) * sea_render_scale / view_scale;
}

static float cell_fy_to_render_coords(float fy, const LWTTLLNGLAT* center, int view_scale) {
    return (cell_fy_to_lat(fy) - center->lat) * sea_render_scale / view_scale;
}
