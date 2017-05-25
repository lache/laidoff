#include <time.h>
#include "lwgl.h"
#include "laidoff.h"
#include "czmq.h"
#include "lwdeltatime.h"

#ifndef BOOL
#define BOOL int
#endif

// Working directory change
#if LW_PLATFORM_OSX || LW_PLATFORM_RPI
#include <dirent.h>
#include <unistd.h>
#endif

#if LW_PLATFORM_WIN32
#define LwChangeDirectory(x) SetCurrentDirectory(x)
#else
#define LwChangeDirectory(x) chdir(x)
#endif

#define INITIAL_SCREEN_RESOLUTION_X 640 //(1920)
#define INITIAL_SCREEN_RESOLUTION_Y 360 //(1080)

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_pos_callback(GLFWwindow* window, double x, double y);
void destroy_ext_sound_lib();

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static BOOL directory_exists(const char* szPath)
{
#if LW_PLATFORM_WIN32
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    DIR* dir = opendir(szPath);
    if (dir)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
	LWCONTEXT* pLwc = (LWCONTEXT*)glfwGetWindowUserPointer(window);

	lw_set_size(pLwc, width, height);
}

static GLFWwindow* create_glfw_window()
{
	return glfwCreateWindow(INITIAL_SCREEN_RESOLUTION_X, INITIAL_SCREEN_RESOLUTION_Y, "LAID OFF", NULL, NULL);
}

static void s_logic_worker(zsock_t *pipe, void *args) {
	GLFWwindow* window = args;
	LWCONTEXT* pLwc = glfwGetWindowUserPointer(window);

	zsock_signal(pipe, 0);

	while (!glfwWindowShouldClose(window)) {
		lwc_update(pLwc, 1.0 / 60);
	}
}

int main(void)
{
	printf(LWU("용사는 휴직중 LAID OFF v0.1\n"));
    
    while (!directory_exists("assets") && LwChangeDirectory(".."))
	{
	}

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	/*
	OpenGL Version -	GLSL Version
	2.0 -	110
	2.1 -	120
	3.0 -	130
	3.1 -	140
	3.2 -	150
	3.3 -	330
	4.0 -	400
	4.1 -	410
	4.2 -	420
	4.3 -	430
	*/

#if !LW_PLATFORM_RPI
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
#endif

	glfwWindowHint(GLFW_DEPTH_BITS, 16);

	glfwWindowHint(GLFW_SAMPLES, 4); // supersampling

#if LW_PLATFORM_RPI
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#endif
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

	GLFWwindow* window = create_glfw_window();

	if (!window)
	{
		// Try with OpenGL API again
		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

		window = create_glfw_window();

		if (!window)
		{
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
	}

	glfwSetWindowPos(window, 600, 400);

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetCursorPosCallback(window, mouse_pos_callback);
	glfwMakeContextCurrent(window);
#if !LW_PLATFORM_RPI
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
#endif
	glfwSwapInterval(1);

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	srand((unsigned int)time(0));

	LWCONTEXT* pLwc = lw_init();

	lw_set_window(pLwc, window);

	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	lw_set_size(pLwc, width, height);

	glfwSetWindowUserPointer(window, pLwc);
	// Start a logic thread
	zactor_new(s_logic_worker, window);
	
	while (!glfwWindowShouldClose(window))
	{
		deltatime_tick(pLwc->render_dt);

		//lwc_update(pLwc, 1.0 / 60);

		lwc_render(pLwc);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
	
	destroy_ext_sound_lib();

	lw_on_destroy(pLwc);
	
    printf(LWU("용사는 휴직중 종료\n"));

	exit(EXIT_SUCCESS);
}

void lw_app_quit(const LWCONTEXT* pLwc)
{
	glfwSetWindowShouldClose(lw_get_window(pLwc), GLFW_TRUE);
}

int request_get_today_played_count()
{
	return 0;
}

int request_get_today_playing_limit_count()
{
	return 5;
}

void request_on_game_start()
{
	// do nothing
}

void request_on_game_over(int point)
{
	// do nothing
}

void request_boast(int point)
{
	// do nothing
}

int request_is_retryable()
{
	return 1;
}

int request_is_boasted()
{
	return 0;
}

int request_get_highscore()
{
	return 0;
}

void request_set_highscore(int highscore)
{
	// do nothing
}
