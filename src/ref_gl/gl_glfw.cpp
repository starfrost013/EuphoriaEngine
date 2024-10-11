/*
Copyright (C) 2023-2024 starfrost

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// =============================================
// gl_glfw.c - Contains GLAD and GLFW glue stuff
// =============================================


#include <cassert>
#include "gl_local.hpp"

gl_state_t gl_state;			// The current renderer state

bool GL_Init();

void GLFW_Error(int32_t error_code, const char* error);
void GL_DestroyWindow();
void GL_WindowSizeChanged(GLFWwindow* window, int32_t width, int32_t height); 

extern cvar_t* vid_fullscreen;
extern cvar_t* vid_borderless;
extern cvar_t* vid_ref;

/*
** Vid_CreateWindow
*/

bool Vid_CreateWindow(int32_t width, int32_t height, bool fullscreen)
{
	// Monitor is NULL for windowed mode,
	// non-NULL for fullscreen
	GLFWmonitor* monitor = NULL;
	char window_title_buffer[MAX_QPATH] = { 0 }; // 256

	// Fullscreen in GLFW Zombono is borderless windowed by default, unless the user chose vid_fullscreen
	if (fullscreen)
	{
		gl_state.fullscreen = true;

		// determine if the user wanted borderless fullscreen or real fullscreen
		bool dedicated_fullscreen = (vid_fullscreen->value);

		if (dedicated_fullscreen)
		{
			monitor = glfwGetPrimaryMonitor(); // set 

			if (!monitor)
			{
				ri.Con_Printf(PRINT_ALL, "ref_gl::Vid_CreateWindow: Failed to obtain the primary monitor for dedicated fullscreen!");
				return false;
			}
		}
		else // fullscreen was specified, but not dedi
		{
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // dedicated fullscreen is a hint in GLFW
		}
	}

	if (!game_asset_path->string) // placeholder
	{
		snprintf(window_title_buffer, MAX_QPATH, "%s (Legacy OpenGL 1.5 " BUILD_PLATFORM " " BUILD_CONFIG ")", "Euphoria Game Engine - set gameinfo.json!");
	}
	else
	{
		snprintf(window_title_buffer, MAX_QPATH, "%s (Legacy OpenGL 1.5 " BUILD_PLATFORM " " BUILD_CONFIG ")", game_name->string);
	}

	gl_state.window = glfwCreateWindow(width, height, window_title_buffer, monitor, NULL);

	if (!gl_state.window)
	{
		ri.Con_Printf(PRINT_ALL, "ref_gl::Vid_CreateWindow: GLFW failed to create a window\n");
		return false;
	}
	
	// make the GLFW context current
	glfwMakeContextCurrent(gl_state.window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		ri.Con_Printf(PRINT_ALL, "ref_gl::Vid_CreateWindow: Failed to initialise GLAD\n");
		return false;
	}

	GL_SetResolution((int32_t)r_width->value, (int32_t)r_height->value);

	// set up callbacks
	glfwSetWindowSizeCallback(gl_state.window, GL_WindowSizeChanged);
	glfwSetWindowCloseCallback(gl_state.window, GL_ShutdownEverything);

	//TODO: vid_xpos, vid_ypos...
	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = (const char*)glGetString(GL_VENDOR);
	ri.Con_Printf(PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string);
	gl_config.renderer_string = (const char*)glGetString(GL_RENDERER);
	ri.Con_Printf(PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string);
	gl_config.version_string = (const char*)glGetString(GL_VERSION);
	ri.Con_Printf(PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string);
	gl_config.extensions_string = (const char*)glGetString(GL_EXTENSIONS);
	ri.Con_Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string);

	glfwSetErrorCallback(GLFW_Error);

	// let the sound and input subsystems know about the new window
	ri.Vid_ChangeResolution();

	// move the window if the user specified 
	if (!fullscreen)
	{
		int32_t vid_xpos = (int32_t)ri.Cvar_Get("vid_xpos", "0", 0)->value;
		int32_t vid_ypos = (int32_t)ri.Cvar_Get("vid_ypos", "0", 0)->value;

		glfwSetWindowPos(gl_state.window, vid_xpos, vid_ypos);
	}

	glfwSwapInterval(gl_vsync->value);

	return true;
}

void GL_WindowSizeChanged(GLFWwindow* window, int32_t width, int32_t height)
{
	GL_SetResolution(width, height);
}

void GL_SetResolution(int32_t width, int32_t height)
{
	glViewport(0, 0, width, height);
	vid.width = width;
	vid.height = height;
	// this will change the resolution
	ri.Cvar_SetValue("r_width", width);
	ri.Cvar_SetValue("r_height", height);
	glfwSetWindowSize(gl_state.window, width, height);
}

