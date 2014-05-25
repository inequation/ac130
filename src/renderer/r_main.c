// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Main renderer module

#include "r_local.h"

SDL_Window	*r_screen;
SDL_GLContext  r_context;

// counters for measuring performance
uint		*r_tri_counter;
uint		*r_vert_counter;
uint		*r_visible_patch_counter;
uint		*r_culled_patch_counter;

// frustum planes
ac_vec4_t	r_frustum[6];

ac_vec4_t	r_viewpoint;

// FBO resources
uint		r_FBOs[2 + FRAME_TRACE];
uint		r_depth_RBO;
uint		r_2D_tex;
uint		r_frame_tex[1 + FRAME_TRACE];	///< 0 - current frame, > 0 -
											///< previous ones
uint		r_current_FBO = 1;

static void r_set_frustum(ac_vec4_t pos,
							ac_vec4_t fwd, ac_vec4_t right, ac_vec4_t up,
							float x, float y, float zNear, float zFar) {
	ac_vec4_t v1, v2;

	// culling debug (look straight down to best see it at work)
#if 0
	x *= 0.5;
	y *= 0.5;
#endif

	// near plane
	r_frustum[0] = fwd;
	r_frustum[0].f[3] = ac_vec_dot(fwd, pos) + zNear;

	// far plane
	r_frustum[1] = ac_vec_mulf(fwd, -1.f);
	r_frustum[1].f[3] = -ac_vec_dot(fwd, pos) - zFar;

	// right plane
	// v1 = fwd * zNear + right * x + up * -y
	v1 = ac_vec_add(ac_vec_mulf(fwd, zNear),
			ac_vec_add(ac_vec_mulf(right, x), ac_vec_mulf(up, -y)));
	// v2 = p1 + up * 2y
	v2 = ac_vec_add(v1, ac_vec_mulf(up, 2.f * y));
	r_frustum[2] = ac_vec_normalize(ac_vec_cross(v2, v1));
	r_frustum[2].f[3] = ac_vec_dot(r_frustum[2], ac_vec_add(v1, pos));

	// left plane
	// v1 -= right * 2 * x
	v1 = ac_vec_add(v1, ac_vec_mulf(right, -2.f * x));
	// v2 -= right * 2 * x
	v2 = ac_vec_add(v2, ac_vec_mulf(right, -2.f * x));
	r_frustum[3] = ac_vec_normalize(ac_vec_cross(v1, v2));
	r_frustum[3].f[3] = ac_vec_dot(r_frustum[3], ac_vec_add(v1, pos));

	// top plane
	// v2 = v1 + right * 2 * x
	v1 = ac_vec_add(v2, ac_vec_mulf(right, 2.f * x));
	r_frustum[4] = ac_vec_normalize(ac_vec_cross(v2, v1));
	r_frustum[4].f[3] = ac_vec_dot(r_frustum[4], ac_vec_add(v1, pos));

	// bottom plane
	// v1 -= up * 2 * y
	v1 = ac_vec_add(v1, ac_vec_mulf(up, -2.f * y));
	// v2 -= up * 2 * y
	v2 = ac_vec_add(v2, ac_vec_mulf(up, -2.f * y));
	r_frustum[5] = ac_vec_normalize(ac_vec_cross(v1, v2));
	r_frustum[5].f[3] = ac_vec_dot(r_frustum[5], ac_vec_add(v1, pos));

	assert(ac_vec_dot(r_frustum[0], r_frustum[1]) < 0.f);
	assert(ac_vec_dot(r_frustum[2], r_frustum[3]) < 1.f);
	assert(ac_vec_dot(r_frustum[4], r_frustum[5]) < 1.f);
}

bool r_cull_sphere(ac_vec4_t p, float radius) {
	int i;
	for (i = 0; i < 6; i++) {
		if (ac_vec_dot(p, r_frustum[i]) + radius < r_frustum[i].f[3])
			// sphere is behind one of the planes
			return true;
	}
	return false;
}

