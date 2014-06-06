// AC-130 shooter
// Written by Leszek Godlewski <github@inequation.org>

// Math module; actually, just a place where the temporary vector can be safely
// compiled into the program

#include <assert.h>
#include "ac_math.h"

static const unsigned int	negzero = 0x80000000;

inline float ac_min(float a, float b) {
	return a < b ? a : b;
}

inline float ac_max(float a, float b) {
	return a > b ? a : b;
}

inline ac_vec4_t ac_vec_set(float x, float y, float z, float w) {
	return (ac_vec4_t)_mm_set_ps(w, z, y, x);
}

inline ac_vec4_t ac_vec_setall(float b) {
	return (ac_vec4_t)_mm_set1_ps(b);
}

inline ac_vec4_t ac_vec_negate(ac_vec4_t a) {
	ac_vec4_t tmp;
	tmp = ac_vec_setall(*((float*)&negzero));
	return (ac_vec4_t)_mm_xor_ps(a.sse, tmp.sse);
}

inline ac_vec4_t ac_vec_add(ac_vec4_t a, ac_vec4_t b) {
	return (ac_vec4_t)_mm_add_ps(a.sse, b.sse);
}

inline ac_vec4_t ac_vec_sub(ac_vec4_t a, ac_vec4_t b) {
	return (ac_vec4_t)_mm_sub_ps(a.sse, b.sse);
}

inline ac_vec4_t ac_vec_mul(ac_vec4_t a, ac_vec4_t b) {
	return (ac_vec4_t)_mm_mul_ps(a.sse, b.sse);
}

inline ac_vec4_t ac_vec_mulf(ac_vec4_t a, float b) {
	ac_vec4_t tmp;
	tmp = ac_vec_setall(b);
	return (ac_vec4_t)_mm_mul_ps(a.sse, tmp.sse);
}

inline ac_vec4_t ac_vec_ma(ac_vec4_t a, ac_vec4_t b, ac_vec4_t c) {
	return (ac_vec4_t)_mm_add_ps(_mm_mul_ps(a.sse, b.sse), c.sse);
}

inline float ac_vec_dot(ac_vec4_t a, ac_vec4_t b) {
    // NOTE: this code causes segfaults on win32 for some bizarre reason...
//#ifndef WIN32
	ac_vec4_t tmp = ac_vec_mul(a, b);
	return tmp.f[0] + tmp.f[1] + tmp.f[2]/* + tmp.f[3]*/;
/*#else
    return a.f[0] * b.f[0] + a.f[1] * b.f[1] + a.f[2] * b.f[2];
#endif*/
}

inline ac_vec4_t ac_vec_cross(ac_vec4_t a, ac_vec4_t b) {
	return ac_vec_set(a.f[1] * b.f[2] - a.f[2] * b.f[1],
					a.f[2] * b.f[0] - a.f[0] * b.f[2],
					a.f[0] * b.f[1] - a.f[1] * b.f[0],
					0.f);
}

inline float ac_vec_length(ac_vec4_t a) {
	return sqrtf(ac_vec_dot(a, a));
}

inline ac_vec4_t ac_vec_normalize(ac_vec4_t a) {
	float invl = 1.f / ac_vec_length(a);
	return ac_vec_mulf(a, invl);
}

inline float ac_vec_decompose(ac_vec4_t b, ac_vec4_t *a) {
	float l = ac_vec_length(b);
	*a = ac_vec_mulf(b, 1.f / l);
	return l;
}

inline void ac_vec_tofloat(ac_vec4_t a, float b[4]) {
	_mm_store_ps(b, a.sse);
}

inline ac_vec4_t ac_vec_tosse(float *f) {
	return ac_vec_set(f[0], f[1], f[2], f[3]);
}
