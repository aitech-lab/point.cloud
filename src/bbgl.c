#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "bbgl.h"
#include "interactive.h"
#include "gui.h"
#include "scene.h"

extern void interactive_update(void);

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

int screen_width;
int screen_height;
float ratio;


static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}

/* Platform */
static GLFWwindow *win;
int screen_width = 0, screen_height = 0;
scene_p scene;


// Обработчик события движения курсора мыши
// Текущее состояние клика и позиция
static bool g_MousePressed = false;
static double g_LastMouseX = 0.0;
static double g_LastMouseY = 0.0;
void curposcb(GLFWwindow *window, double xpos, double ypos) {

  ImGuiIO *ioptr = igGetIO();

  // **ВАЖНО:** Если ImGui использует мышь, мы не обрабатываем событие для
  // сцены.
  if (ioptr->WantCaptureMouse)
    return;

  // Логика перетаскивания (Drag)
  if (g_MousePressed) {
    double dx = xpos - g_LastMouseX;
    double dy = ypos - g_LastMouseY;

    // ВАЖНО: OpenGL Y-координата часто инвертирована по сравнению с экранными
    // координатами (сверху-вниз). Если вы перемещаете 3D-объект, вам, возможно,
    // придётся инвертировать 'dy'.

    printf("Dragging: Delta (%.1f, %.1f) | New Pos (%.1f, %.1f)\n", dx, dy,
           xpos, ypos);

    // Обновление последней позиции для следующего кадра
    g_LastMouseX = xpos;
    g_LastMouseY = ypos;

    // Здесь можно добавить логику для перемещения камеры/объектов
    // Например, изменение угла поворота в зависимости от dx и dy
  }
}



void bbgl_init() {

    /* GLFW */
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) exit(1);
    
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Cluster cloud", NULL, NULL);
    if(!win) exit(1);

    glfwMakeContextCurrent(win);
    gl3wInit();
    glfwGetWindowSize(win, &screen_width, &screen_height);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    interactive_init(win);
    gui_init(win);

    scene = scene_ctor();
}

void bbgl_loop() {

    while (!glfwWindowShouldClose(win)) {
        /* Input */
        glfwPollEvents();
        interactive_update();

        gui_update(scene);

        /* Draw */
        glfwGetWindowSize(win, &screen_width, &screen_height);
        ratio = (float)screen_width/(float)screen_height;
        glViewport(0, 0, screen_width, screen_height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        
        //SCENE
        glEnable(GL_BLEND);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        scene_render(scene);
        glDisable(GL_BLEND);
        
        // GUI
        gui_render();
        
        glfwSwapBuffers(win);
    }
 
}

void bbgl_terminate() {
    
    // cleanup
    scene_dtor(scene);
    gui_terminate();
    // glfwTerminate();

    // clean up
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(NULL);

    glfwDestroyWindow(win);
    glfwTerminate();

}

