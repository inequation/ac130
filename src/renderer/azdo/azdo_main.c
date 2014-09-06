// AC-130 shooter
// Written by Leszek Godlewski <github@inequation.org>

// Main AZDO renderer module

#include "azdo_local.h"

extern SDL_Window	*r_screen;
SDL_GLContext  azdo_context;

// counters for measuring performance
uint		*azdo_tri_counter;
uint		*azdo_vert_counter;
uint		*azdo_visible_patch_counter;
uint		*azdo_culled_patch_counter;

// frustum planes
ac_vec4_t	azdo_frustum[6];

ac_vec4_t	azdo_viewpoint;

// FBO resources
uint		azdo_FBOs[2 + FRAME_TRACE];
uint		azdo_depth_RBO;
uint		azdo_2D_tex;
uint		azdo_frame_tex[1 + FRAME_TRACE];	///< 0 - current frame, > 0 -
												///< previous ones
uint		azdo_current_FBO = 1;

#if OPENGL_DEBUG
static void azdo_debug_callback(GLenum source, GLenum type, GLuint id,
								GLenum severity, GLsizei length,
								const GLchar *message, GLvoid *user_param) {
	const char *source_str = "unknown";
	const char *type_str = "unknown";
	const char *severity_str = "unknown";

	// shut up, compiler
	(void)length;
	(void)user_param;

	switch (source)	{
	case GL_DEBUG_SOURCE_API:				source_str = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		source_str = "window system"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:	source_str = "shader cmplr"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:		source_str = "3rd party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:		source_str = "app"; break;
	case GL_DEBUG_SOURCE_OTHER:				source_str = "other"; break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:				type_str = "error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	type_str = "deprecated bhvr"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	type_str = "undef bhvr"; break;
	case GL_DEBUG_TYPE_PORTABILITY:			type_str = "portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:			type_str = "perf"; break;
	case GL_DEBUG_TYPE_OTHER:				type_str = "other"; break;
	case GL_DEBUG_TYPE_MARKER:				type_str = "marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:			type_str = "group push"; break;
	case GL_DEBUG_TYPE_POP_GROUP:			type_str = "group pop"; break;
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:			severity_str = "high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:			severity_str = "medium"; break;
	case GL_DEBUG_SEVERITY_LOW:				severity_str = "low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:	severity_str = "info"; break;
	}

	printf("OpenGL %s severity %s message #%d from %s: %s\n",
		severity_str, type_str, id, source_str, message);

#ifndef NDEBUG
	// break on errors
	if (type == GL_DEBUG_TYPE_ERROR)
	#ifdef WIN32
		DebugBreak();
	#else
		raise(SIGTRAP);
	#endif
#endif
}
#endif

static void azdo_set_frustum(ac_vec4_t pos,
							ac_vec4_t fwd, ac_vec4_t right, ac_vec4_t up,
							float x, float y, float zNear, float zFar) {
	ac_vec4_t v1, v2;

	// culling debug (look straight down to best see it at work)
#if 0
	x *= 0.5;
	y *= 0.5;
#endif

	// near plane
	azdo_frustum[0] = fwd;
	azdo_frustum[0].f[3] = ac_vec_dot(fwd, pos) + zNear;

	// far plane
	azdo_frustum[1] = ac_vec_mulf(fwd, -1.f);
	azdo_frustum[1].f[3] = -ac_vec_dot(fwd, pos) - zFar;

	// right plane
	// v1 = fwd * zNear + right * x + up * -y
	v1 = ac_vec_add(ac_vec_mulf(fwd, zNear),
			ac_vec_add(ac_vec_mulf(right, x), ac_vec_mulf(up, -y)));
	// v2 = p1 + up * 2y
	v2 = ac_vec_add(v1, ac_vec_mulf(up, 2.f * y));
	azdo_frustum[2] = ac_vec_normalize(ac_vec_cross(v2, v1));
	azdo_frustum[2].f[3] = ac_vec_dot(azdo_frustum[2], ac_vec_add(v1, pos));

	// left plane
	// v1 -= right * 2 * x
	v1 = ac_vec_add(v1, ac_vec_mulf(right, -2.f * x));
	// v2 -= right * 2 * x
	v2 = ac_vec_add(v2, ac_vec_mulf(right, -2.f * x));
	azdo_frustum[3] = ac_vec_normalize(ac_vec_cross(v1, v2));
	azdo_frustum[3].f[3] = ac_vec_dot(azdo_frustum[3], ac_vec_add(v1, pos));

	// top plane
	// v2 = v1 + right * 2 * x
	v1 = ac_vec_add(v2, ac_vec_mulf(right, 2.f * x));
	azdo_frustum[4] = ac_vec_normalize(ac_vec_cross(v2, v1));
	azdo_frustum[4].f[3] = ac_vec_dot(azdo_frustum[4], ac_vec_add(v1, pos));

	// bottom plane
	// v1 -= up * 2 * y
	v1 = ac_vec_add(v1, ac_vec_mulf(up, -2.f * y));
	// v2 -= up * 2 * y
	v2 = ac_vec_add(v2, ac_vec_mulf(up, -2.f * y));
	azdo_frustum[5] = ac_vec_normalize(ac_vec_cross(v1, v2));
	azdo_frustum[5].f[3] = ac_vec_dot(azdo_frustum[5], ac_vec_add(v1, pos));

	assert(ac_vec_dot(azdo_frustum[0], azdo_frustum[1]) < 0.f);
	assert(ac_vec_dot(azdo_frustum[2], azdo_frustum[3]) < 1.f);
	assert(ac_vec_dot(azdo_frustum[4], azdo_frustum[5]) < 1.f);
}

