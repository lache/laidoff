#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
char* lw_get_text_input_for_writing();
const char* lw_get_text_input();
int lw_get_text_input_seq();
void lw_increase_text_input_seq();
int lw_text_input_max();
#ifdef __cplusplus
};
#endif