cullResult_t r_cull_bbox(ac_vec4_t bounds[2]) {
	ac_vec4_t v;
	int i, x, y, z;
	bool intersect = false;

	for (i = 0; i < 6; i++) {
		// floating point magic! extract the sign bits
#define AS_INT(f)		(*(int *)(&(f)))
		x = (AS_INT(r_frustum[i].f[0]) & 0x80000000) >> 31;
		y = (AS_INT(r_frustum[i].f[1]) & 0x80000000) >> 31;
		z = (AS_INT(r_frustum[i].f[2]) & 0x80000000) >> 31;
#undef AS_INT
		// test the negative far point against the plane
		v = ac_vec_set(
				bounds[1 - x].f[0],
				bounds[1 - y].f[1],
				bounds[1 - z].f[2],
				0);
		if (ac_vec_dot(v, r_frustum[i]) < r_frustum[i].f[3])
			// negative far point behind plane -> box outside frustum
			return CR_OUTSIDE;
		// test the positive far point against the plane
		v = ac_vec_set(
				bounds[x].f[0],
				bounds[y].f[1],
				bounds[z].f[2],
				0);
		if (ac_vec_dot(v, r_frustum[i]) < r_frustum[i].f[3])
			intersect = true;
	}
	return intersect ? CR_INTERSECT : CR_INSIDE;
}

static bool r_init_FBO(void) {
	size_t i;
	GLenum status;

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// depth RBO initialization
	glGenRenderbuffersEXT(1, &r_depth_RBO);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, r_depth_RBO);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
		m_screen_width, m_screen_height);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

	// set frame textures up
	glGenTextures(sizeof(r_frame_tex) / sizeof(r_frame_tex[0]), r_frame_tex);
	glGenTextures(1, &r_2D_tex);
	glGenFramebuffersEXT(sizeof(r_FBOs) / sizeof(r_FBOs[0]), r_FBOs);
	for (i = 0; i < sizeof(r_FBOs) / sizeof(r_FBOs[0]); i++) {
		if (i == FBO_2D)
			glBindTexture(GL_TEXTURE_2D, r_2D_tex);
		else
			glBindTexture(GL_TEXTURE_2D, r_frame_tex[i - 1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		// don't need mipmaps
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		if (i == FBO_2D)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
				m_screen_width, m_screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				NULL);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
				m_screen_width, m_screen_height, 0, GL_RGB, GL_UNSIGNED_BYTE,
				NULL);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, r_FBOs[i]);
		// attach the texture to the colour attachment point
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, i == FBO_2D ? r_2D_tex : r_frame_tex[i - 1], 0);
		// attach the renderbuffer to the depth attachment point
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
			GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, r_depth_RBO);

		// check FBO status
		status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if(status != GL_FRAMEBUFFER_COMPLETE_EXT) {
			fprintf(stderr, "Incomplete frame buffer object #%zd: %s\n", i,
				status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT ?
					"attachment"
				: status == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT ?
					"dimensions"
				: status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT ?
					"draw buffer"
				: status == GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT ? "formats"
				: status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT ?
					"layer count"
				: status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT ?
					"layer targets"
				: status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT ?
					"missing attachment"
				: status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT ?
					"multisample"
				: status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT ?
					"read buffer"
				: "unsupported");

			OPENGL_EVENT_END();

			return false;
		}
		// make sure all buffers are cleared before proceeding
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	// switch back to window-system-provided framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	OPENGL_EVENT_END();

	return true;
}

