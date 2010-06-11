// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Terrain rendering engine

#include "r_local.h"

#define TERRAIN_PATCH_SIZE			17
#define TERRAIN_PATCH_SIZE_F		17.f
#define TERRAIN_NUM_VERTS			(										\
										TERRAIN_PATCH_SIZE					\
										* TERRAIN_PATCH_SIZE				\
										+ TERRAIN_PATCH_SIZE * 4 - 4		\
									)	// plus skirt verts at each edge minus
										// duplicated corners
#define TERRAIN_NUM_BODY_INDICES	(										\
										(2 * TERRAIN_PATCH_SIZE + 2)		\
										* (TERRAIN_PATCH_SIZE - 1)			\
									)
#define TERRAIN_NUM_SKIRT_INDICES	(										\
										2 * (TERRAIN_PATCH_SIZE * 2			\
											+ 2 * (TERRAIN_PATCH_SIZE - 2))	\
										+ 2									\
									)	// + 2 degenerate triangles
#define TERRAIN_NUM_INDICES			(TERRAIN_NUM_BODY_INDICES				\
										+ TERRAIN_NUM_SKIRT_INDICES)
#define TERRAIN_LOD					40.f

// uncomment to enable uniform-based height transfers instead of VBO
// retransmissions
#define UNIFORM_HEIGHTS
// uncomment to enable buffer memory mapping instead of reuploading the entire
// geometry every time
//#define MAP_VBO

// resources
GLuint		r_hmap_tex;
int			r_ter_max_levels;
ac_vertex_t	r_ter_verts[TERRAIN_NUM_VERTS];
uint		r_ter_VBOs[2];

static void r_fill_terrain_indices(ushort *indices) {
	short	i, j;	// must be signed
	ushort	*p = indices;

	// patch body
	for (i = 0; i < TERRAIN_PATCH_SIZE - 1; i++) {
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
			*(p++) = i * TERRAIN_PATCH_SIZE + j;
			*(p++) = (i + 1) * TERRAIN_PATCH_SIZE + j;
		}
		if (i < TERRAIN_PATCH_SIZE - 2) {	// add a degenerate triangle
	        *p = *(p - 1);
	        p++;
	        *(p++) = (i + 1) * TERRAIN_PATCH_SIZE;
	    }
	}

	// add a degenerate triangle to connect body with skirt
	*(p++) = (TERRAIN_PATCH_SIZE - 1) * TERRAIN_PATCH_SIZE + TERRAIN_PATCH_SIZE - 1;
	*(p++) = TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;

	// skirt
	// a single strip that goes around the entire patch
	// -Z edge
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;
		*(p++) = i;
	}
	// -X edge
	for (i = 1; i < TERRAIN_PATCH_SIZE - 1; i++) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE + 2) - 1;
		*(p++) = i * TERRAIN_PATCH_SIZE + TERRAIN_PATCH_SIZE - 1;
	}
	// +Z edge
	for (i = TERRAIN_PATCH_SIZE - 1; i >= 0; i--) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE + 1;
		*(p++) = i + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE - 1);
	}
	// +X edge
	for (i = TERRAIN_PATCH_SIZE - 2; i >= 1; i--) {
		*(p++) = i * 2 + TERRAIN_PATCH_SIZE * (TERRAIN_PATCH_SIZE + 2) - 2;
		*(p++) = i * TERRAIN_PATCH_SIZE;
	}
	*(p++) = TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE;
	*(p++) = 0;
	assert(p - indices == TERRAIN_NUM_INDICES);
}

