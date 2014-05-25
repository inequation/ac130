// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Shaders module

#include "r_local.h"

// helper define for terrain vertex shader
#define TERRAIN_HS_VECS	(TERRAIN_PATCH_SIZE * TERRAIN_PATCH_SIZE + 3) / 4

// embed shader sources
#define STRINGIFY(A)  		#A
#define SHADER_DEFINE(A)	"#define " #A " " STRINGIFY(A) "\n"
#include "../shaders/terrain_vs.glsl"
#include "../shaders/terrain_fs.glsl"
#include "../shaders/prop_vs.glsl"
#include "../shaders/prop_fs.glsl"
#include "../shaders/sprite_vs.glsl"
#include "../shaders/sprite_fs.glsl"
#include "../shaders/footmobile_vs.glsl"
#include "../shaders/font_fs.glsl"
#include "../shaders/compositor_vs.glsl"
#include "../shaders/compositor_fs.glsl"
#include "../shaders/compositor_compat_fs.glsl"

uint		r_ter_prog = 0;
uint		r_ter_vs = 0;
uint		r_ter_fs = 0;
int			r_ter_patch_params = -1;
int			r_ter_height_samples = -1;

uint		r_prop_prog = 0;
uint		r_prop_vs = 0;
uint		r_prop_fs = 0;

uint		r_sprite_prog = 0;
uint		r_sprite_vs = 0;
uint		r_sprite_fs = 0;

uint		r_fmb_prog = 0;
uint		r_fmb_vs = 0;
uint		r_fmb_fs = 0;

uint		r_font_prog = 0;
uint		r_font_vs = 0;
uint		r_font_fs = 0;

uint		r_comp_prog = 0;
uint		r_comp_vs = 0;
uint		r_comp_fs = 0;
int			r_comp_frames = -1;
int			r_comp_neg = -1;
int			r_comp_contrast = -1;

static bool r_shader_check(GLuint obj, GLenum what,
		const char *id, char *desc) {
	int retval, loglen;

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);
	OPENGL_EVENT(0, desc);

	glGetObjectParameterivARB(obj, what, &retval);
	glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &loglen);
	if (retval != GL_TRUE || loglen > 3) {
		char *p = malloc(loglen + 1);
		glGetInfoLogARB(obj, loglen, NULL, p);
		fprintf(stderr, "%s %s %s:\n%s\n", id, desc,
			retval == GL_TRUE ? "warning" : "error", p);
		free(p);
		if (retval != GL_TRUE) {
			OPENGL_EVENT_END();

			return false;
		}
	}

	OPENGL_EVENT_END();

	return true;
}

static inline bool r_create_program(const char *id, const char *vss,
	const char *fss, uint *vs, uint *fs, uint *prog) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	*vs = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	*fs = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	// shut up compiler...
	glShaderSourceARB(*vs, 1, (const char **)&vss, NULL);
	glShaderSourceARB(*fs, 1, (const char **)&fss, NULL);

	// compile vertex shader
	glCompileShaderARB(*vs);
	if (!r_shader_check(*vs, GL_OBJECT_COMPILE_STATUS_ARB, id,
		"vertex shader compilation")) {
		OPENGL_EVENT_END();

		return false;
	}

	// compile fragment shader
	glCompileShaderARB(*fs);
	if (!r_shader_check(*fs, GL_OBJECT_COMPILE_STATUS_ARB, id,
		"fragment shader compilation")) {
		OPENGL_EVENT_END();

		return false;
	}

	// link the program together
	*prog = glCreateProgramObjectARB();
	glAttachObjectARB(*prog, *vs);
	glAttachObjectARB(*prog, *fs);
	glLinkProgramARB(*prog);
	if (!r_shader_check(*prog, GL_OBJECT_LINK_STATUS_ARB, id,
		"GPU program linking")) {
		OPENGL_EVENT_END();

		return false;
	}

	// validate the program
	glValidateProgramARB(*prog);
	if (!r_shader_check(*prog, GL_OBJECT_VALIDATE_STATUS_ARB, id,
		"GPU program validation")) {
		OPENGL_EVENT_END();

		return false;
	}

	OPENGL_EVENT_END();

	return true;
}

