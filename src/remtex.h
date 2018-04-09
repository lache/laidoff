#pragma once

void* remtex_new();
void remtex_load(void* r, const char* name);
void remtex_destroy(void** r);
void remtex_update(void* r);
void remtex_udp_update(void* r);
