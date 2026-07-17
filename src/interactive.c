#include <GLFW/glfw3.h>
#include <math.h>

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

    if (io->WantCaptureMouse) return;

    static double old_xpos;
    static double old_ypos;
    double dx = xpos - old_xpos;
    double dy = ypos - old_ypos;

    if(!gui_focused){
        if(glfwGetMouseButton(win, 1) == GLFW_PRESS) {
            vec3 local_up = {0.0f, 1.0f, 0.0f};
            vec3 local_right = {1.0f, 0.0f, 0.0f};

            vec3 screen_up, screen_right;
            glm_quat_rotatev(gui_camera_quat, local_up, screen_up);
            glm_quat_rotatev(gui_camera_quat, local_right, screen_right);

            float angle_horizontal = -dx * 0.005f;
            float angle_vertical = -dy * 0.005f;

            versor q_h = {
                sinf(angle_horizontal*0.5f) * screen_up[0],
                sinf(angle_horizontal*0.5f) * screen_up[1],
                sinf(angle_horizontal*0.5f) * screen_up[2],
                cosf(angle_horizontal*0.5f)
            };

            versor q_v = {
                sinf(angle_vertical*0.5f) * screen_right[0],
                sinf(angle_vertical*0.5f) * screen_right[1],
                sinf(angle_vertical*0.5f) * screen_right[2],
                cosf(angle_vertical*0.5f)
            };

            versor q_result;
            glm_quat_mul(q_h, gui_camera_quat, q_result);
            glm_quat_mul(q_v, q_result, gui_camera_quat);
        } else if(glfwGetMouseButton(win, 2) == GLFW_PRESS) {
            vec3 cam_offset = {0.0f, 0.0f, gui_camera_radius};
            vec3 cam_pos;
            glm_quat_rotatev(gui_camera_quat, cam_offset, cam_pos);

            vec3 world_cam_pos = {gui_camera_tx + cam_pos[0], gui_camera_ty + cam_pos[1], gui_camera_tz + cam_pos[2]};
            vec3 target = {gui_camera_tx, gui_camera_ty, gui_camera_tz};
            vec3 view_dir;
            glm_vec3_sub(target, world_cam_pos, view_dir);
            glm_vec3_normalize(view_dir);

            vec3 world_up = {0.0, 1.0, 0.0};
            vec3 right;
            glm_vec3_cross(view_dir, world_up, right);
            glm_vec3_normalize(right);

            vec3 up;
            glm_vec3_cross(right, view_dir, up);
            glm_vec3_normalize(up);

            float pan_speed = 0.1;
            gui_camera_target_tx += right[0] * dx * pan_speed - up[0] * dy * pan_speed;
            gui_camera_target_ty += right[1] * dx * pan_speed - up[1] * dy * pan_speed;
            gui_camera_target_tz += right[2] * dx * pan_speed - up[2] * dy * pan_speed;
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

    printf("%d %d %d\n", button, action, mods);
    if(button == 0 && action == 1) {
        render_id = 1;
        // +ctrl
        if(mods & 1) {
            do_search_nearest = 1;
        }
        if(mods & 2) {
            do_search_cluster = 1;
        }
    }
}

static void mouse_scroll_callback(
    GLFWwindow* win,
    double xoffset,
    double yoffset) {
    
    if (io->WantCaptureMouse) return;

    gui_camera_radius -= yoffset;
}