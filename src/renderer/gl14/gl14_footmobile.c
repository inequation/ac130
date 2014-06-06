// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Ground troops drawing engine

#include "gl14_local.h"

// embed font texture
#define STRINGIFY(A)  #A
#include "../../footmobile.h"

uint		gl14_fmb_tex;
uint		gl14_fmb_VBOs[2];

void gl14_create_footmobile(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// generate texture
	glGenTextures(1, &gl14_fmb_tex);
	glBindTexture(GL_TEXTURE_2D, gl14_fmb_tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8,
				FOOTMOBILE_WIDTH, FOOTMOBILE_HEIGHT, 0,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, FOOTMOBILE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	// HACK HACK HACK: this is all temporary, we'll be using proper 3D geometry
	// later on
	ac_vertex_t verts[4];
	uchar indices[4];
	uint i;

	for (i = 0; i < 4; i++) {
		verts[i].pos = ac_vec_set(
			i % 2 == 0 ? -0.25 : 0.25,
			i < 2 ? 1.f : 0.f,
			0,
			0);
		verts[i].st[0] = i % 2 == 0 ? 0 : 0.5;
		verts[i].st[1] = i < 2 ? 0 : 1;
		indices[i] = i;
	}

	// generate VBOs
	glGenBuffersARB(2, gl14_fmb_VBOs);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, gl14_fmb_VBOs[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, gl14_fmb_VBOs[1]);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
		sizeof(verts), verts, GL_STATIC_DRAW_ARB);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
		sizeof(indices), indices, GL_STATIC_DRAW_ARB);
	// unbind VBOs
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

#if OPENGL_DEBUG
	if (GLEW_KHR_debug) {
		glObjectLabel(GL_TEXTURE, gl14_fmb_tex, -1, "Footmobile");
		for (i = 0; i < sizeof(gl14_fmb_VBOs) / sizeof(gl14_fmb_VBOs[0]); ++i)
			glObjectLabel(GL_BUFFER, gl14_fmb_VBOs[i], -1, "Footmobile");
	}
#endif

	OPENGL_EVENT_END();
}

void gl14_start_footmobiles(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// make the necessary state changes
	glBindTexture(GL_TEXTURE_2D, gl14_fmb_tex);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	// disable writing to the depth buffer to get rid of the ugly artifacts
	glDepthMask(GL_FALSE);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, gl14_fmb_VBOs[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, gl14_fmb_VBOs[1]);
	glVertexPointer(3, GL_FLOAT, sizeof(ac_vertex_t),
					(void *)offsetof(ac_vertex_t, pos.f[0]));
	glTexCoordPointer(2, GL_FLOAT, sizeof(ac_vertex_t),
					(void *)offsetof(ac_vertex_t, st[0]));
	glUseProgramObjectARB(gl14_fmb_prog);

	OPENGL_EVENT_END();
}

void gl14_draw_squad(ac_footmobile_t *squad, size_t troops) {
	size_t i;

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	for (i = 0; i < troops; i++, squad++) {
		glMultiTexCoord3fv(GL_TEXTURE1, squad->pos.f);
		glMultiTexCoord3f(GL_TEXTURE2,
			squad->stance == STANCE_STAND ? 0.f : 0.5, 0.f, squad->ang);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, (void *)0);
		*gl14_vert_counter += 4;
		*gl14_tri_counter += 2;
	}

	OPENGL_EVENT_END();
}

void gl14_finish_footmobiles(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// bring the previous state back
	glUseProgramObjectARB(0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	// reenable writing to the depth buffer
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	OPENGL_EVENT_END();
}

void gl14_destroy_footmobile(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glDeleteBuffersARB(2, gl14_fmb_VBOs);
	glDeleteTextures(1, &gl14_fmb_tex);

	OPENGL_EVENT_END();
}
