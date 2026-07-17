#pragma once

typedef struct GLFWwindow GLFWwindow;
void interactive_init(GLFWwindow* win);
void interactive_update(void);
void interactive_start_rebind(int* key_ptr);
void settings_load(void);
void settings_save(void);

extern int mouse_x;
extern int mouse_y;

extern int key_move_forward;
extern int key_move_left;
extern int key_move_back;
extern int key_move_right;
extern int key_move_up;
extern int key_move_down;
extern int rebinding_key;