bool r_create_shaders(void) {
	int i, frames[1 + FRAME_TRACE];

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// create the terrain GPU program
	if (!r_create_program("Terrain", TERRAIN_VS, TERRAIN_FS,
		&r_ter_vs, &r_ter_fs, &r_ter_prog)) {
		OPENGL_EVENT_END();

		return false;
	}
	// create the compositor GPU program
	if (!r_create_program("Compositor", COMPOSITOR_VS,
		(m_compatshader ? COMPOSITOR_COMPAT_FS : COMPOSITOR_FS),
		&r_comp_vs, &r_comp_fs, &r_comp_prog))
	{
		// try the compat shader before definitely failing
		if (!m_compatshader) {
			fprintf(stderr, "Failed compositor shader compilation, "
				"falling back to compat\n");
			if (!r_create_program("Compositor", COMPOSITOR_VS,
				COMPOSITOR_COMPAT_FS, &r_comp_vs, &r_comp_fs, &r_comp_prog)) {
				OPENGL_EVENT_END();

				return false;
			}
		}
		else
		{
			OPENGL_EVENT_END();

			return false;
		}
	}
	// create the font GPU program
	if (!r_create_program("Font", COMPOSITOR_VS, FONT_FS,
		&r_font_vs, &r_font_fs, &r_font_prog)) {
		OPENGL_EVENT_END();

		return false;
	}
	// create the prop GPU program
	if (!r_create_program("Prop", PROP_VS, PROP_FS,
		&r_prop_vs, &r_prop_fs, &r_prop_prog)) {
		OPENGL_EVENT_END();

		return false;
	}
	// create the sprite GPU program
	if (!r_create_program("Sprite", SPRITE_VS, SPRITE_FS,
		&r_sprite_vs, &r_sprite_fs, &r_sprite_prog)) {
		OPENGL_EVENT_END();

		return false;
	}
	// create the footmobile GPU program
	if (!r_create_program("Footmobile", FOOTMOBILE_VS, FONT_FS,
		&r_fmb_vs, &r_fmb_fs, &r_fmb_prog)) {
		OPENGL_EVENT_END();

		return false;
	}

	// set the terrain shader up
	glUseProgramObjectARB(r_ter_prog);
	if ((i = glGetUniformLocationARB(r_ter_prog, "terTex")) < 0) {
		fprintf(stderr, "Failed to find terrain texture uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	glUniform1iARB(i, 0);
	if ((i = glGetUniformLocationARB(r_ter_prog, "constParams")) < 0) {
		fprintf(stderr, "Failed to find constant params uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	glUniform2fARB(i, HEIGHTMAP_SIZE, HEIGHT_SCALE);
	if ((r_ter_patch_params = glGetUniformLocationARB(r_ter_prog,
		"patchParams")) < 0) {
		fprintf(stderr, "Failed to find per-patch params uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	if ((r_ter_height_samples = glGetUniformLocationARB(r_ter_prog,
		"heightSamples")) < 0) {
		fprintf(stderr, "Failed to find height samples uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}

	// set the prop shader up
	glUseProgramObjectARB(r_prop_prog);
	if ((i = glGetUniformLocationARB(r_prop_prog, "propTex")) < 0) {
		fprintf(stderr, "Failed to find prop texture uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	glUniform1iARB(i, 0);

	// set the sprite shader up
	glUseProgramObjectARB(r_sprite_prog);
	if ((i = glGetUniformLocationARB(r_sprite_prog, "spriteTex")) < 0) {
		fprintf(stderr, "Failed to find sprite texture uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	glUniform1iARB(i, 0);

	// set the footmobile shader up
	glUseProgramObjectARB(r_fmb_prog);
	if ((i = glGetUniformLocationARB(r_fmb_prog, "fontTex")) < 0) {
		fprintf(stderr, "Failed to find footmobile texture uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	glUniform1iARB(i, 0);

	// set the font shader up
	glUseProgramObjectARB(r_font_prog);
	if ((i = glGetUniformLocationARB(r_font_prog, "fontTex")) < 0) {
		fprintf(stderr, "Failed to find font texture uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	glUniform1iARB(i, 0);

	// find uniform locations
	glUseProgramObjectARB(r_comp_prog);
	if ((i = glGetUniformLocationARB(r_comp_prog, "overlay")) < 0) {
		fprintf(stderr, "Failed to find overlay uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	glUniform1iARB(i, 0);
	if ((r_comp_frames = glGetUniformLocationARB(r_comp_prog, "frames")) < 0) {
		fprintf(stderr, "Failed to find frames uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	if ((r_comp_neg = glGetUniformLocationARB(r_comp_prog, "negative")) < 0) {
		fprintf(stderr, "Failed to find negative uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	if ((r_comp_contrast = glGetUniformLocationARB(r_comp_prog, "cont")) < 0) {
		fprintf(stderr, "Failed to find contrast uniform variable\n");

		OPENGL_EVENT_END();

		return false;
	}
	// fill the frames array; frames at GL_TEXTURE1 + i
	for (i = 0; i < 1 + FRAME_TRACE; i++)
		frames[i] = i + 1;
	glUniform1ivARB(r_comp_frames, 1 + FRAME_TRACE, frames);

	glUseProgramObjectARB(0);

	OPENGL_EVENT_END();

	return true;
}

static inline void r_destroy_program(uint prog, uint vs, uint fs) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glDetachObjectARB(prog, vs);
	glDetachObjectARB(prog, fs);
	glDeleteObjectARB(vs);
	glDeleteObjectARB(fs);
	glDeleteObjectARB(prog);

	OPENGL_EVENT_END();
}

void r_destroy_shaders(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	r_destroy_program(r_comp_prog, r_comp_vs, r_comp_fs);
	r_destroy_program(r_font_prog, r_font_vs, r_font_fs);
	r_destroy_program(r_sprite_prog, r_sprite_vs, r_sprite_fs);
	r_destroy_program(r_prop_prog, r_prop_vs, r_prop_fs);
	r_destroy_program(r_ter_prog, r_ter_vs, r_ter_fs);

	OPENGL_EVENT_END();
}
