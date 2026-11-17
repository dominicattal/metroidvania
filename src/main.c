#include "context.h"
#include <string.h>

typedef struct {
    GLFWmonitor* monitor;
    GLFWwindow* window;
} WindowContext;

GlobalContext global_ctx;
static WindowContext window_ctx;

bool key_pressed(i32 key)
{
    return glfwGetKey(window_ctx.window, key) == GLFW_PRESS;
}

void window_error_callback(int x, const char* message)
{
    log(ERROR, message);
}

void framebuffer_size_callback(GLFWwindow* window, i32 width, i32 height)
{
    glViewport(0, 0, width, height);
    global_ctx.window_width = width;
    global_ctx.window_height = height;
}

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
    {
        if (window_ctx.monitor == NULL)
        {
            window_ctx.monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(window_ctx.monitor);
            glfwSetWindowMonitor(window, window_ctx.monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            window_ctx.monitor = NULL;
            glfwSetWindowMonitor(window, NULL, 200, 200, 640, 480, 0);
        }
    }

    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
        global_ctx.draw_hitboxes = !global_ctx.draw_hitboxes;
}

int main()
{
    glfwSetErrorCallback(window_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
 
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    global_ctx.window_width = 640;
    global_ctx.window_height = 480;
    window_ctx.window= glfwCreateWindow(global_ctx.window_width, global_ctx.window_height, "El Game", NULL, NULL);
    glfwMakeContextCurrent(window_ctx.window);
    glfwSetFramebufferSizeCallback(window_ctx.window, framebuffer_size_callback);
    glfwSetKeyCallback(window_ctx.window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, global_ctx.window_width, global_ctx.window_height);
    glfwSwapInterval(1);

    render_init();
    game_init();

    f64 dt = 1.0 / 60.0;
    global_ctx.time = 0;

    while (!glfwWindowShouldClose(window_ctx.window))
    {
        game_update(dt);
        render();

        glfwSwapBuffers(window_ctx.window);
        glfwPollEvents();

        global_ctx.time += dt;
    }
    glfwTerminate();
}
