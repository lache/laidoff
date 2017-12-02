#include <math.h>
#include <string.h>
#include "numcomp.h"
#include "pcg_basic.h"
#include "lwlog.h"

void numcomp_init_float_preset(LWNUMCOMPFLOATPRESET* preset, int c, float m, float M) {
    preset->c = c;
    preset->m = m;
    preset->M = M;
    preset->l = M - m;
    preset->s = preset->l / powf(2.0f, c);
}

unsigned int numcomp_compress_float(float v, const LWNUMCOMPFLOATPRESET* preset) {
    if (v < preset->m) {
        v = preset->m;
    }
    if (v > preset->M) {
        v = preset->M;
    }
    float base = v - preset->m;
    return (unsigned int)((base + preset->s * 0.5f) / preset->s);
}

float numcomp_decompress_float(unsigned int v_comp, const LWNUMCOMPFLOATPRESET* preset) {
    return preset->m + v_comp * preset->s;
}

float numcomp_test_float(float v, const LWNUMCOMPFLOATPRESET* preset) {
    unsigned int v_comp = numcomp_compress_float(v, preset);
    float v_decomp = numcomp_decompress_float(v_comp, preset);
    return fabsf(v - v_decomp);
}

void numcomp_test_float_print(float v, const LWNUMCOMPFLOATPRESET* preset) {
    float err = numcomp_test_float(v, preset);
    LOGI("float:%.4f (err:%.6f)", v, err);
}

void numcomp_init_vec3_preset(LWNUMCOMPVEC3PRESET* preset,
                              int cx, float mx, float Mx,
                              int cy, float my, float My,
                              int cz, float mz, float Mz) {
    numcomp_init_float_preset(&preset->p[0], cx, mx, Mx);
    numcomp_init_float_preset(&preset->p[1], cy, my, My);
    numcomp_init_float_preset(&preset->p[2], cz, mz, Mz);
}

unsigned int numcomp_compress_vec3(const float v[3], const LWNUMCOMPVEC3PRESET* preset) {
    unsigned int v_comp_x = numcomp_compress_float(v[0], &preset->p[0]);
    unsigned int v_comp_y = numcomp_compress_float(v[1], &preset->p[1]);
    unsigned int v_comp_z = numcomp_compress_float(v[2], &preset->p[2]);
    return v_comp_x
           | (v_comp_y << (preset->p[0].c))
           | (v_comp_z << (preset->p[0].c + preset->p[1].c));
}

void numcomp_decompress_vec3(float v[3], unsigned int v_comp, const LWNUMCOMPVEC3PRESET* preset) {
    unsigned int v_comp_x = (v_comp                                     ) & ((1 << preset->p[0].c) - 1);
    unsigned int v_comp_y = (v_comp >> (preset->p[0].c)                 ) & ((1 << preset->p[1].c) - 1);
    unsigned int v_comp_z = (v_comp >> (preset->p[0].c + preset->p[1].c)) & ((1 << preset->p[2].c) - 1);
    v[0] = numcomp_decompress_float(v_comp_x, &preset->p[0]);
    v[1] = numcomp_decompress_float(v_comp_y, &preset->p[1]);
    v[2] = numcomp_decompress_float(v_comp_z, &preset->p[2]);
}