bool r_init(uint *vcounter, uint *tcounter,
					uint *dpcounter, uint *cpcounter) {
	int i;
	float fogcolour[] = {0, 0, 0, 1};

	// we need this parameter to be squared anyway
	m_terrain_LOD *= m_terrain_LOD;

	// these GL attribs don't change regardless of matching visual search
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// stick to old GL 1.4 for now
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

    // attempt to find a matching visual
    for (i = 0; i < 16 && !r_screen; ++i) {
		// try disabling FSAA on odd iterations
		bool useFSAA = m_FSAA && (i % 2 == 0);
		// try decreasing colour precision to 16 bits on the 2nd half
		int colourbits = i < 8 ? 8 : 5;
		// try decreasing depth precision by 8 bits every 2 iterations
		int depthbits = 32 - (i % 8) / 2 * 8;

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, colourbits);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, colourbits);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, colourbits);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthbits);

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, useFSAA ? 1 : 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, useFSAA ? 4 : 0);

		printf("Trying visual: R%dG%dB%dD%d, MSAA %s\n",
			colourbits, colourbits, colourbits, depthbits, useFSAA ? "on" : "off");
		if (!(r_screen = SDL_CreateWindow("AC-130",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			m_screen_width, m_screen_height,
			SDL_WINDOW_OPENGL | (m_full_screen ? SDL_WINDOW_FULLSCREEN : 0)))) {
				fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		}
	}

	if (!r_screen) {
		fprintf(stderr, "Failed to find a supported GL visual\n");
		return false;
	}

	r_context = SDL_GL_CreateContext(r_screen);
	if (!r_context) {
		fprintf(stderr, "Failed to initialize the GL context\n");
		return false;
	}

	if (!vcounter || !tcounter || !dpcounter || !cpcounter)
		return false;

	r_vert_counter = vcounter;
	r_tri_counter = tcounter;
	r_visible_patch_counter = dpcounter;
	r_culled_patch_counter = cpcounter;

	// initialize the extension wrangler
	glewInit();
	// check for required features
	if (!GLEW_EXT_framebuffer_object) {
		fprintf(stderr, "Hardware does not support frame buffer objects\n");
		return false;
	}
	if (!GLEW_ARB_vertex_buffer_object) {
		fprintf(stderr, "Hardware does not support vertex buffer objects\n");
		return false;
	}
	if (!GLEW_ARB_multitexture) {
		fprintf(stderr, "Hardware does not support multitexturing\n");
		return false;
	}
	if (!GLEW_ARB_vertex_shader) {
		fprintf(stderr, "Hardware does not support vertex shaders\n");
		return false;
	}
	if (!GLEW_ARB_fragment_shader) {
		fprintf(stderr, "Hardware does not support fragment shaders\n");
		return false;
	}

	OPENGL_EVENT_BEGIN(0, "Initialization");

	// all geometry uses vertex and index arrays
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// initialize matrices
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// set face culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1, 1, 1, 1);

	// set up fog
	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogfv(GL_FOG_COLOR, fogcolour);
	glFogf(GL_FOG_DENSITY, 0.002);

	// set line smoothing
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	// initialize FBO
	if (!r_init_FBO())
		return false;

	// initialize shaders
	if (!r_create_shaders())
		return false;

	// generate resources
	r_create_terrain();
	r_create_props();
	r_create_fx();
	r_create_font();
	r_create_footmobile();

	OPENGL_EVENT_END();

	return true;
}

void r_shutdown(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	r_destroy_footmobile();
	r_destroy_font();
	r_destroy_fx();
	r_destroy_props();
	r_destroy_terrain();
	r_destroy_shaders();

	glDeleteFramebuffersEXT(sizeof(r_FBOs) / sizeof(r_FBOs[0]), r_FBOs);
	glDeleteRenderbuffersEXT(1, &r_depth_RBO);

	glDeleteTextures(sizeof(r_frame_tex) / sizeof(r_frame_tex[0]), r_frame_tex);
	glDeleteTextures(1, &r_2D_tex);

	OPENGL_EVENT_END();

	SDL_GL_DeleteContext(r_context);
	SDL_DestroyWindow(r_screen);

	// close SDL down
	SDL_QuitSubSystem(SDL_INIT_VIDEO);	// FIXME: this shuts input down as well
}

