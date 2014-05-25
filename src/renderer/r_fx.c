// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Special effects engine

#include "r_local.h"

// FX resources
uint		r_fx_tex;
uint		r_fx_VBOs[2];

void r_create_fx(void) {
	ac_vertex_t	verts[4];
	uchar		indices[4];
	uchar		texture[2 * FX_TEXTURE_SIZE * FX_TEXTURE_SIZE];

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	gen_fx(texture, verts, indices);

	// generate texture
	glGenTextures(1, &r_fx_tex);
	glBindTexture(GL_TEXTURE_2D, r_fx_tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8_ALPHA8,
				FX_TEXTURE_SIZE, FX_TEXTURE_SIZE, 0,
				GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// generate VBOs
	glGenBuffersARB(2, r_fx_VBOs);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, r_fx_VBOs[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_fx_VBOs[1]);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,
		sizeof(verts), verts, GL_STATIC_DRAW_ARB);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
		sizeof(indices), indices, GL_STATIC_DRAW_ARB);
	// unbind VBOs
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

	OPENGL_EVENT_END();
}

// uncomment to restore fixed pipeline sprites
//#define FX_FIXED_PIPELINE
void r_start_fx(void) {
	// not ended in this function on purpose - ended in r_finish_fx() instead
	// for better apitrace output
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// make the necessary state changes
	glBindTexture(GL_TEXTURE_2D, r_fx_tex);
	glEnable(GL_BLEND);
	// disable writing to the depth buffer to get rid of the ugly artifacts
	glDepthMask(GL_FALSE);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, r_fx_VBOs[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, r_fx_VBOs[1]);
	glVertexPointer(3, GL_FLOAT, sizeof(ac_vertex_t),
					(void *)offsetof(ac_vertex_t, pos.f[0]));
	glTexCoordPointer(2, GL_FLOAT, sizeof(ac_vertex_t),
					(void *)offsetof(ac_vertex_t, st[0]));
#ifndef FX_FIXED_PIPELINE
	glUseProgramObjectARB(r_sprite_prog);
#endif
}

void r_finish_fx(void) {
	// bring the previous state back
#ifndef FX_FIXED_PIPELINE
	glUseProgramObjectARB(0);
#endif
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	// reenable writing to the depth buffer
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
#ifdef FX_FIXED_PIPELINE
	glColor4f(1, 1, 1, 1);
#endif

	OPENGL_EVENT_END();
}

void r_draw_fx(ac_vec4_t pos, float scale, float alpha, float angle) {
#ifdef FX_FIXED_PIPELINE
	static GLmatrix_t m;
	float s = sinf(angle);
	float c = cosf(angle);

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glPushMatrix();

	// cheap, cheated sprites
	glTranslatef(pos.f[0], pos.f[1], pos.f[2]);
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	// nullify the rotation part of the matrix
	memset(m, 0, sizeof(m[0]) * 3 * 4);
	m[0] = scale * c;
	m[1] = scale * s;
	m[4] = scale * -s;
	m[5] = scale * c;
	m[10] = scale;
	// reload the matrix
	glLoadMatrixf(m);

	glColor4f(1, 1, 1, alpha);
#else
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glMultiTexCoord3fv(GL_TEXTURE1, pos.f);
	glMultiTexCoord3f(GL_TEXTURE2, angle, alpha, scale);
#endif

	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, (void *)0);
	*r_vert_counter += 4;
	*r_tri_counter += 2;

#ifdef FX_FIXED_PIPELINE
	glPopMatrix();
#endif

	OPENGL_EVENT_END();
}

void r_draw_tracer(ac_vec4_t pos, ac_vec4_t dir, float scale) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glLineWidth(scale);
	dir = ac_vec_add(pos, ac_vec_mulf(dir, -scale));
	glColor4f(1, 1, 1, 1);
	glBegin(GL_LINES);
	glVertex3fv(pos.f);
	glVertex3fv(dir.f);
	glEnd();

	OPENGL_EVENT_END();
}

void r_destroy_fx(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glDeleteBuffersARB(2, r_fx_VBOs);
	glDeleteTextures(1, &r_fx_tex);

	OPENGL_EVENT_END();
}
