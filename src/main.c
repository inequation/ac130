// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Main module

#include <string.h>
#include <time.h>
#include "ac130.h"

int m_screen_width = 1024;
int m_screen_height = 768;
bool m_full_screen = true;

float m_terrain_LOD = 40.f;
bool m_FSAA = true;
bool m_compatshader = false;

static void parse_args(int argc, char *argv[]) {
	int i;

	if (argc < 2)
		return;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-w")) {
			m_full_screen = false;
			continue;
		}
		if (!strcmp(argv[i], "-r") && i + 1 < argc) {
			char buf[32];
			float aspect;

			strncpy(buf, argv[++i], sizeof(buf) - 1);
			char *x = strchr(buf, 'x');
			if (x == NULL)
				continue;
			*x++ = 0;
			m_screen_width = atoi(buf);
			m_screen_height = atoi(x);
			// sanitize the values
			if (m_screen_width <= 0)
				m_screen_width = 1024;
			if (m_screen_height <= 0)
				m_screen_height = 768;
			// enforce a 4:3 or 5:4 aspect ratio
			aspect = (float)m_screen_width / (float)m_screen_height;
			if (fabs(aspect - 4.f / 3.f) > 0.01
				&& fabs(aspect - 5.f / 4.f) > 0.01) {
				// OK, this aspect isn't right, shrink the window to get 4:3
				if (aspect > 1.334)
					m_screen_width = 1.33 * m_screen_height;
				else
					m_screen_height = 0.75 * m_screen_width;
			}
		}
		if (!strcmp(argv[i], "-noaa")) {
			m_FSAA = false;
			continue;
		}
		if (!strcmp(argv[i], "-c")) {
			m_compatshader = true;
			continue;
		}
		if (!strcmp(argv[i], "-t") && i + 1 < argc) {
			m_terrain_LOD = atof(argv[++i]);
			if (m_terrain_LOD < 1.f)
				m_terrain_LOD = 1.f;
			continue;
		}
	}
}

static bool initialize_controller(int index,
	SDL_GameController **controller, SDL_Haptic **rumbler) {
	*controller = SDL_GameControllerOpen(index);
	if (*controller) {
		SDL_Joystick *joystick = SDL_GameControllerGetJoystick(*controller);
		// also try getting the rumble device
		*rumbler = SDL_HapticOpenFromJoystick(joystick);
		// try the initializing the rumbler
		if (*rumbler && !SDL_HapticRumbleInit(*rumbler))
			SDL_HapticClose(*rumbler);
		printf("Opened controller #%d (%s) with%s rumble (%s)\n",
			index, SDL_GameControllerName(*controller), *rumbler ? "" : "out",
			SDL_GetError());
		return true;
	}
	return false;
}

static void destroy_controller(SDL_GameController **controller,
	SDL_Haptic **rumbler) {
	if (*controller)
		printf("Destroying controller %s\n",
			SDL_GameControllerName(*controller));
	if (*rumbler)
		SDL_HapticClose(*rumbler);
	if (*controller)
		SDL_GameControllerClose(*controller);
	*controller = NULL;
	*rumbler = NULL;
}

int main (int argc, char *argv[]) {
	Uint32		prevTime;	/// Time of the previous frame time in milliseconds.
	Uint32		curTime;	/// Time of the current frame time in milliseconds.
	float		frameTime;	/// \ref curTime - \ref prevTime / 1000 (in seconds)
	SDL_Event	event;
	ac_input_t	prevInput;
	ac_input_t	curInput;
	bool		done;
	uint		frameCount = 0;
	uint		vertCount = 0;
	uint		triCount = 0;
	uint		dpCount = 0;
	uint		cpCount = 0;
	uint		frameCountTime;
	int			index;
	SDL_GameController	*controller;
	SDL_Haptic			*rumbler;
	Sint16		controllerAxes[SDL_CONTROLLER_AXIS_MAX] = {0};

	parse_args(argc, argv);

	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}

	// initialize renderer
	if (!r_init(&vertCount, &triCount, &dpCount, &cpCount)) {
		fprintf(stderr, "Unable to init renderer\n");
		return 1;
	}
	extern SDL_Window	*r_screen;

	// initialize the system random number generator
	srand((uint)time(NULL));

	// set window caption to say that we're working
	SDL_SetWindowTitle(r_screen, "AC-130 - Generating resources, please wait...");

	// hide mouse cursor and grab input
	SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_bool grab =
#ifdef NDEBUG
        SDL_TRUE
#else
        m_full_screen ? SDL_TRUE : SDL_FALSE
#endif // NDEBUG
    ;
	SDL_SetWindowGrab(r_screen, grab);

	// make sure SDL cleans up before exit
	// FIXME: apparently, this is bad
	atexit(SDL_Quit);

	// clear out input structs
	memset(&prevInput, 0, sizeof(prevInput));
	memset(&curInput, 0, sizeof(curInput));

	// initialize game logic
	g_init();

	// update window caption to say that we're done generating stuff
	SDL_SetWindowTitle(r_screen, "AC-130");

	memset(&prevInput, 0, sizeof(prevInput));

	// initialize tick counter
	frameCountTime = SDL_GetTicks();
	// hardcode the first frame time at 20ms to get a bit more accurate results
	// of the FPS counter on the first FPS calculation
	prevTime = frameCountTime - 20;

#ifndef NDEBUG
	// grab the input in debug
	if (!grab) {
        grab = SDL_TRUE;
		SDL_SetWindowGrab(r_screen, grab);
	}