void r_start_scene(int time, ac_viewpoint_t *vp) {
	static int lastTime = 0;
	static GLmatrix_t m = {
		1, 0, 0, 0,	// 0
		0, 1, 0, 0,	// 4
		0, 0, 1, 0,	// 8
		0, 0, 0, 1	// 12
	};
	static const double zNear = 2.0, zFar = 800.0;
	double x, y;
	float cy, cp, sy, sp;
	ac_vec4_t fwd, right, up;

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// scroll the FBO every 27 ms
	if (time - lastTime >= 27) {
		r_current_FBO = (r_current_FBO + 1)
			% (sizeof(r_FBOs) / sizeof(r_FBOs[0]));
		if (r_current_FBO == FBO_2D)
			r_current_FBO++;
		lastTime = time;
	}

	// activate FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, r_FBOs[r_current_FBO]);

	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// don't bother going further if we don't have a valid viewpoint
	if (!vp)
		return;

	// flick some switches for the 3D rendering
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);
	glEnable(GL_CULL_FACE);

	r_viewpoint = vp->origin;

	// set up the GL projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	x = zNear * tan(vp->fov);
	y = zNear * tan(vp->fov * 0.75);
	glFrustum(-x, x, -y, y, zNear, zFar);

	// set camera matrix
	glMatrixMode(GL_MODELVIEW);

	cy = cosf(vp->angles[0]);
	sy = sinf(vp->angles[0]);
	cp = cosf(vp->angles[1]);
	sp = sinf(vp->angles[1]);
	// column 1
	m[0] = cy;
	m[1] = sp * sy;
	m[2] = cp * sy;
	//m[3] = 0;
	// column 2
	//m[4] = 0;
	m[5] = cp;
	m[6] = -sp;
	//m[7] = 0;
	// column 3
	m[8] = -sy;
	m[9] = sp * cy;
	m[10] = cp * cy;
	//m[11] = 0;
	// column 4
	m[12] = -vp->origin.f[0] * m[0]
		- vp->origin.f[1] * m[4]
		- vp->origin.f[2] * m[8];
	m[13] = -vp->origin.f[0] * m[1]
		- vp->origin.f[1] * m[5]
		- vp->origin.f[2] * m[9];
	m[14] = -vp->origin.f[0] * m[2]
		- vp->origin.f[1] * m[6]
		- vp->origin.f[2] * m[10];
	//m[15] = 1;
	glLoadMatrixf(m);

	// calculate frustum planes
	fwd = ac_vec_set(-m[2], -m[6], -m[10], 0);
	right = ac_vec_set(m[0], m[4], m[8], 0);
	up = ac_vec_set(m[1], m[5], m[9], 0);
	r_set_frustum(vp->origin, fwd, right, up, x, y, zNear, zFar);

	// draw terrain
	r_draw_terrain();

	// draw props
	r_draw_props();

	glDisable(GL_FOG);

	OPENGL_EVENT_END();
}

void r_finish_3D(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// switch to the 2D FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, r_FBOs[FBO_2D]);

	glClear(GL_COLOR_BUFFER_BIT);

	// depth testing and backface culling are useless, or even harmful in 2D
	// drawing
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);

	// switch to 2D rendering
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	OPENGL_EVENT_END();
}

void r_finish_2D(void) {
	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glDisable(GL_BLEND);

	// switch back to system-provided FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// prepare projection matrix for compositing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	OPENGL_EVENT_END();
}

void r_composite(float negative, float contrast) {
	int i;

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgramObjectARB(r_comp_prog);
	glUniform1fARB(r_comp_neg, negative);
	glUniform1fARB(r_comp_contrast, contrast);

	glActiveTextureARB(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, r_2D_tex);
	// mind you that the frame texture #0 is attached to FBO #1
	//printf("Frames: ");
	for (i = 0; i < 1 + FRAME_TRACE; i++) {
		glActiveTextureARB(GL_TEXTURE1 + i);
		glBindTexture(GL_TEXTURE_2D, r_frame_tex[(r_current_FBO - 1 + i)
						% (1 + FRAME_TRACE)]);
		//printf("%d=%d ", i, (r_currentFBO - 1 + i) % (1 + FRAME_TRACE));
	}
	//printf("\n");
	glActiveTextureARB(GL_TEXTURE0);

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(0, 1);
	glVertex2f(0, 1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 0);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glEnd();

	glUseProgramObjectARB(0);

	OPENGL_EVENT_END();

	// dump everything to screen
	SDL_GL_SwapWindow(r_screen);
}