/*
** GL_SetMode
*/
rserr_t GL_SetMode(uint32_t* pwidth, uint32_t* pheight, bool fullscreen)
{
	uint32_t width = (uint32_t)r_width->value, height = (int32_t)r_height->value;
	const char *window_modes[] = { "Windowed", "Fullscreen"};

	// destroy the existing window
	if (gl_state.window)
	{
		bool need_to_destroy = (vid_borderless->modified
			|| vid_fullscreen->modified);

		if (!need_to_destroy)
		{
			GL_SetResolution(width, height);
		}
		else
		{
			GL_DestroyWindow();
		}
	}

	// if the window has been destroyed,
	// or it hasn't been created yet
	if (!gl_state.window)
	{
		// do a CDS if needed
		if (fullscreen)
		{
			ri.Con_Printf(PRINT_ALL, "...attempting fullscreen\n");
			ri.Con_Printf(PRINT_ALL, "...calling CDS: ");

			if (Vid_CreateWindow(width, height, true))
			{
				*pwidth = width;
				*pheight = height;

				gl_state.fullscreen = true;

				ri.Con_Printf(PRINT_ALL, "ok\n");
				return rserr_ok;
			}
			else
			{
				ri.Con_Printf(PRINT_ALL, " failed\n");
				ri.Con_Printf(PRINT_ALL, "...setting windowed mode\n");

				// gl_state.window is already null
				Vid_CreateWindow(width, height, false);

				*pwidth = width;
				*pheight = height;

				gl_state.fullscreen = false;

				return rserr_invalid_fullscreen;
			}
		}
		else
		{
			ri.Con_Printf(PRINT_ALL, "...setting windowed mode\n");

			*pwidth = width;
			*pheight = height;

			gl_state.fullscreen = false;

			if (!Vid_CreateWindow(width, height, false))
				return rserr_invalid_mode;
		}
	}

	return rserr_ok;
}

void GL_DestroyWindow()
{
	glfwDestroyWindow(gl_state.window);
	gl_state.window = NULL;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this deleting the rendering context, and killing the window.
** The state structure is also nulled out.
**
*/
void GL_Shutdown()
{
	GL_DestroyWindow();
	glfwTerminate();
}

// This routine tells the engine to shut down. Calls GL_Shutdown in the process of doing so
void GL_ShutdownEverything(GLFWwindow* window)
{
	ri.Com_Quit();
}

// This routine initialises GLAD and GLFW, and gets an OpenGL context.
bool GL_Init()
{
	if (glfwInit() == GLFW_FALSE)
	{
		ri.Con_Printf(PRINT_ALL, "GL_Init: Failed to initialise GLFW\n");
		return false;
	}

	// Set it to OpenGL 1.5, compatibility profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	return true;
}

/*
** GL_BeginFrame
*/
void GL_BeginFrame()
{
	if (gl_state.window == NULL)
		return;

	glDrawBuffer(GL_BACK);
}

/*
** GL_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GL_EndFrame()
{
	// if the window closed exit
	if (gl_state.window == NULL)
		return;

	//int32_t	err;

	//err = glGetError();
	//assert(err == GL_NO_ERROR);

	glfwPollEvents();

	// if the window closed *AFTER* events were polled (e.g. the user pressed X on the window), do nothing
	if (gl_state.window == NULL)
		return;

	if (!stricmp(gl_drawbuffer->string, "GL_BACK"))
		// swap the buffers
		glfwSwapBuffers(gl_state.window);
}

void GL_SetMousePressedProc(void proc(void* unused, int32_t button, int32_t action, int32_t mods))
{
	glfwSetMouseButtonCallback(gl_state.window, (GLFWmousebuttonfun)proc);
}

void GL_SetMouseScrollProc(void proc(void* unused, double xoffset, double yoffset))
{
	glfwSetScrollCallback(gl_state.window, (GLFWscrollfun)proc);
}

void GL_SetKeyPressedProc(void proc(void* unused, int32_t key, int32_t scancode, int32_t action, int32_t mods))
{
	glfwSetKeyCallback(gl_state.window, (GLFWkeyfun)proc);
}

void GL_SetMouseMovedProc(void proc(void* unused, double xpos, double ypos))
{
	glfwSetCursorPosCallback(gl_state.window, (GLFWcursorposfun)proc);
}

void GL_SetWindowFocusProc(void proc(void* unused, int32_t focused))
{
	glfwSetWindowFocusCallback(gl_state.window, (GLFWwindowfocusfun)proc);
}

void GL_SetWindowIconifyProc(void proc(void* unused, int32_t iconified))
{
	glfwSetWindowIconifyCallback(gl_state.window, (GLFWwindowiconifyfun)proc);
}

void GL_GetCursorPosition(double* x, double* y)
{
	glfwGetCursorPos(gl_state.window, x, y);
}

// RELATIVE TO THE WINDOW
void GL_SetCursorPosition(double x, double y)
{
	glfwSetCursorPos(gl_state.window, x, y);
}

void GL_SetWindowPosition(double x, double y)
{
	glfwSetWindowPos(gl_state.window, x, y);
}

void GL_EnableCursor(bool enabled)
{
	if (!enabled)
	{
		// turn off mouse acceleration
		if (glfwRawMouseMotionSupported())
		{
			glfwSetInputMode(gl_state.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}

		glfwSetInputMode(gl_state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else
	{
		glfwSetInputMode(gl_state.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

		// turn it back on
		glfwSetInputMode(gl_state.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void GLFW_Error(int32_t error_code, const char* error)
{
	Sys_Error("A fatal GLFW error occurred:\n\nError %d: %s", error_code, error);
}