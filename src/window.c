#include "string.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	char title[256];
	int width, height;
	int in_focus;
	int should_close;
	GLFWwindow * raw_window;
} Window;

int glfw_initialized = 0;
size_t window_count = 0;

Window * window_create(char * title, int width, int height)
{
	// Make sure to initialize glfw once
	if (!glfw_initialized && glfwInit())
		glfw_initialized = 1;

		glfwInit();
	printf("Creating window\n");
	Window * window = malloc(sizeof(Window));
	if (window == NULL)
	{
		return NULL;
	}
	window->width = width;
	window->height = height;
	window->in_focus = 1;
	window->should_close = 0;
	strcpy(window->title, title);

	window->raw_window = glfwCreateWindow(width, height, title, NULL, NULL);
	window_count++;
	return window;
}

void window_destroy(Window * window)
{
	glfwDestroyWindow(window->raw_window);
	window_count--;
	if (window_count == 0)
		glfwTerminate();
	free(window);
}

void window_update(Window * window)
{
	glfwPollEvents();
	window->should_close = glfwWindowShouldClose(window->raw_window);
}

bool window_get_close(Window * window)
{
	return window->should_close;
}
