// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// Terrain heightmap viewer app

#include "../ac130.h"

void g_loading_tick(void) {
	// placeholder for the program to link properly
}

int main(int argc, char **argv) {
	// initialize SDL video
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf ("Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}

	// make sure SDL cleans up before exit
	atexit (SDL_Quit);

	// create a new window
	SDL_Surface *screen = SDL_SetVideoMode(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, 16,
	                                        SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (!screen) {
		printf ("Unable to set 640x480 video: %s\n", SDL_GetError());
		return 1;
	}

	SDL_WM_SetCaption("Generating heightmap...", "Terrain viewer");
	gen_terrain(0xDEADBEEF);
	// load the heightmap into a surface
	SDL_Surface *bmp = SDL_CreateRGBSurfaceFrom(
		gen_heightmap, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, 8,
		HEIGHTMAP_SIZE, 0xFF, 0xFF, 0xFF, 0);
	SDL_WM_SetCaption("Terrain viewer", "Terrain viewer");

	// centre the bitmap on screen
	SDL_Rect dstrect;
	dstrect.x = (screen->w - bmp->w) / 2;
	dstrect.y = (screen->h - bmp->h) / 2;

	// program main loop
	bool done = false;
	while (!done) {
		// message processing loop
		SDL_Event event;
		while (SDL_PollEvent (&event)) {
			// check for messages
			switch (event.type) {
					// exit if the window is closed
				case SDL_QUIT:
					done = true;
					break;

					// check for keypresses
				case SDL_KEYDOWN: {
						// exit if ESCAPE is pressed
						if (event.key.keysym.sym == SDLK_ESCAPE)
							done = true;
						break;
					}
			} // end switch
		} // end of message processing

		// DRAWING STARTS HERE

		// clear screen
		SDL_FillRect (screen, 0, SDL_MapRGB (screen->format, 0, 0, 0));

		// draw bitmap
		SDL_BlitSurface (bmp, 0, screen, &dstrect);

		// DRAWING ENDS HERE

		// finally, update the screen :)
		SDL_Flip (screen);
	} // end main loop

	// free loaded bitmap
	SDL_FreeSurface (bmp);

	// all is well ;)
	printf ("Exited cleanly\n");
	return 0;
}
