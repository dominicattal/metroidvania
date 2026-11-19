#include "context.h"
#include <string.h>

GlobalContext global_ctx;

bool key_pressed(i32 key)
{
    return glfwGetKey(global_ctx.window, key) == GLFW_PRESS;
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
        if (global_ctx.monitor == NULL)
        {
            global_ctx.monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(global_ctx.monitor);
            glfwSetWindowMonitor(window, global_ctx.monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            global_ctx.monitor = NULL;
            glfwSetWindowMonitor(window, NULL, 200, 200, 640, 480, 0);
        }
    }

    if (key == GLFW_KEY_F3 && action == GLFW_PRESS)
        global_ctx.draw_entities = !global_ctx.draw_entities;

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
        global_ctx.draw_hitboxes = !global_ctx.draw_hitboxes;

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        screen_load(SCREEN_1);
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        screen_load(SCREEN_2);
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
    global_ctx.window= glfwCreateWindow(global_ctx.window_width, global_ctx.window_height, "El Game", NULL, NULL);
    glfwMakeContextCurrent(global_ctx.window);
    glfwSetFramebufferSizeCallback(global_ctx.window, framebuffer_size_callback);
    glfwSetKeyCallback(global_ctx.window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, global_ctx.window_width, global_ctx.window_height);
    glfwSwapInterval(1);

    audio_init();
    render_init();
    game_init();

    f64 dt = 1.0 / 60.0;
    global_ctx.time = global_ctx.next_update_time = 0;

    while (!glfwWindowShouldClose(global_ctx.window))
    {
        if (global_ctx.time > global_ctx.next_update_time)
        {
            game_update(dt);
            global_ctx.next_update_time += dt;
        }
        render();
        audio_update();

        glfwSwapBuffers(global_ctx.window);
        glfwPollEvents();

        global_ctx.time += dt;
    }
    glfwTerminate();
}
