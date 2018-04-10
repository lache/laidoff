#pragma once

void* remtex_new(const char* host);
void remtex_preload(void* r, const char* name);
GLuint remtex_load_tex(void* r, const char* name);
void remtex_destroy_render(void* r);
void remtex_destroy(void** r);
void remtex_update(void* r);
void remtex_render(void* r);
void remtex_udp_update(void* r);
void remtex_loading_str(void* r, char* str, size_t max_len);