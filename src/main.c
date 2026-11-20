#include "context.h"
#include <string.h>
#include <sys/timeb.h>
#include <assert.h>

GlobalContext global_ctx;

bool key_down(i32 key)
{
    return glfwGetKey(global_ctx.window, key) == GLFW_PRESS;
}

bool key_pressed(i32 key)
{
    return global_ctx.key_pressed[key];
}

void window_error_callback(i32 x, const char* message)
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
    if (action == GLFW_PRESS)
    {
        assert(key <= GLFW_KEY_LAST);
        global_ctx.key_pressed[key] = true;
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, true);

        if (key == GLFW_KEY_F11)
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

        if (key == GLFW_KEY_F3)
            global_ctx.draw_entities = !global_ctx.draw_entities;

        if (key == GLFW_KEY_F2)
            global_ctx.draw_hitboxes = !global_ctx.draw_hitboxes;

        if (key == GLFW_KEY_1)
            screen_load(SCREEN_1);
        if (key == GLFW_KEY_2)
            screen_load(SCREEN_2);
        if (key == GLFW_KEY_3)
            screen_load(SCREEN_NONE);
    }
    game_key_callback(key, action);
}

int main()
{
    glfwSetErrorCallback(window_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
 
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    global_ctx.window_width = 640;
    global_ctx.window_height = 480;
    global_ctx.window = glfwCreateWindow(global_ctx.window_width, global_ctx.window_height, "El Game", NULL, NULL);

    glfwSetFramebufferSizeCallback(global_ctx.window, framebuffer_size_callback);
    glfwSetKeyCallback(global_ctx.window, key_callback);

    glfwMakeContextCurrent(global_ctx.window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    audio_init();
    render_init();
    game_init();
    ui_init();

    f64 dt = 1.0 / 60.0;
    global_ctx.time = global_ctx.next_update_time = 0;

    struct timeb start, end;

    ftime(&start);
    while (!glfwWindowShouldClose(global_ctx.window))
    {
        if (global_ctx.time > global_ctx.next_update_time)
        {
            game_update(dt);
            ui_update(dt);
            global_ctx.next_update_time += dt;
        }

        render();
        audio_update();

        memset(global_ctx.key_pressed, 0, sizeof(global_ctx.key_pressed));
        glfwSwapBuffers(global_ctx.window);
        glfwPollEvents();

        ftime(&end);
        global_ctx.time += (f64)(end.time - start.time + (end.millitm - start.millitm) / 1000.0f);
        start = end;
    }
    render_cleanup();
    glfwDestroyWindow(global_ctx.window);
    glfwTerminate();
}