static void r_fill_terrain_vertices(ac_vertex_t *verts) {
	int i, j;
	float s, t;
	const float invScaleX = 1.f / (TERRAIN_PATCH_SIZE - 1.f);
	const float invScaleY = 1.f / (TERRAIN_PATCH_SIZE - 1.f);

	// fill patch body vertices
	ac_vertex_t *v = verts;
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		t = 1.f - i * invScaleY;
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
			s = j * invScaleX;
			v->st[0] = s;
			v->st[1] = t;
			v->pos = ac_vec_set(s, 1.f, t, 0.f);
			v++;
		}
	}

	// fill skirt vertices
	for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
		s = j * invScaleX;
		v->st[0] = s;
		v->st[1] = 1.f;
		v->pos = ac_vec_set(s, -1.f, 1.f, 0.f);
		v++;
		v->st[0] = s;
		v->st[1] = 0.f;
		v->pos = ac_vec_set(s, -1.f, 0.f, 0.f);
		v++;
	}

	for (i = 1; i < TERRAIN_PATCH_SIZE - 1; i++) {
		t = 1.f - i * invScaleY;
		v->st[0] = 0.f;
		v->st[1] = t;
		v->pos = ac_vec_set(0.f, -1.f, t, 0.f);
		v++;
		v->st[0] = 1.f;
		v->st[1] = t;
		v->pos = ac_vec_set(1.f, -1.f, t, 0.f);
		v++;
	}
	assert(v - verts == TERRAIN_NUM_VERTS);
}

static void r_calc_terrain_lodlevels(void) {
	// calculate max LOD levels
	int i, pow2 = (HEIGHTMAP_SIZE - 1) / (TERRAIN_PATCH_SIZE - 1);
	r_ter_max_levels = 0;
	for (i = 1; i < pow2; i *= 2)
		r_ter_max_levels++;
}

void r_create_terrain(void) {
	ushort		indices[TERRAIN_NUM_INDICES];
	// signal the game from time to time
	g_loading_tick();
	r_fill_terrain_indices(indices);
	g_loading_tick();
	r_fill_terrain_vertices(r_ter_verts);
	g_loading_tick();
	r_calc_terrain_lodlevels();
	g_loading_tick();

	// generate VBOs
	glGenBuffersARB(2, r_ter_VBOs);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, r_ter_VBOs[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_ter_VBOs[1]);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
		sizeof(r_ter_verts), r_ter_verts,
#ifndef UNIFORM_HEIGHTS
		GL_DYNAMIC_DRAW_ARB
#else
		GL_STATIC_DRAW_ARB
#endif
	);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
		sizeof(indices), indices, GL_STATIC_DRAW_ARB);
	// unbind VBOs
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
}

void r_set_heightmap(void) {
	if (gen_heightmap != NULL)
		glDeleteTextures(1, &r_hmap_tex);
	glGenTextures(1, &r_hmap_tex);
	glBindTexture(GL_TEXTURE_2D, r_hmap_tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8,
				HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, 0,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, gen_heightmap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void r_destroy_terrain(void) {
	glDeleteTextures(1, &r_hmap_tex);
	glDeleteBuffersARB(2, r_ter_VBOs);
}

static inline float r_sample_height(float s, float t) {
	int x = roundf(s * (HEIGHTMAP_SIZE - 1));
	int y = roundf(t * (HEIGHTMAP_SIZE - 1));
	return (float)gen_heightmap[y * HEIGHTMAP_SIZE + x];
}

#if !defined(UNIFORM_HEIGHTS) && defined(MAP_VBO)
// uncomment to enable read/write access to the VBO, thus slightly reducing the
// number of calculations; this may be offset by optimized DMA of a write-only,
// though, which is why we have it as a toggleable option
#define RW_TERRAIN_VBO
#endif
static void r_terrain_patch(float bu, float bv, float scale) {
	int i, j;
#if defined(UNIFORM_HEIGHTS) || (defined(MAP_VBO) && defined(RW_TERRAIN_VBO))
	float s, t;
	static const float invScaleX = 1.f / (TERRAIN_PATCH_SIZE - 1.f);
	static const float invScaleY = 1.f / (TERRAIN_PATCH_SIZE - 1.f);
#endif
#ifdef UNIFORM_HEIGHTS
	float heights[TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE + 3];
#else
	ac_vertex_t *v;
#endif

#ifdef UNIFORM_HEIGHTS
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		t = /*(1.f - */i * invScaleY/*)*/ * scale;
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++) {
			s = j * invScaleX * scale;
			heights[i * TERRAIN_PATCH_SIZE + j] = r_sample_height(bu + s,
																bv + t);
		}
	}
	glUniform4fvARB(r_ter_height_samples,
		sizeof(heights) / (sizeof(heights[0]) * 4), heights);
#elif defined(MAP_VBO)
	// sample the heightmap and set vertex Y component
	v = glMapBufferARB(GL_ARRAY_BUFFER_ARB,
#ifdef RW_TERRAIN_VBO
		GL_READ_WRITE_ARB
#else
		GL_WRITE_ONLY_ARB
#endif
	);

	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
#ifndef RW_TERRAIN_VBO
		t = 1.f - i * invScaleY;
#endif
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++, v++) {
#ifndef RW_TERRAIN_VBO
			s = j * invScaleX;
#endif
			v->pos.f[1] = r_sample_height(
#ifdef RW_TERRAIN_VBO
				bu + v->st[0] * scale,
				bv + v->st[1] * scale
#else
				bu + s * scale,
				bv + t * scale
#endif
			);
		}
	}
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
#else
	v = r_ter_verts;
	for (i = 0; i < TERRAIN_PATCH_SIZE; i++) {
		for (j = 0; j < TERRAIN_PATCH_SIZE; j++, v++) {
			v->pos.f[1] = r_sample_height(
				bu + v->st[0] * scale,
				bv + v->st[1] * scale
			);
		}
	}
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
		sizeof(r_ter_verts), r_ter_verts, GL_STREAM_DRAW_ARB);
