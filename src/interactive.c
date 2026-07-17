#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>

#include "interactive.h"
#include "globals.h"
#include "gui.h"

#define SETTINGS_FILE "point_cloud_settings.cfg"

int mouse_x;
int mouse_y;

int key_move_forward = 26;  // W
int key_move_left    = 38;  // A
int key_move_back    = 39;  // S
int key_move_right   = 40;  // D
int key_move_up      = 65;  // Space
int key_move_down    = 50;  // LCtrl

int rebinding_key = 0;
static int* rebinding_target = NULL;
static int keys_pressed[256] = {0};

int* get_rebinding_target(void) {
    return rebinding_target;
}

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
    // If user is rebinding a key, capture the next key press and save it
    if (action == GLFW_PRESS && rebinding_target != NULL) {
        *rebinding_target = scancode;
        rebinding_target = NULL;
        rebinding_key = 0;
        settings_save();
        return;
    }

    // Track which keys are currently pressed (for game-style continuous movement)
    if (scancode < 256) {
        keys_pressed[scancode] = (action != GLFW_RELEASE);
    }
}

void interactive_start_rebind(int* key_ptr) {
    rebinding_target = key_ptr;
    rebinding_key = 1;
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
            // Right-drag: quaternion-based orbit rotation around screen axes
            // Extract screen-aligned axes from camera quaternion to avoid gimbal lock
            vec3 local_up = {0.0f, 1.0f, 0.0f};
            vec3 local_right = {1.0f, 0.0f, 0.0f};

            vec3 screen_up, screen_right;
            glm_quat_rotatev(gui_camera_quat, local_up, screen_up);
            glm_quat_rotatev(gui_camera_quat, local_right, screen_right);

            float angle_horizontal = -dx * 0.005f;
            float angle_vertical = -dy * 0.005f;

            // Build quaternions for horizontal (around screen Y) and vertical (around screen X) rotations
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

            // Apply both rotations: horizontal first, then vertical
            versor q_result;
            glm_quat_mul(q_h, gui_camera_quat, q_result);
            glm_quat_mul(q_v, q_result, gui_camera_quat);
        } else if(glfwGetMouseButton(win, 2) == GLFW_PRESS) {
            // Middle-drag: pan camera in view-space plane (screen-relative)
            vec3 cam_offset = {0.0f, 0.0f, gui_camera_radius};
            vec3 cam_pos;
            glm_quat_rotatev(gui_camera_quat, cam_offset, cam_pos);

            vec3 world_cam_pos = {gui_camera_tx + cam_pos[0], gui_camera_ty + cam_pos[1], gui_camera_tz + cam_pos[2]};
            vec3 target = {gui_camera_tx, gui_camera_ty, gui_camera_tz};
            vec3 view_dir;
            glm_vec3_sub(target, world_cam_pos, view_dir);
            glm_vec3_normalize(view_dir);

            // Build view-space axes for panning (not world-aligned)
            vec3 local_up = {0.0, 1.0, 0.0};
            vec3 screen_up;
            glm_quat_rotatev(gui_camera_quat, local_up, screen_up);

            vec3 right;
            glm_vec3_cross(view_dir, screen_up, right);
            glm_vec3_normalize(right);

            vec3 up;
            glm_vec3_cross(right, view_dir, up);
            glm_vec3_normalize(up);

            // Pan target in view-space: negative dx moves left, negative dy moves up
            float pan_speed = 0.1;
            gui_camera_target_tx -= right[0] * dx * pan_speed - up[0] * dy * pan_speed;
            gui_camera_target_ty -= right[1] * dx * pan_speed - up[1] * dy * pan_speed;
            gui_camera_target_tz -= right[2] * dx * pan_speed - up[2] * dy * pan_speed;
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

void interactive_update(void) {
    if (io->WantCaptureKeyboard || gui_focused) return;

    // Game-style WASD movement: all movement is relative to current camera orientation
    // Compute camera-space axes from the quaternion to move in local coordinates
    vec3 local_up = {0.0f, 1.0f, 0.0f};
    vec3 local_right = {1.0f, 0.0f, 0.0f};
    vec3 local_forward = {0.0f, 0.0f, 1.0f};

    vec3 screen_up, screen_right, screen_forward;
    glm_quat_rotatev(gui_camera_quat, local_up, screen_up);
    glm_quat_rotatev(gui_camera_quat, local_right, screen_right);
    glm_quat_rotatev(gui_camera_quat, local_forward, screen_forward);

    float move_speed = 0.5f;

    // Each key moves the camera target in the corresponding screen-aligned direction
    if (keys_pressed[key_move_forward]) {
        gui_camera_target_tx -= screen_forward[0] * move_speed;
        gui_camera_target_ty -= screen_forward[1] * move_speed;
        gui_camera_target_tz -= screen_forward[2] * move_speed;
    }
    if (keys_pressed[key_move_back]) {
        gui_camera_target_tx += screen_forward[0] * move_speed;
        gui_camera_target_ty += screen_forward[1] * move_speed;
        gui_camera_target_tz += screen_forward[2] * move_speed;
    }
    if (keys_pressed[key_move_left]) {
        gui_camera_target_tx -= screen_right[0] * move_speed;
        gui_camera_target_ty -= screen_right[1] * move_speed;
        gui_camera_target_tz -= screen_right[2] * move_speed;
    }
    if (keys_pressed[key_move_right]) {
        gui_camera_target_tx += screen_right[0] * move_speed;
        gui_camera_target_ty += screen_right[1] * move_speed;
        gui_camera_target_tz += screen_right[2] * move_speed;
    }
    if (keys_pressed[key_move_up]) {
        gui_camera_target_tx += screen_up[0] * move_speed;
        gui_camera_target_ty += screen_up[1] * move_speed;
        gui_camera_target_tz += screen_up[2] * move_speed;
    }
    if (keys_pressed[key_move_down]) {
        gui_camera_target_tx -= screen_up[0] * move_speed;
        gui_camera_target_ty -= screen_up[1] * move_speed;
        gui_camera_target_tz -= screen_up[2] * move_speed;
    }
}

// Load keybindings from config file (scan codes, not layout-dependent)
void settings_load(void) {
    FILE* f = fopen(SETTINGS_FILE, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        int val;
        if (sscanf(line, "key_move_forward=%d", &val) == 1) key_move_forward = val;
        else if (sscanf(line, "key_move_left=%d", &val) == 1) key_move_left = val;
        else if (sscanf(line, "key_move_back=%d", &val) == 1) key_move_back = val;
        else if (sscanf(line, "key_move_right=%d", &val) == 1) key_move_right = val;
        else if (sscanf(line, "key_move_up=%d", &val) == 1) key_move_up = val;
        else if (sscanf(line, "key_move_down=%d", &val) == 1) key_move_down = val;
    }
    fclose(f);
}

// Persist keybindings to config file so they survive app restarts
void settings_save(void) {
    FILE* f = fopen(SETTINGS_FILE, "w");
    if (!f) return;

    fprintf(f, "key_move_forward=%d\n", key_move_forward);
    fprintf(f, "key_move_left=%d\n", key_move_left);
    fprintf(f, "key_move_back=%d\n", key_move_back);
    fprintf(f, "key_move_right=%d\n", key_move_right);
    fprintf(f, "key_move_up=%d\n", key_move_up);
    fprintf(f, "key_move_down=%d\n", key_move_down);

    fclose(f);
}