bool azdo_cull_sphere(ac_vec4_t p, float radius) {
	int i;
	for (i = 0; i < 6; i++) {
		if (ac_vec_dot(p, azdo_frustum[i]) + radius < azdo_frustum[i].f[3])
			// sphere is behind one of the planes
			return true;
	}
	return false;
}

cullResult_t azdo_cull_bbox(ac_vec4_t bounds[2]) {
	ac_vec4_t v;
	int i, x, y, z;
	bool intersect = false;

	for (i = 0; i < 6; i++) {
		// floating point magic! extract the sign bits
#define AS_INT(f)		(*(int *)(&(f)))
		x = (AS_INT(azdo_frustum[i].f[0]) & 0x80000000) >> 31;
		y = (AS_INT(azdo_frustum[i].f[1]) & 0x80000000) >> 31;
		z = (AS_INT(azdo_frustum[i].f[2]) & 0x80000000) >> 31;
#undef AS_INT
		// test the negative far point against the plane
		v = ac_vec_set(
				bounds[1 - x].f[0],
				bounds[1 - y].f[1],
				bounds[1 - z].f[2],
				0);
		if (ac_vec_dot(v, azdo_frustum[i]) < azdo_frustum[i].f[3])
			// negative far point behind plane -> box outside frustum
			return CR_OUTSIDE;
		// test the positive far point against the plane
		v = ac_vec_set(
				bounds[x].f[0],
				bounds[y].f[1],
				bounds[z].f[2],
				0);
		if (ac_vec_dot(v, azdo_frustum[i]) < azdo_frustum[i].f[3])
			intersect = true;
	}
	return intersect ? CR_INTERSECT : CR_INSIDE;
}