#endif
	glUniform3fARB(r_ter_patch_params, bu, bv, scale);

	glVertexPointer(3, GL_FLOAT, sizeof(ac_vertex_t), (void *)0);
	glTexCoordPointer(2, GL_FLOAT, sizeof(ac_vertex_t),
		(void *)offsetof(ac_vertex_t, st[0]));
	glDrawElements(GL_TRIANGLE_STRIP,
					TERRAIN_NUM_INDICES,
					GL_UNSIGNED_SHORT,
					(void *)0);
	*r_vert_counter += TERRAIN_NUM_VERTS;
	*r_tri_counter += TERRAIN_NUM_INDICES - 2;
	(*r_visible_patch_counter)++;
}

static void r_recurse_terrain(float minU, float minV,
								float maxU, float maxV,
								int level, float scale) {
	ac_vec4_t v, bounds[2];
	float halfU = (minU + maxU) * 0.5;
	float halfV = (minV + maxV) * 0.5;

	// apply frustum culling
	bounds[0] = ac_vec_set((minU - 0.5) * HEIGHTMAP_SIZE,
					-10.f,
					(minV - 0.5) * HEIGHTMAP_SIZE,
					0.f);
	bounds[1] = ac_vec_set((maxU - 0.5) * HEIGHTMAP_SIZE,
					HEIGHT,
					(maxV - 0.5) * HEIGHTMAP_SIZE,
					0.f);
	if (r_cull_bbox(bounds) == CR_OUTSIDE) {
		(*r_culled_patch_counter)++;
		return;
	}

	float d2 = (maxU - minU) * (float)HEIGHTMAP_SIZE
				/ (TERRAIN_PATCH_SIZE_F - 1.f);
	d2 *= d2;

	v = ac_vec_set((halfU - 0.5) * (float)HEIGHTMAP_SIZE,
				128.f,
				(halfV - 0.5) * (float)HEIGHTMAP_SIZE,
				0.f);
	v = ac_vec_sub(v, r_viewpoint);

	// use distances squared
	float f2 = ac_vec_dot(v, v) / d2;

	if (f2 > TERRAIN_LOD * TERRAIN_LOD || level < 1)
		r_terrain_patch(minU, minV, scale);
	else {
		scale *= 0.5;
		r_recurse_terrain(minU, minV, halfU, halfV, level - 1,
									scale);
		r_recurse_terrain(halfU, minV, maxU, halfV, level - 1,
									scale);
		r_recurse_terrain(minU, halfV, halfU, maxV, level - 1,
									scale);
		r_recurse_terrain(halfU, halfV, maxU, maxV, level - 1,
									scale);
	}
}

void r_draw_terrain(void) {
	glUseProgramObjectARB(r_ter_prog);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, r_ter_VBOs[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_ter_VBOs[1]);
	glBindTexture(GL_TEXTURE_2D, r_hmap_tex);

	// traverse the quadtree
	r_recurse_terrain(0.f, 0.f, 1.f, 1.f,
								r_ter_max_levels, 1.f);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	glUseProgramObjectARB(0);
}