#endif

	// try using the first available game controller
	controller = NULL;
	rumbler = NULL;
	for (index = -1; index < SDL_NumJoysticks(); ++index) {
		if (SDL_IsGameController(index)
			&& initialize_controller(index, &controller, &rumbler))
			break;
	}

	// program main loop
	done = false;
	while (!done) {
		curTime = SDL_GetTicks();
		frameTime = (float)(curTime - prevTime) * 0.001;
		prevTime = curTime;

		memset(&curInput, 0, sizeof(curInput));
		// copy buttons from last frame in case there was no MOUSEBUTTONUP event
		curInput.flags |= prevInput.flags
			& (INPUT_MOUSE_LEFT | INPUT_MOUSE_RIGHT);
		// dispatch events
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				// exit if the window is closed
				case SDL_QUIT:
					done = true;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							done = true;
							break;
						case SDLK_f:
							curInput.flags |= INPUT_NEGATIVE;
							break;
						case SDLK_1:
							curInput.flags |= INPUT_1;
							break;
						case SDLK_2:
							curInput.flags |= INPUT_2;
							break;
						case SDLK_3:
							curInput.flags |= INPUT_3;
							break;
						case SDLK_p:
							curInput.flags |= INPUT_PAUSE;
							break;
#ifndef NDEBUG
						case SDLK_g:
							grab = !grab;
							SDL_SetWindowGrab(r_screen, grab);
                            SDL_SetRelativeMouseMode(grab);
							break;
#endif // NDEBUG
						default:	// shut up compiler
							break;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if (event.button.state == SDL_PRESSED)
						curInput.flags |= event.button.button == SDL_BUTTON_LEFT
							? INPUT_MOUSE_LEFT : INPUT_MOUSE_RIGHT;
					else
						curInput.flags &= event.button.button == SDL_BUTTON_LEFT
							? ~INPUT_MOUSE_LEFT : ~INPUT_MOUSE_RIGHT;
					break;
				case SDL_MOUSEMOTION:
					curInput.deltaX = event.motion.xrel;
					curInput.deltaY = event.motion.yrel;
					break;
				case SDL_CONTROLLERDEVICEADDED:
					if (!controller)
						initialize_controller(event.cdevice.which,
							&controller, &rumbler);
					break;
				case SDL_CONTROLLERDEVICEREMOVED:
					if (controller)
						destroy_controller(&controller, &rumbler);
					break;
				case SDL_CONTROLLERAXISMOTION:
					// special case for the triggers - map them to buttons
					if (/*event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT
						||*/ event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
						#define TRIGGER_AXIS_THRESHOLD	10000
						// neat little trick here with arithmetic on booleans :)
						int cross = (event.caxis.value > TRIGGER_AXIS_THRESHOLD)
							- (controllerAxes[event.caxis.axis]
								> TRIGGER_AXIS_THRESHOLD);
						if (cross > 0)
							curInput.flags |= INPUT_MOUSE_LEFT;
						else if (cross < 0)
							curInput.flags &= ~INPUT_MOUSE_LEFT;
					}
					controllerAxes[event.caxis.axis] = event.caxis.value;
					break;
				case SDL_CONTROLLERBUTTONDOWN:
				case SDL_CONTROLLERBUTTONUP:
					{
						ac_input_flags_t flag = 0;
						switch (event.cbutton.button) {
							case SDL_CONTROLLER_BUTTON_A:
								flag = INPUT_NEGATIVE;
								break;
							case SDL_CONTROLLER_BUTTON_Y:
								flag = INPUT_MOUSE_RIGHT;
								break;
							case SDL_CONTROLLER_BUTTON_START:
								flag = INPUT_PAUSE;
								break;
							case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
								flag = INPUT_1;
								break;
							case SDL_CONTROLLER_BUTTON_DPAD_UP:
								flag = INPUT_2;
								break;
							case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
								flag = INPUT_3;
								break;
						}
						if (event.cbutton.state == SDL_PRESSED)
							curInput.flags |= flag;
						else
							curInput.flags &= ~flag;
					}
					break;
			}
		}

		// synthesize game controller axis input
		if (controller) {
			curInput.deltaX = controllerAxes[SDL_CONTROLLER_AXIS_RIGHTX] / 2048;
			curInput.deltaY = controllerAxes[SDL_CONTROLLER_AXIS_RIGHTY] / 2048;
		}

		// show fps
		if (false && curTime - frameCountTime >= 2000) {
			float perFrameScale = 1.f / (float)frameCount;
			printf("%.0f FPS, %.0f tris/%.0f verts, "
					"%.0f/%.0f terrain patches culled (per frame)\n",
					(float)frameCount
						/ ((float)(curTime - frameCountTime) * 0.001),
					(float)triCount * perFrameScale,
					(float)vertCount * perFrameScale,
					(float)cpCount * perFrameScale,
					(float)(dpCount + cpCount) * perFrameScale);
			frameCountTime = curTime;
			frameCount = triCount = vertCount = dpCount = cpCount = 0;
		}

		g_frame(curTime, frameTime, &curInput);
		prevInput = curInput;
		frameCount++;

#if	0
		// bad performance simulation
		SDL_Delay(100);
#endif
	} // end main loop

	// show mouse cursor and release input
	SDL_SetWindowGrab(r_screen, SDL_FALSE);
    SDL_SetRelativeMouseMode(SDL_FALSE);

	// shut all subsystems down
	r_shutdown();
	g_shutdown();

	return 0;
}
