// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// 2D drawing module

#include "r_local.h"

// embed font texture
#define STRINGIFY(A)  #A
#include "../font.h"

uint		r_font_tex;

void r_create_font(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// generate texture
	glGenTextures(1, &r_font_tex);
	glBindTexture(GL_TEXTURE_2D, r_font_tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8,
				FONT_WIDTH, FONT_HEIGHT, 0,
				GL_LUMINANCE, GL_UNSIGNED_BYTE, FONT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

#if OPENGL_DEBUG
	if (GLEW_KHR_debug)
		glObjectLabel(GL_TEXTURE, r_font_tex, -1, "Font");
#endif

	OPENGL_EVENT_END();
}

#define GLYPH_W	(float)FONT_WIDTH / 13.f
#define GLYPH_H	(float)FONT_WIDTH / 13.f * (128.f / 75.f)
void r_draw_string(char *str, float ox, float oy, float scale) {
	static const float glyph_tw = GLYPH_W / (float)FONT_WIDTH;
	static const float glyph_th = GLYPH_H / (float)FONT_HEIGHT;
	// the glyph screen sizes should always be proportional to the 1024x768
	// resolution
	float glyph_sw = GLYPH_W / 1024.f * scale;
	float glyph_sh = GLYPH_H / 768.f * scale;

	int c;
	float x = ox, y = oy, s, t, dx = glyph_sw, dy = glyph_sh;
	bool revord = false;

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// handle align inversions
	if (ox < 0.f) {
		dx = -glyph_sw;
		x = ox = -ox + dx;
		revord = true;
	}
	if (oy < 0.f) {
		dy = -glyph_sh;
		y = oy = -oy + dy;
		revord = true;
	}

	glBindTexture(GL_TEXTURE_2D, r_font_tex);
	glUseProgramObjectARB(r_font_prog);
	glBegin(GL_QUADS);

#define LOOP_BODY(ptr)								\
	{												\
		if (*ptr == '\n') {							\
			y += dy;								\
			x = ox;									\
			continue;								\
		}											\
		c = *ptr - ' ';								\
		if (c <= 0 || c > 90) {						\
			x += dx;								\
			continue;								\
		}											\
		s = (float)(c % 13) * glyph_tw;				\
		t = (float)(c / 13) * glyph_th;				\
		glTexCoord2f(s, t);							\
		glVertex2f(x, y);							\
		glTexCoord2f(s + glyph_tw, t);				\
		glVertex2f(x + glyph_sw, y);				\
		glTexCoord2f(s + glyph_tw, t + glyph_th);	\
		glVertex2f(x + glyph_sw, y + glyph_sh);		\
		glTexCoord2f(s, t + glyph_th);				\
		glVertex2f(x, y + glyph_sh);				\
		x += dx;									\
	}

	if (revord) {
		char *p;
		for (p = strchr(str, 0); p >= str; p--)
			LOOP_BODY(p)
	} else {
		for (; *str; str++)
			LOOP_BODY(str)
	}

#undef LOOP_BODY

	glEnd();
	glUseProgramObjectARB(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	OPENGL_EVENT_END();
}

void r_draw_lines(float pts[][2], uint num_pts, float width) {
	uint i;

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// need to scale line width because it's absolute in pixels, and we need it
	// to be relative to window size
	glLineWidth(width * (float)m_screen_width / 1024.f);
	glBegin(GL_LINES);
	for (i = 0; i < num_pts; i++)
		glVertex2fv(pts[i]);
	glEnd();

	OPENGL_EVENT_END();
}

void r_destroy_font(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glDeleteTextures(1, &r_font_tex);

	OPENGL_EVENT_END();
}
