// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Prop drawing engine

#include "gl14_local.h"

#define PROP_LOD_DISTANCE	250.f

uint		gl14_prop_tex;
uint		gl14_prop_VBOs[2];

void gl14_create_props(void) {
	ac_vertex_t	verts[TREE_BASE + 1		// LOD 0
					+ TREE_BASE - 2		// LOD 1
					+ TREE_BASE - 4		// LOD 2
					+ BLDG_FLAT_VERTS
					+ BLDG_SLNT_VERTS];
	uchar		indices[TREE_BASE + 2		// LOD 0
					+ TREE_BASE			// LOD 1
					+ TREE_BASE - 2		// LOD 2
					+ BLDG_FLAT_INDICES
					+ BLDG_SLNT_INDICES];
	uchar		texture[PROP_TEXTURE_SIZE * PROP_TEXTURE_SIZE];
#if OPENGL_DEBUG
	uint		i;
#endif

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	gen_props(texture, verts, indices);

	// generate texture
	glGenTextures(1, &gl14_prop_tex);
	glBindTexture(GL_TEXTURE_2D, gl14_prop_tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8,
				PROP_TEXTURE_SIZE, PROP_TEXTURE_SIZE, 0,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// generate VBOs
	glGenBuffersARB(2, gl14_prop_VBOs);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, gl14_prop_VBOs[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, gl14_prop_VBOs[1]);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
		sizeof(verts), verts, GL_STATIC_DRAW_ARB);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
		sizeof(indices), indices, GL_STATIC_DRAW_ARB);
	// unbind VBOs
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

#if OPENGL_DEBUG
	if (GLEW_KHR_debug) {
		glObjectLabel(GL_TEXTURE, gl14_prop_tex, -1, "Props");
		for (i = 0; i < sizeof(gl14_prop_VBOs) / sizeof(gl14_prop_VBOs[0]); ++i)
			glObjectLabel(GL_BUFFER, gl14_prop_VBOs[i], -1, "Props");
	}
#endif

	OPENGL_EVENT_END();
}

static void gl14_recurse_proptree_drawall(ac_prop_t *node) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	if (node->trees) {
		int i;
		float d2;
		ac_tree_t *t;
		long ofs, num;
		// pick level of detail
		ac_vec4_t l = ac_vec_mulf(
			ac_vec_add(node->bounds[0], node->bounds[1]), 0.5);
		l = ac_vec_sub(l, gl14_viewpoint);
		d2 = ac_vec_dot(l, l);

		if (d2 < PROP_LOD_DISTANCE * PROP_LOD_DISTANCE) {
			ofs = 0;
			num = TREE_BASE + 2;
		} else if (d2 < PROP_LOD_DISTANCE * PROP_LOD_DISTANCE * 4) {
			ofs = TREE_BASE + 2;
			num = TREE_BASE;
		} else {
			ofs = TREE_BASE + 2 + TREE_BASE;
			num = TREE_BASE - 2;
		}

		for (t = node->trees, i = 0; i < TREES_PER_FIELD; i++, t++) {
			OPENGL_EVENT_BEGIN(0, "Draw tree");

			glMultiTexCoord3fv(GL_TEXTURE1, t->pos.f);
			glMultiTexCoord4f(GL_TEXTURE2, t->XZscale, t->Yscale, t->XZscale,
				t->ang);

			glDrawElements(GL_TRIANGLE_FAN, num, GL_UNSIGNED_BYTE, (void *)ofs);
			*gl14_vert_counter += num - 1;
			*gl14_tri_counter += num - 2;

			OPENGL_EVENT_END();
		}

		OPENGL_EVENT_END();

		return;
	} else if (node->bldgs) {
		int i;
		ac_bldg_t *b;
		for (b = node->bldgs, i = 0; i < BLDGS_PER_FIELD; i++, b++) {
			OPENGL_EVENT_BEGIN(0, "Draw building");

			glMultiTexCoord3fv(GL_TEXTURE1, b->pos.f);
			glMultiTexCoord4f(GL_TEXTURE2, b->Xscale, b->Yscale, b->Zscale,
				b->ang);

			if (b->slantedRoof) {
				OPENGL_EVENT(0, "Slanted roof");

				glDrawElements(GL_TRIANGLE_STRIP, BLDG_SLNT_INDICES,
					GL_UNSIGNED_BYTE, (void *)(TREE_BASE * 3
						+ BLDG_FLAT_INDICES));
				*gl14_vert_counter += BLDG_FLAT_VERTS;
				*gl14_tri_counter += BLDG_FLAT_INDICES - 2;
			} else {
				OPENGL_EVENT(0, "Flat roof");

				glDrawElements(GL_TRIANGLE_STRIP, BLDG_FLAT_INDICES,
					GL_UNSIGNED_BYTE, (void *)(TREE_BASE * 3));
				*gl14_vert_counter += BLDG_SLNT_VERTS;
				*gl14_tri_counter += BLDG_SLNT_INDICES - 2;
			}

			OPENGL_EVENT_END();
		}

		OPENGL_EVENT_END();

		return;
	}
	if (node->child[0])
		gl14_recurse_proptree_drawall(node->child[0]);
	if (node->child[1])
		gl14_recurse_proptree_drawall(node->child[1]);
	if (node->child[2])
		gl14_recurse_proptree_drawall(node->child[2]);
	if (node->child[3])
		gl14_recurse_proptree_drawall(node->child[3]);

	OPENGL_EVENT_END();
}

static void gl14_recurse_proptree(ac_prop_t *node, int step) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	switch (gl14_cull_bbox(node->bounds)) {
		case CR_OUTSIDE:
			OPENGL_EVENT_END();
			return;
		case CR_INSIDE:
			if (node)
				gl14_recurse_proptree_drawall(node);
			break;
		case CR_INTERSECT:
			if ((step >>= 1) < 2 && node) {
				gl14_recurse_proptree_drawall(node);

				OPENGL_EVENT_END();

				return;
			}
			if (node->child[0])
				gl14_recurse_proptree(node->child[0], step);
			if (node->child[1])
				gl14_recurse_proptree(node->child[1], step);
			if (node->child[2])
				gl14_recurse_proptree(node->child[2], step);
			if (node->child[3])
				gl14_recurse_proptree(node->child[3], step);
			break;
	}

	OPENGL_EVENT_END();
}

void gl14_draw_props(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// make the necessary state changes
	glBindTexture(GL_TEXTURE_2D, gl14_prop_tex);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, gl14_prop_VBOs[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, gl14_prop_VBOs[1]);
	glVertexPointer(3, GL_FLOAT, sizeof(ac_vertex_t),
					(void *)offsetof(ac_vertex_t, pos.f[0]));
	glTexCoordPointer(2, GL_FLOAT, sizeof(ac_vertex_t),
					(void *)offsetof(ac_vertex_t, st[0]));
	glUseProgramObjectARB(gl14_prop_prog);

	gl14_recurse_proptree(gen_proptree, PROPMAP_SIZE / 2);

	// bring the previous state back
	glUseProgramObjectARB(0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	OPENGL_EVENT_END();
}

void gl14_destroy_props(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	gen_free_proptree(NULL);
	glDeleteTextures(1, &gl14_prop_tex);
	glDeleteBuffersARB(2, gl14_prop_VBOs);

	OPENGL_EVENT_END();
}
