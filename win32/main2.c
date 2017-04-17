#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
//#include "gl3w.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>



#define countof(x) (sizeof(x) / sizeof(0[x]))

#define M_PI 3.141592653589793
#define ATTRIB_POINT 0

static GLuint
compile_shader(GLenum type, const GLchar *source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint param;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
	if (!param) {
		GLchar log[4096];
		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		fprintf(stderr, "error: %s: %s\n",
			type == GL_FRAGMENT_SHADER ? "frag" : "vert", (char *)log);
		exit(EXIT_FAILURE);
	}
	return shader;
}

static GLuint
link_program(GLuint vert, GLuint frag)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	GLint param;
	glGetProgramiv(program, GL_LINK_STATUS, &param);
	if (!param) {
		GLchar log[4096];
		glGetProgramInfoLog(program, sizeof(log), NULL, log);
		fprintf(stderr, "error: link: %s\n", (char *)log);
		exit(EXIT_FAILURE);
	}
	return program;
}

struct graphics_context {
	GLFWwindow *window;
	GLuint program;
	GLint uniform_angle;
	GLint uniform_test_mat4;
	GLuint vbo_point;
	GLuint vao_point;
	double angle;
	long framecount;
	double lastframe;
};

const float SQUARE[] = { 0.0f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, };

static void
render(struct graphics_context *context)
{
	glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(context->program);
	glUniform1f(context->uniform_angle, (float)context->angle);
	GLfloat test_mat4[16] = {
		0.5, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
	glUniformMatrix4fv(context->uniform_test_mat4, 1, GL_FALSE, test_mat4);
	GLenum error_num = glGetError();
	glBindVertexArray(context->vao_point);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
	glUseProgram(0);

	/* Physics */
	double now = glfwGetTime();
	double udiff = now - context->lastframe;
	context->angle += 1.0 * udiff;
	if (context->angle > 2 * M_PI)
		context->angle -= 2 * M_PI;
	context->framecount++;
	if ((long)now != (long)context->lastframe) {
		printf("FPS: %ld\n", context->framecount);
		context->framecount = 0;
	}
	context->lastframe = now;

	glfwSwapBuffers(context->window);
}

static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	(void)scancode;
	(void)mods;
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

int
main_working_2(int argc, char **argv)
{
	/* Options */
	bool fullscreen = false;
	const char *title = "OpenGL 3.3 Demo";

	/* Create window and OpenGL context */
	struct graphics_context context;
	context.angle = 0;
	if (!glfwInit()) {
		fprintf(stderr, "GLFW3: failed to initialize\n");
		exit(EXIT_FAILURE);
	}
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	if (fullscreen) {
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *m = glfwGetVideoMode(monitor);
		context.window =
			glfwCreateWindow(m->width, m->height, title, monitor, NULL);
	}
	else {
		context.window =
			glfwCreateWindow(640, 640, title, NULL, NULL);
	}
	glfwMakeContextCurrent(context.window);
	glfwSwapInterval(1);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	/*
	if (gl3wInit()) {
		fprintf(stderr, "failed to initialize OpenGL\n");
		return -1;
	}
	if (!gl3wIsSupported(3, 2)) {
		fprintf(stderr, "OpenGL 3.2 not supported\n");
		return -1;
	}
	printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION),
		glGetString(GL_SHADING_LANGUAGE_VERSION));
		*/

	/* Shader sources */
	const GLchar *vert_shader =
		"#version 410\n"
		"in vec2 point;\n"
		"uniform float angle;\n"
		"uniform mat4 test_mat4;\n"
		"void main() {\n"
		"    mat2 rotate = mat2(cos(angle), -sin(angle),\n"
		"                       sin(angle), cos(angle));\n"
		"    gl_Position = test_mat4 * vec4(0.75 * rotate * point, 0.0, 1.0);\n"
		"}\n";
	const GLchar *frag_shader =
		"#version 410\n"
		"out vec4 color;\n"
		"void main() {\n"
		"    color = vec4(1, 0.15, 0.15, 0);\n"
		"}\n";

	/* Compile and link OpenGL program */
	GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_shader);
	GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_shader);
	context.program = link_program(vert, frag);
	context.uniform_angle = glGetUniformLocation(context.program, "angle");
	context.uniform_test_mat4 = glGetUniformLocation(context.program, "test_mat4");
	glDeleteShader(frag);
	glDeleteShader(vert);

	/* Prepare vertex buffer object (VBO) */
	glGenBuffers(1, &context.vbo_point);
	glBindBuffer(GL_ARRAY_BUFFER, context.vbo_point);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SQUARE), SQUARE, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Prepare vertrex array object (VAO) */
	glGenVertexArrays(1, &context.vao_point);
	glBindVertexArray(context.vao_point);
	glBindBuffer(GL_ARRAY_BUFFER, context.vbo_point);
	glVertexAttribPointer(ATTRIB_POINT, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(ATTRIB_POINT);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	/* Start main loop */
	glfwSetKeyCallback(context.window, key_callback);
	context.lastframe = glfwGetTime();
	context.framecount = 0;
	while (!glfwWindowShouldClose(context.window)) {
		render(&context);
		glfwPollEvents();
	}
	fprintf(stderr, "Exiting ...\n");

	/* Cleanup and exit */
	glDeleteVertexArrays(1, &context.vao_point);
	glDeleteBuffers(1, &context.vbo_point);
	glDeleteProgram(context.program);

	glfwTerminate();
	return 0;
}