static bool azdo_init_FBO(void) {
	size_t i;
	GLenum status;

	OPENGL_EVENT_BEGIN(0, __PRETTY_FUNCTION__);

	// depth RBO initialization
	glGenRenderbuffers(1, &azdo_depth_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, azdo_depth_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
		m_screen_width, m_screen_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// set frame textures up
	glGenTextures(sizeof(azdo_frame_tex) / sizeof(azdo_frame_tex[0]), azdo_frame_tex);
	glGenTextures(1, &azdo_2D_tex);
	glGenFramebuffers(sizeof(azdo_FBOs) / sizeof(azdo_FBOs[0]), azdo_FBOs);
	for (i = 0; i < sizeof(azdo_FBOs) / sizeof(azdo_FBOs[0]); i++) {
		if (i == FBO_2D) {
#if OPENGL_DEBUG
			if (GLEW_KHR_debug)
				glObjectLabel(GL_TEXTURE, azdo_2D_tex, -1, "2D overlay RT");
#endif
			glBindTexture(GL_TEXTURE_2D, azdo_2D_tex);
		} else {
#if OPENGL_DEBUG
			if (GLEW_KHR_debug)
				glObjectLabel(GL_TEXTURE, azdo_frame_tex[i - 1], -1, "Frame RT");
#endif
			glBindTexture(GL_TEXTURE_2D, azdo_frame_tex[i - 1]);
		}
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

		glBindFramebuffer(GL_FRAMEBUFFER, azdo_FBOs[i]);
		// attach the texture to the colour attachment point
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, i == FBO_2D ? azdo_2D_tex : azdo_frame_tex[i - 1], 0);
		// attach the renderbuffer to the depth attachment point
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, azdo_depth_RBO);

		// check FBO status
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status != GL_FRAMEBUFFER_COMPLETE) {
			fprintf(stderr, "Incomplete frame buffer object #%zd: %s\n", i,
				status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ?
					"attachment"
				: status == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS ?
					"dimensions"
				: status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER ?
					"draw buffer"
				: status == GL_FRAMEBUFFER_INCOMPLETE_FORMATS ? "formats"
				: status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT ?
					"layer count"
				: status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS ?
					"layer targets"
				: status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ?
					"missing attachment"
				: status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE ?
					"multisample"
				: status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER ?
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
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	OPENGL_EVENT_END();

	return true;
}

bool azdo_init(uint *vcounter, uint *tcounter,
					uint *dpcounter, uint *cpcounter) {
	int i;
	float fogcolour[] = {0, 0, 0, 1};
	SDL_DisplayMode desktopMode;
	const char *missingExt = NULL;

	// we need this parameter to be squared anyway
	m_terrain_LOD *= m_terrain_LOD;

	// process the screen resolution
	// get primary display mode for a fallback
	SDL_GetDesktopDisplayMode(0, &desktopMode);
	// sanitize the values
	if (m_screen_width <= 0)
		m_screen_width = desktopMode.w;
	if (m_screen_height <= 0)
		m_screen_height = desktopMode.h;

	// these GL attribs don't change regardless of matching visual search
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// OpenGL 4.2 for AZDO!
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	// core profile
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#if OPENGL_DEBUG
	// make it a debug context, if we want debugging
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

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

	azdo_context = SDL_GL_CreateContext(r_screen);
	if (!azdo_context) {
		fprintf(stderr, "Failed to initialize the GL context\n");
		return false;
	}

	if (!vcounter || !tcounter || !dpcounter || !cpcounter)
		return false;

	azdo_vert_counter = vcounter;
	azdo_tri_counter = tcounter;
	azdo_visible_patch_counter = dpcounter;
	azdo_culled_patch_counter = cpcounter;

	// initialize the extension wrangler
	glewInit();
	// check for required extensions
#define CHECK_EXT(ext)	if (!missingExt && !GLEW_ ## ext)			\
							missingExt = #ext;
	CHECK_EXT(ARB_multi_draw_indirect)
	CHECK_EXT(ARB_sparse_texture)
#undef CHECK_EXT()
	if (missingExt) {
		fprintf(stderr, "Driver does not support required extension %s\n",
			missingExt);
		return false;
	}

#if OPENGL_DEBUG
	// register the debug callback handler, enable verbose output and squelch
	// all messages from the app
	if (GLEW_KHR_debug) {
		glDebugMessageCallback(azdo_debug_callback, NULL);
#if OPENGL_DEBUG > 1
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
			NULL, GL_TRUE);
#endif // OPENGL_DEBUG > 1
		glDebugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DONT_CARE,
			GL_DONT_CARE, 0, NULL, GL_FALSE);
	} else if (GLEW_ARB_debug_output) {
		glDebugMessageCallbackARB(azdo_debug_callback, NULL);
#if OPENGL_DEBUG > 1
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
			NULL, GL_TRUE);
#endif // OPENGL_DEBUG > 1
		glDebugMessageControlARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DONT_CARE,
			GL_DONT_CARE, 0, NULL, GL_FALSE);
	}
#endif

	OPENGL_EVENT_BEGIN(0, "Initialization");

	// set face culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	// set line smoothing
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	// initialize FBO
	if (!azdo_init_FBO())
		return false;

	// initialize shaders
	if (!azdo_create_shaders())
		return false;

	// generate resources
	azdo_create_terrain();
	azdo_create_props();
	azdo_create_fx();
	azdo_create_font();
	azdo_create_footmobile();

	OPENGL_EVENT_END();

	return true;
}

void azdo_shutdown(void) {
}
