#pragma once

int v_comp_11bit(float v);
float v_decomp_11bit(int c);
int quat_comp(const float* q0);
void quat_decomp(float* q, int c);
void to_quat(float* q, float pitch, float roll, float yaw);
void to_euler(const float* q, float* pitch, float* roll, float* yaw);
float wrap_max(float x, float max);
float wrap_min_max(float x, float min, float max);
float wrap_radian(float r);

