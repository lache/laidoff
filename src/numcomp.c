#include "numcomp.h"
#define _USE_MATH_DEFINES // for C ... not standard
#include <math.h>
#include <string.h>

// v valid range : [-32.0f, 31.0f]
// compressed output: 11-bit (sign 1-bit, integral part 5-bit, fractional part 5-bit)
int v_comp_11bit(float v) {
	if (v > 31.0f) {
		return 2048 - 1;
	}
	else if (v < -32.0f) {
		return 0;
	}
	v += 32.0f;
	float v_int, v_frac;
	v_frac = modff(v, &v_int);
	int i = (int)v_int;
	int p = (int)(v_frac * 32);
	return ((i << 5) + p) & (2048 - 1);
}

float v_decomp_11bit(int c) {
	int i = c >> 5;
	int p = c & (32 - 1);
	return (i + p / 32.0f) - 32.0f;
}

// should len(q)=1
// 4-float quaternion -> 29-bit
int quat_comp(const float* q0) {
	float q[4];
	int all_minus = 1;
	for (int i = 0; i < 4; i++) {
		if (q0[i] > 0) {
			all_minus = 0;
			break;
		}
	}
	if (all_minus) {
		q[0] = -q0[0];
		q[1] = -q0[1];
		q[2] = -q0[2];
		q[3] = -q0[3];
	}
	else {
		memcpy(q, q0, sizeof(float) * 4);
	}
	// absmax_idx  smallest three components
	// [  2-bit  ][  9-bit  ][  9-bit  ][  9-bit  ]
	int c = 0;
	// Find abs max component index
	int absmax_idx = 0;
	float absmax = q[0];
	for (int i = 1; i < 4; i++) {
		if (absmax < q[i]) {
			absmax = q[i];
			absmax_idx = i;
		}
	}

	float m = 1.0f / sqrtf(2);
	int shift = 0;
	for (int i = 0; i < 4; i++) {
		if (i == absmax_idx) {
			continue;
		}
		c |= (int)fabs(q[i] / m * (256-1)) << shift;
		shift += 9;
		if (q[i] < 0) {
			c |= 1 << (shift - 1);
		}
	}
	// Set max index (2-bit)
	c |= absmax_idx << (9 * 3);
	return c & 536870911; // (2^29 - 1) - max 29-bit value
}

void quat_decomp(float* q, int c) {
	int absmax_idx = (c >> (9 * 3)) & 3;
	int shift = 0;
	float m = 1.0f / sqrtf(2);
	float qsum = 0;
	for (int i = 0; i < 4; i++) {
		if (i == absmax_idx) {
			continue;
		}

		q[i] = m * ((c >> shift) & (256 - 1)) / (256.0f - 1.0f);
		shift += 9;
		if ((c >> (shift - 1)) & 1) {
			q[i] *= -1;
		}
		qsum += q[i] * q[i];
	}
	q[absmax_idx] = sqrtf(1.0f - qsum);
}

void to_quat(float* q, float pitch, float roll, float yaw) {
	float cy = cosf(yaw * 0.5f);
	float sy = sinf(yaw * 0.5f);
	float cr = cosf(roll * 0.5f);
	float sr = sinf(roll * 0.5f);
	float cp = cosf(pitch * 0.5f);
	float sp = sinf(pitch * 0.5f);
	q[0] = cy * cr * cp + sy * sr * sp;
	q[1] = cy * sr * cp - sy * cr * sp;
	q[2] = cy * cr * sp + sy * sr * cp;
	q[3] = sy * cr * cp - cy * sr * sp;
}

void to_euler(const float* q, float* pitch, float* roll, float* yaw) {
	// roll (x-axis rotation)
	float sinr = +2.0f * (q[0] * q[1] + q[2] * q[3]);
	float cosr = +1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]);
	*roll = atan2f(sinr, cosr);

	// pitch (y-axis rotation)
	float sinp = +2.0f * (q[0] * q[2] - q[3] * q[1]);
	if (fabsf(sinp) >= 1.0f)
		*pitch = copysignf((float)M_PI / 2.0f, sinp); // use 90 degrees if out of range
	else
		*pitch = asinf(sinp);

	// yaw (z-axis rotation)
	float siny = +2.0f * (q[0] * q[3] + q[1] * q[2]);
	float cosy = +1.0f - 2.0f * (q[2] * q[2] + q[3] * q[3]);
	*yaw = atan2f(siny, cosy);
}

/* change to `float/fmodf` or `long float/fmodl` or `int/%` as appropriate */

/* wrap x -> [0,max) */
float wrap_max(float x, float max) {
	/* integer math: `(max + x % max) % max` */
	return fmodf(max + fmodf(x, max), max);
}

/* wrap x -> [min,max) */
float wrap_min_max(float x, float min, float max) {
	return min + wrap_max(x - min, max - min);
}

float wrap_radian(float r) {
	return wrap_min_max(r, (float)-M_PI, (float)M_PI);
}

void test_quat_comp(float pitch, float roll, float yaw) {
	pitch = wrap_radian(pitch);
	roll = wrap_radian(roll);
	yaw = wrap_radian(yaw);

	float q1[4];
	to_quat(q1, pitch, roll, yaw);
	int q1c = quat_comp(q1);
	float q1dc[4];
	quat_decomp(q1dc, q1c);
	float pitchd, rolld, yawd;
	to_euler(q1dc, &pitchd, &rolld, &yawd);
}

int numcomp_test() {
	float v1 = v_decomp_11bit(v_comp_11bit(0));
	float v2 = v_decomp_11bit(v_comp_11bit(32.0f));
	float v3 = v_decomp_11bit(v_comp_11bit(-32.0f));
	float v4 = v_decomp_11bit(v_comp_11bit(12.34f));
	float v5 = v_decomp_11bit(v_comp_11bit(-12.34f));
	float v6 = v_decomp_11bit(v_comp_11bit(3.0f));
	float v7 = v_decomp_11bit(v_comp_11bit(0.87654f));
	float v8 = v_decomp_11bit(v_comp_11bit(999.0f));
	float v9 = v_decomp_11bit(v_comp_11bit(-999.0f));
	float v10 = v_decomp_11bit(v_comp_11bit(31.5f));
	float v11 = v_decomp_11bit(v_comp_11bit(-31.9f));

	test_quat_comp(0, 0, 0);
	test_quat_comp(1.0f, 0, 0);
	test_quat_comp(0, 1.0f, 0);
	test_quat_comp(0, 0, 1.0f);
	test_quat_comp(1.0f, 2.0f, 3.0f);
	test_quat_comp(10.0f, -20.0f, 30.0f);
	test_quat_comp(-10.0f, -20.0f, 30.0f);
	test_quat_comp(1.234f, -0.051f, 2.99f);
    return 0;
}
