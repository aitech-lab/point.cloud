#include <GLFW/glfw3.h>

#include "interactive.h"
#include "globals.h"
#include "gui.h"

int mouse_x;
int mouse_y;

static void key_callback            (GLFWwindow* win, int key, int scancode, int action, int mods);
static void cursor_position_callback(GLFWwindow* win, double xpos, double ypos);
static void mouse_button_callback   (GLFWwindow* win, int button, int action, int mods);
static void mouse_scroll_callback   (GLFWwindow* win, double xoffset, double yoffset);

void interactive_init(GLFWwindow* win) {
    glfwSetKeyCallback        (win, key_callback);
    glfwSetCursorPosCallback  (win, cursor_position_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetScrollCallback     (win, mouse_scroll_callback);
}

static void key_callback(
    GLFWwindow* win, 
    int key, 
    int scancode, 
    int action, 
    int mods) {
    // printf("key %d\n", key);
}

static bool g_MousePressed = false;
static double g_LastMouseX = 0.0;
static double g_LastMouseY = 0.0;
static void cursor_position_callback(
    GLFWwindow* win, 
    double xpos, 
    double ypos) {

    mouse_x = xpos;
    mouse_y = ypos;

    // ImGuiIO *ioptr = igGetIO();
    if (io->WantCaptureMouse) return;

    static double old_xpos;
    static double old_ypos; 
    mouse_x = xpos;
    mouse_y = ypos;
    if(!gui_focused){ 
        if(glfwGetMouseButton(win,1) == GLFW_PRESS) {
            gui_camera_rx -= (old_ypos-ypos)*0.1;
            gui_camera_ry -= (old_xpos-xpos)*0.1;
        } else if(glfwGetMouseButton(win,1) == GLFW_PRESS) {
            gui_rot_u -= (old_xpos-xpos)*0.01;
            // gui_rot_v -= (old_ypos-ypos)*0.01;
            gui_off_u -= (old_ypos-ypos)*0.1;        
        }
    }
    old_xpos = xpos;
    old_ypos = ypos;
}


static void mouse_button_callback(
    GLFWwindow* win, 
    int button, 
    int action, 
    int mods) {

    if (io->WantCaptureMouse) return;

    // printf("%d %d %d\n", button, action, mods);
    if(button == 0 && action == 1) {
        render_id = 1;
    }
}

static void mouse_scroll_callback(
    GLFWwindow* win,
    double xoffset,
    double yoffset) {
    
    if (io->WantCaptureMouse) return;

    gui_camera_radius -= yoffset;
}