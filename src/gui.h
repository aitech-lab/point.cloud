#pragma once

typedef struct GLFWwindow GLFWwindow;
typedef struct scene_t scene_t;

void gui_init(GLFWwindow* win);
void gui_update(scene_t* scene);
void gui_render();
void gui_terminate();

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <cimgui_impl.h>

#ifdef IMGUI_HAS_IMSTR
#define igBegin igBegin_Str
#define igSliderFloat igSliderFloat_Str
#define igCheckbox igCheckbox_Str
#define igColorEdit3 igColorEdit3_Str
#define igButton igButton_Str
#endif

#define igGetIO igGetIO_Nil

extern ImGuiContext* ctx;
extern ImGuiIO* io;