float numcomp_test_vec3(const float* v, const LWNUMCOMPVEC3PRESET* preset) {
    unsigned int v_comp = numcomp_compress_vec3(v, preset);
    float v_decomp[3];
    numcomp_decompress_vec3(v_decomp, v_comp, preset);
    float dx = v[0] - v_decomp[0];
    float dy = v[1] - v_decomp[1];
    float dz = v[2] - v_decomp[2];
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

void numcomp_test_vec3_print(const float* v, const LWNUMCOMPVEC3PRESET* preset) {
    float err = numcomp_test_vec3(v, preset);
    LOGI("vec3:(%.4f,.%4f,.%4f) (err:%.6f)", v[0], v[1], v[2], err);
}

float float_random_01() {
    const int random_mask = 0x00ffffff;
    return (float)(pcg32_random() & random_mask) / random_mask;
}

float float_random_range(float m, float M) {
    return m + float_random_01() * (M - m);
}

void numcomp_test_all() {
    numcomp_batch_test_float();
    numcomp_batch_test_vec3();
    numcomp_batch_test_quaternion();
    numcomp_batch_test_mat4x4();
}

void numcomp_batch_test_vec3() {
    LWNUMCOMPVEC3PRESET p2;
    numcomp_init_vec3_preset(&p2,
                             11, -2.0f, +2.0f,
                             11, -2.0f, +2.0f,
                             10, +0.0f, +5.0f);
    const int random_mask = 0x00ffffff;
    for (int i = 0; i < 500; i++) {
        float v[3] = {
                float_random_range(-2.0f, +2.0f),
                float_random_range(-2.0f, +2.0f),
                float_random_range(+0.0f, +5.0f),
        };
        numcomp_test_vec3_print(v, &p2);
    }
}

void numcomp_batch_test_float() {
    LWNUMCOMPFLOATPRESET p1;
    numcomp_init_float_preset(&p1, 11, -2.0f, 2.0f);
    numcomp_test_float_print(-100.0f, &p1);
    for (int i = 0; i < 500; i++) {
        numcomp_test_float_print(p1.m + 0.0123f * i, &p1);
    }
}

unsigned int numcomp_compress_quaternion(const float q0[4], const LWNUMCOMPQUATERNIONPRESET* preset) {
    // abs_max_idx  smallest three components
    // [  2-bit  ][  10-bit  ][  10-bit  ][  10-bit  ]
    float q[4];
    memcpy(q, q0, sizeof(float) * 4);
//    int all_negative = 1;
//    for (int i = 0; i < 4; i++) {
//        if (q[i] > 0) {
//            all_negative = 0;
//            break;
//        }
//    }
//    if (all_negative) {
//        for (int i = 0; i < 4; i++) {
//            q[i] *= -1;
//        }
//    }
    // Find abs max component index
    int abs_max_idx = 0;
    float abs_max = fabsf(q[0]);
    int abs_max_negative = q[0] < 0;
    for (int i = 1; i < 4; i++) {
        if (abs_max < fabsf(q[i])) {
            abs_max = fabsf(q[i]);
            abs_max_idx = i;
            abs_max_negative = q[i] < 0;
        }
    }
    LOGI("abs_max_idx = %d (negative = %d)", abs_max_idx, abs_max_negative);
    if (abs_max_negative) {
        for (int i = 0; i < 4; i++) {
            q[i] *= -1;
        }
    }
    float smallest_threes[3];
    int st_index = 0;
    for (int i = 0; i < 4; i++) {
        if (i == abs_max_idx) {
            continue;
        }
        smallest_threes[st_index] = q[i];
        st_index++;
    }
    return numcomp_compress_vec3(smallest_threes, &preset->smallest_threes)
           | (abs_max_idx << preset->c_sum);
}

void numcomp_decompress_quaternion(float q[4], unsigned int c, const LWNUMCOMPQUATERNIONPRESET* preset) {
    int abs_max_idx = c >> preset->c_sum;
    unsigned int v_comp = c & ((1 << preset->c_sum) - 1);
    float smallest_threes[3];
    numcomp_decompress_vec3(smallest_threes, v_comp, preset);
    float q_sqr_sum = 0;
    int st_index = 0;
    for (int i = 0; i < 4; i++) {
        if (i == abs_max_idx) {
            continue;
        }

        q[i] = smallest_threes[st_index];
        q_sqr_sum += q[i] * q[i];
        st_index++;
    }
    q[abs_max_idx] = sqrtf(1.0f - q_sqr_sum);
}

// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
// q = [w, x, y, z]
void numcomp_convert_pry_to_quaternion(float *q, float pitch, float roll, float yaw) {
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

// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
// q = [w, x, y, z]
void numcomp_convert_quaternion_to_pry(const float *q, float *pitch, float *roll, float *yaw) {
//    int all_negative = 1;
//    for (int i = 0; i < 4; i++) {
//        if (q0[i] > 0) {
//            all_negative = 0;
//            break;
//        }
//    }
//    float q[4];
//    if (all_negative) {
//        q[0] = -q0[0];
//        q[1] = -q0[1];
//        q[2] = -q0[2];
//        q[3] = -q0[3];
//    } else {
//        memcpy(q, q0, sizeof(q));
//    }
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

void numcomp_test_quaternion(float pitch, float roll, float yaw, const LWNUMCOMPQUATERNIONPRESET* preset) {
    pitch = wrap_radian(pitch);
    roll = wrap_radian(roll);
    yaw = wrap_radian(yaw);

    float q[4];
    numcomp_convert_pry_to_quaternion(q, pitch, roll, yaw);
    float pitch_copy, roll_copy, yaw_copy;
    numcomp_convert_quaternion_to_pry(q, &pitch_copy, &roll_copy, &yaw_copy);

    int q_comp = numcomp_compress_quaternion(q, preset);
    float q_decomp[4], q_decomp_neg[4];
    numcomp_decompress_quaternion(q_decomp, q_comp, preset);
    for (int i = 0; i < 4; i++) {
        q_decomp_neg[i] = -q_decomp[i];
    }
    float pitch_decomp, roll_decomp, yaw_decomp;
    float pitch_decomp_neg, roll_decomp_neg, yaw_decomp_neg;
    numcomp_convert_quaternion_to_pry(q_decomp, &pitch_decomp, &roll_decomp, &yaw_decomp);
    numcomp_convert_quaternion_to_pry(q_decomp_neg, &pitch_decomp_neg, &roll_decomp_neg, &yaw_decomp_neg);
    float pitch_diff = pitch - pitch_decomp;
    float roll_diff = roll - roll_decomp;
    float yaw_diff = yaw - yaw_decomp;
    float err = sqrtf(pitch_diff*pitch_diff + roll_diff*roll_diff + yaw_diff*yaw_diff);
    LOGI("quat pitch-roll-yaw:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f)",
         pitch, roll, yaw,
         q[0], q[1], q[2], q[3]);
    LOGI("quat pitch-roll-yaw:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f) <decompressed>",
         pitch_decomp, roll_decomp, yaw_decomp,
         q_decomp[0], q_decomp[1], q_decomp[2], q_decomp[3]);
    LOGI("quat pitch-roll-yaw:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f) <decompressed> (negative)",
         pitch_decomp_neg, roll_decomp_neg, yaw_decomp_neg,
         q_decomp_neg[0], q_decomp_neg[1], q_decomp_neg[2], q_decomp_neg[3]);
    LOGI("quat pitch-roll-yaw: error %f",
         err);
}

void numcomp_init_quaternion_preset(LWNUMCOMPQUATERNIONPRESET* preset) {
    float m = 1.0f / sqrtf(2);
    int c = 10;
    numcomp_init_vec3_preset(&preset->smallest_threes,
                             c, -m, m,
                             c, -m, m,
                             c, -m, m);
    preset->c_sum = c + c + c;
}

void numcomp_batch_test_quaternion() {
    LWNUMCOMPQUATERNIONPRESET preset;
    numcomp_init_quaternion_preset(&preset);

    float zeros[3] = {-0.000000119209304f,};
    unsigned int zeros_comp = numcomp_compress_vec3(zeros, &preset.smallest_threes);
    float zeros_decomp[3];
    numcomp_decompress_vec3(zeros_decomp, zeros_comp, &preset.smallest_threes);

    numcomp_test_quaternion(+ 0.000f, + 0.000f,  0.00f, &preset);
    numcomp_test_quaternion(+ 1.000f, + 0.000f,  0.00f, &preset);
    numcomp_test_quaternion(+ 0.000f, + 1.000f,  0.00f, &preset);
    numcomp_test_quaternion(+ 0.000f, + 0.000f,  1.00f, &preset);
    numcomp_test_quaternion(+ 1.000f, + 2.000f,  3.00f, &preset);
    numcomp_test_quaternion(+10.000f, -20.000f, 30.00f, &preset); //***
    numcomp_test_quaternion(-10.000f, -20.000f, 30.00f, &preset);
    numcomp_test_quaternion(+ 1.234f, - 0.051f,  2.99f, &preset);
    numcomp_test_quaternion(+ 1.234f, - 3.100f,  0.00f, &preset);
    numcomp_test_quaternion(- 1.234f, - 3.100f,  0.00f, &preset);
    numcomp_test_quaternion(- 1.234f, - 3.100f,  3.00f, &preset);
    numcomp_test_quaternion(- 1.234f, - 3.100f, 30.00f, &preset);

    //mat4x4_from_quat()
    //quat_from_mat4x4()
}

void numcomp_test_mat4x4(float pitch, float roll, float yaw, const LWNUMCOMPQUATERNIONPRESET* preset) {
    pitch = wrap_radian(pitch);
    roll = wrap_radian(roll);
    yaw = wrap_radian(yaw);

    float q[4];
    numcomp_convert_pry_to_quaternion(q, pitch, roll, yaw);
    float pitch_copy, roll_copy, yaw_copy;
    numcomp_convert_quaternion_to_pry(q, &pitch_copy, &roll_copy, &yaw_copy);

    mat4x4 m;
    mat4x4_from_quat(m, q);
    int m_comp = numcomp_compress_mat4x4(m, preset);
    mat4x4 m_decomp;
    numcomp_decompress_mat4x4(m_decomp, m_comp, preset);
    quat q_decomp;
    quat_from_mat4x4(q_decomp, m_decomp);

    float pitch_decomp, roll_decomp, yaw_decomp;
    numcomp_convert_quaternion_to_pry(q_decomp, &pitch_decomp, &roll_decomp, &yaw_decomp);
    float pitch_diff = pitch - pitch_decomp;
    float roll_diff = roll - roll_decomp;
    float yaw_diff = yaw - yaw_decomp;
    float err = sqrtf(pitch_diff*pitch_diff + roll_diff*roll_diff + yaw_diff*yaw_diff);
    LOGI("mat4x4 pitch-roll-yaw:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f)",
         pitch, roll, yaw,
         q[0], q[1], q[2], q[3]);
    LOGI("mat4x4 pitch-roll-yaw:(%f,%f,%f) --- quaternion:(%f,%f,%f,%f) <decompressed>",
         pitch_decomp, roll_decomp, yaw_decomp,
         q_decomp[0], q_decomp[1], q_decomp[2], q_decomp[3]);
    LOGI("mat4x4 pitch-roll-yaw: error %f",
         err);
}

void numcomp_batch_test_mat4x4() {
    LWNUMCOMPQUATERNIONPRESET preset;
    numcomp_init_quaternion_preset(&preset);
    numcomp_test_mat4x4(+ 0.000f, + 0.000f,  0.00f, &preset);
    numcomp_test_mat4x4(+ 1.000f, + 0.000f,  0.00f, &preset);
    numcomp_test_mat4x4(+ 0.000f, + 1.000f,  0.00f, &preset);
    numcomp_test_mat4x4(+ 0.000f, + 0.000f,  1.00f, &preset);
    numcomp_test_mat4x4(+ 1.000f, + 2.000f,  3.00f, &preset);
    numcomp_test_mat4x4(+10.000f, -20.000f, 30.00f, &preset); //***
    numcomp_test_mat4x4(-10.000f, -20.000f, 30.00f, &preset);
    numcomp_test_mat4x4(+ 1.234f, - 0.051f,  2.99f, &preset);
    numcomp_test_mat4x4(+ 1.234f, - 3.100f,  0.00f, &preset);
    numcomp_test_mat4x4(- 1.234f, - 3.100f,  0.00f, &preset);
    numcomp_test_mat4x4(- 1.234f, - 3.100f,  3.00f, &preset);
    numcomp_test_mat4x4(- 1.234f, - 3.100f, 30.00f, &preset);
}

unsigned int numcomp_compress_mat4x4(const mat4x4 m, const LWNUMCOMPQUATERNIONPRESET* preset) {
    quat q;
    quat_from_mat4x4(q, m);
    return numcomp_compress_quaternion(q, preset);
}

void numcomp_decompress_mat4x4(mat4x4 m, unsigned int v_comp, const LWNUMCOMPQUATERNIONPRESET* preset) {
    quat q;
    numcomp_decompress_quaternion(q, v_comp, preset);
    mat4x4_from_quat(m, q);
}
