// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

#ifndef AC_MATH_H
#define AC_MATH_H

#include <math.h>

/// \file ac_math.h
/// \brief Public interface to the math library.
/// \addtogroup mathlib Math library
/// @{

// general math

/// \f$ min(a, b) \f$
extern inline float ac_min(float a, float b);

/// \f$ max(a, b) \f$
extern inline float ac_max(float a, float b);

// vector math

// SSE intrinsics headers
#include <xmmintrin.h>

#ifdef __GNUC__
	#define ALIGNED_16	__attribute__ ((__aligned__ (16)))
#else
	#define ALIGNED_16 	__declspec(align(16))
#endif

/// 4-element vector union.
typedef union {
	ALIGNED_16 float		f[4];	///< traditional floating point numbers
	__m128					sse;	///< SSE 128-bit data type
} ac_vec4_t;

// This is to fix certain SSE-related crashes on 32-bit systems (it's
// unnecessary on x86-64). What happens is that since I'm mixing non-SSE and SSE
// code, it can sometimes happen that the stack is aligned to 4 bytes (the
// legacy way) instead of 16 bytes (what SSE expects). This can lead to
// seemingly random crashes. Adding this attribute causes the compiler to add
// some code to enforce 16-byte alignment.
#if __x86_64__
#define STACK_ALIGN
#else
#define STACK_ALIGN		__attribute__((force_align_arg_pointer))
#endif

/// \f$ \vec a = [x, y, z, w] \f$
extern inline ac_vec4_t ac_vec_set(float x, float y, float z, float w)
	STACK_ALIGN;

/// \f$ \vec a = [b, b, b, b] \f$
extern inline ac_vec4_t ac_vec_setall(float b) STACK_ALIGN;

/// \f$ \vec b = - \vec a \f$
extern inline ac_vec4_t ac_vec_negate(ac_vec4_t a) STACK_ALIGN;

/// \f$ \vec c = \vec a + \vec b \f$
extern inline ac_vec4_t ac_vec_add(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// \f$ \vec c = \vec a - \vec b \f$
extern inline ac_vec4_t ac_vec_sub(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// \f$ \vec c = \vec a * \vec b \f$ (\f$ c_1 = a_1 * b_1, c_2 = a_2 * b_2 \f$
/// etc.)
extern inline ac_vec4_t ac_vec_mul(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// \f$ \vec c = [a_1 * b, a_2 * b, a_3 * b, a_4 * b] \f$
extern inline ac_vec4_t ac_vec_mulf(ac_vec4_t a, float b) STACK_ALIGN;

/// \f$ \vec d = \vec a * \vec b + \vec c \f$
extern inline ac_vec4_t ac_vec_ma(ac_vec4_t a, ac_vec4_t b, ac_vec4_t c)
	STACK_ALIGN;

/// \f$ c = \vec a \circ \vec b \f$
extern inline float ac_vec_dot(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// \f$ \vec c = \vec a \times \vec b \f$
extern inline ac_vec4_t ac_vec_cross(ac_vec4_t a, ac_vec4_t b) STACK_ALIGN;

/// \f$ l = |\vec a| \f$
extern inline float ac_vec_length(ac_vec4_t a) STACK_ALIGN;

/// \f$ \vec n = \frac{\vec a}{|\vec a|} \f$
extern inline ac_vec4_t ac_vec_normalize(ac_vec4_t a) STACK_ALIGN;

/// Vector decomposition into a unit length direction vector and length scalar.
/// \param b	vector to decompose
/// \param a	pointer to vector to put the direction vector into
/// \return		length of vector \e b
extern inline float ac_vec_decompose(ac_vec4_t b, ac_vec4_t *a) STACK_ALIGN;

/// Write from __m128 (a) to flat floats (b).
extern inline void ac_vec_tofloat(ac_vec4_t a, float b[4]) STACK_ALIGN;

/// Write from flat floats (a) to __m128 (b).
extern inline ac_vec4_t ac_vec_tosse(float *f) STACK_ALIGN;

/// @}

#endif // AC_MATH_H
