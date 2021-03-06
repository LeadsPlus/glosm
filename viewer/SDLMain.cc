/*
 * Copyright (C) 2010-2012 Dmitry Marakasov
 *
 * This file is part of glosm.
 *
 * glosm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * glosm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with glosm.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "GlosmViewer.hh"

#include <glosm/util/gl.h>

#include <SDL/SDL.h>
#include <SDL/SDL_keysym.h>
#if defined(WITH_GLES)
#	include <SDL_gles.h>
#else
#	define NO_SDL_GLEXT
#	include <SDL/SDL_opengl.h>
#endif
#if defined(WIN32)
#	include <winuser.h>
#endif

#include <cstdio>

class GlosmViewerImpl : public GlosmViewer {
protected:
	bool ignoremouse_;

public:
	GlosmViewerImpl() : ignoremouse_(true) {
	}

	virtual void MouseMove(int x, int y) {
		if (!ignoremouse_)
			GlosmViewer::MouseMove(x, y);
	}

protected:
	virtual void WarpCursor(int x, int y) {
		ignoremouse_ = true;
		SDL_WarpMouse(x, y);
		ignoremouse_ = false;
	}

	virtual void Flip() {
#if defined(WITH_GLES)
		SDL_GLES_SwapBuffers();
#else
		SDL_GL_SwapBuffers();
#endif
	}

	virtual void ShowCursor(bool show) {
#if defined(WITH_TOUCHPAD)
		SDL_ShowCursor(SDL_DISABLE);
#else
		SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
#endif
	}
};

#if defined(WITH_GLES)
SDL_GLES_Context* gles_context = 0;
#endif

GlosmViewerImpl app;

void Reshape(int w, int h) {
#if !defined(WITH_GLES)
	if (SDL_SetVideoMode(w, h, 0, SDL_OPENGL | SDL_RESIZABLE | SDL_HWSURFACE) == NULL)
		throw Exception() << "SDL_SetVideoMode failed: " << (const char*)SDL_GetError();
#endif

	app.Resize(w, h);
}

void KeyDown(SDLKey key) {
	if (key < 0x100)
		app.KeyDown(key);
	else
		switch(key) {
		case SDLK_UP: app.KeyDown(GlosmViewer::KEY_UP); break;
		case SDLK_DOWN: app.KeyDown(GlosmViewer::KEY_DOWN); break;
		case SDLK_LEFT: app.KeyDown(GlosmViewer::KEY_LEFT); break;
		case SDLK_RIGHT: app.KeyDown(GlosmViewer::KEY_RIGHT); break;
		case SDLK_KP_PLUS: app.KeyDown('+'); break;
		case SDLK_KP_MINUS: app.KeyDown('-'); break;
		case SDLK_LSHIFT: case SDLK_RSHIFT: app.KeyDown(GlosmViewer::KEY_SHIFT); break;
		case SDLK_LCTRL: case SDLK_RCTRL: app.KeyDown(GlosmViewer::KEY_CTRL); break;
		default: break;
		}
}

void KeyUp(SDLKey key) {
	if (key < 0x100)
		app.KeyUp(key);
	else
		switch(key) {
		case SDLK_UP: app.KeyUp(GlosmViewer::KEY_UP); break;
		case SDLK_DOWN: app.KeyUp(GlosmViewer::KEY_DOWN); break;
		case SDLK_LEFT: app.KeyUp(GlosmViewer::KEY_LEFT); break;
		case SDLK_RIGHT: app.KeyUp(GlosmViewer::KEY_RIGHT); break;
		case SDLK_KP_PLUS: app.KeyUp('+'); break;
		case SDLK_KP_MINUS: app.KeyUp('-'); break;
		case SDLK_LSHIFT: case SDLK_RSHIFT: app.KeyUp(GlosmViewer::KEY_SHIFT); break;
		case SDLK_LCTRL: case SDLK_RCTRL: app.KeyUp(GlosmViewer::KEY_CTRL); break;
		default: break;
		}
}

void Cleanup() {
#if defined(WITH_GLES)
	if (gles_context)
		SDL_GLES_DeleteContext(gles_context);
#endif
	SDL_Quit();
}

int GetEvents() {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		int ret = 1;
		switch(event.type) {
		case SDL_QUIT:
			return 0;
		case SDL_KEYDOWN:
			KeyDown(event.key.keysym.sym);
			break;
		case SDL_KEYUP:
			KeyUp(event.key.keysym.sym);
			break;
		case SDL_MOUSEMOTION:
			app.MouseMove(event.motion.x, event.motion.y);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			{
				int b = GlosmViewer::BUTTON_LEFT;
				switch (event.button.button) {
				case SDL_BUTTON_LEFT: b = GlosmViewer::BUTTON_LEFT; break;
				case SDL_BUTTON_RIGHT: b = GlosmViewer::BUTTON_RIGHT; break;
				case SDL_BUTTON_MIDDLE: b = GlosmViewer::BUTTON_MIDDLE; break;
				}
				app.MouseButton(b, event.button.state == SDL_PRESSED, event.button.x, event.button.y);
			}
			break;
		case SDL_VIDEORESIZE:
			Reshape(event.resize.w, event.resize.h);
			break;
		default:
			break;
		}

		if (ret == 0)
			return 0;
	}

	return 1;
}

int real_main(int argc, char** argv) {
	app.Init(argc, argv);

	/* glut init */
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		throw Exception() << "Couldn't initialize SDL: " << (const char*)SDL_GetError();

	SDL_WM_SetCaption("glosm", "glosm");

	atexit(Cleanup);

#if !defined(WITH_GLES)
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	try {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
		Reshape(800, 600);
	} catch (Exception& e) {
		fprintf(stderr, "warning: %s, retrying without multisampling\n", e.what());
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		Reshape(800, 600);
	}
#else
	/* Using fixed resolution for N900
	 * it should be detected on the fly instead */
	SDL_SetVideoMode(800, 480, 0, SDL_SWSURFACE | SDL_FULLSCREEN);

	SDL_GLES_Init(SDL_GLES_VERSION_1_1);

	SDL_GLES_SetAttribute(SDL_GLES_DEPTH_SIZE, 24);

	gles_context = SDL_GLES_CreateContext();

	SDL_GLES_MakeCurrent(gles_context);

	Reshape(800, 480);
#endif

	SDL_EnableKeyRepeat(0, 0);

	app.InitGL();

	/* main loop */
	while (GetEvents())
		app.Render();

	return 0;
}

int main(int argc, char** argv) {
	try {
		return real_main(argc, argv);
	} catch (std::exception &e) {
#if defined(WIN32)
		MessageBox(NULL, e.what(), "Fatal error", MB_OK | MB_ICONERROR);
#else
		fprintf(stderr, "Fatal error: %s\n", e.what());
#endif
    } catch (...) {
#if defined(WIN32)
		MessageBox(NULL, "Unknown exception", "Fatal error", MB_OK | MB_ICONERROR);
#else
		fprintf(stderr, "Fatal error: unknown exception\n");
#endif
	}

	return 1;
}
