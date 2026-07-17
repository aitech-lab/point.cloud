#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include "globals.h"
#include "bbgl.h"
#include "gui.h"
#include "data.h"
#include "scene.h"
#include "kdtree.h"
#include "curl.h"

ImGuiContext* ctx;
ImGuiIO* io;
ImDrawData idd;

extern int key_move_forward;
extern int key_move_left;
extern int key_move_back;
extern int key_move_right;
extern int key_move_up;
extern int key_move_down;
extern int rebinding_key;
extern int* get_rebinding_target(void);
extern void interactive_start_rebind(int* key_ptr);
extern void settings_load(void);
extern void settings_save(void);

# define CID_COL 3

static bool render_labels = false;
static int labels_size = 18;

#define MESSAGES_MAX 1000
static float old_picked_cluster = -2.0f;
static int picked_cluster_count = 0;
static char* cluster_messages[MESSAGES_MAX] = {0};

#define MAX_SEARCH_RESULTS 1000
static int found_ids[MAX_SEARCH_RESULTS];
static char* found_messages[MAX_SEARCH_RESULTS];
static int found_cnt = 0;


static char input_buf[128 + 1] = {0}; // +1 для терминатора
static char search_buf[512 + 1] = {0}; // +1 для терминатора
#define COPY_BUFFER_SIZE (1<<20)
static char copy_buffer[COPY_BUFFER_SIZE+1];

#define CLASSIFY_BUFFER_SIZE (1<<20)
static char classify_request_buf[CLASSIFY_BUFFER_SIZE+1];
static char classify_response_buf[CLASSIFY_BUFFER_SIZE+1];
static int classify_in_progress = 0;
#define PROMPT_SIZE (1<<16)
static char classify_prompt[PROMPT_SIZE] = "Дай общую, короткую классификацию, в 1-10 слов, для всех сообщений сразу. Ничего больше не предлагай. Далее идет список сообщений, по одному в строку:\n"; 

// Функции
static void world2screen(vec4 r, mat4 vp, vec4 p);

static char* format_float(const char* format, float f);
void clusters_window();
void columns_window();
void search_window();
void keyboard_settings_window();
void draw_axis(scene_t* scene);
void draw_markers(scene_t* scene);
void draw_labels(scene_t* scene);

static void update_data();
static void search_nearest(int pid);
static void search_same_category(int pid);

static GLFWwindow* glfw_window;

void gui_init(GLFWwindow* win) {

    glfw_window = win;

    // // IMGUI_CHECKVERSION();
    ctx = igCreateContext(NULL);
    io  = igGetIO();
    
    //io.Fonts->AddFontDefault();
    // ImWchar* range = ImFontAtlas_GetGlyphRangesCyrillic(io->Fonts);
    ImWchar* range = ImFontAtlas_GetGlyphRangesDefault(io->Fonts);
    ImFontAtlas_AddFontFromFileTTF(io->Fonts, "./font.ttf", 16.f, NULL, range);

    const char* glsl_version = "#version 330 core";
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    settings_load();

    // Setup style
    // igStyleColorsDark(NULL);

}


// Callback для ImGui
void set_clipboard_text(const char* text) {
    glfwSetClipboardString(glfw_window, text);
}

const char* get_clipboard_text(void* user_data) {
    return glfwGetClipboardString(glfw_window);
}

// // При инициализации ImGui:
// ImGuiIO* io = igGetIO();
// io->SetClipboardUserData = glfw_window;  // ваш основной GLFWwindow*
// io->SetClipboardTextFn = set_clipboard_text;
// io->GetClipboardTextFn = get_clipboard_text;

void
gui_update(scene_t* scene) {

    update_data();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();

    columns_window();
    clusters_window();
    // search_window();
    keyboard_settings_window();

    draw_axis(scene);
    draw_markers(scene);    
    if(render_labels) draw_labels(scene);
   
    igRender();
   
    // // igShowDemoWindow(NULL);
    
    // gui_focused = igIsWindowFocused(ImGuiFocusedFlags_AnyWindow);
}


static void
update_data() {
    
    // Собираем список ближайших сообщений для выбранного
    // Если искать ближайших нужно и сменился выбранный семпл 
    static int picked_id_old = 0;
    if(do_search_nearest && picked_id != picked_id_old) {
        search_nearest(picked_id);
        do_search_nearest = 0;
    }

    // Собираем список сообщений для из того же кластера
    if(do_search_cluster && picked_id != picked_id_old) {
        search_same_category(picked_id);
        do_search_cluster = 0;
    } 
    picked_id_old = picked_id;

    // Собираем список сообщений для выбранного кластера
    if (old_picked_cluster != picked_cluster) {
        picked_cluster_count = 0;
        // for (int i = 0; i < data->rows; i++) {
        //     float* row = &data->data[i * data->cols];
        //     if ((int)picked_cluster == (int)row[CID_COL]) {
        //         cluster_messages[picked_cluster_count] = data->messages[i];
        //         if (picked_cluster_count >= MESSAGES_MAX - 1) break;
        //         picked_cluster_count++;
        //     }
        // }
        old_picked_cluster = picked_cluster;
    }    
}

int add_label(char* label) {
    float* r = &data->data[picked_id*data->cols];
    return data_add_label(label, r[0], r[1], r[2]);
}

void update_min_max() {
    gui_min = data->min[gui_col_id] + (data->max[gui_col_id]-data->min[gui_col_id])*0.01;
    gui_max = data->max[gui_col_id];
}

// Ищем сообщение включающие строку str
// В dynamic выставляем им 1.0
int on_search(char* str) {
    found_cnt = 0;
    for(int i=0; i<data->rows; i++) {
        char* msg = data->messages[i];
        if (strstr(msg, str) != NULL) {
            data->dynamic[i] = 1.0;
            if(found_cnt<MAX_SEARCH_RESULTS) {
                found_ids[found_cnt] = i;
                found_messages[found_cnt] = msg;
                found_cnt++;
            }
        } 
        // Сброс старого поиска
        // else {
        //     data->dynamic[i] = 0.0;
        // }
    }
    gui_col_id = data->cols;
    data->max[data->cols] = 1.0;
    update_min_max();
    dynamic_data_updated = 1;
    return 1;
}

int reset_search_results() {
    found_cnt = 0;
    for(int i=0; i<data->rows; i++) {
        data->dynamic[i] = 0.0;
    }
    update_min_max();
    dynamic_data_updated = 1;
    return 1;
}


bool label_input_widget() {
    bool added = false;
    ImVec2_c button_size = { .x = 30.0f, .y = 0.0f };
    
    igCheckbox("Show labels", &render_labels);
    if (render_labels) igSliderInt("Labels size", &labels_size,16,28, NULL,0);
    
    // Группируем текстовое поле и кнопку на одной строке
    // igSetNextItemWidth(-button_size.x - igGetStyle()->ItemSpacing.x);
    if (igInputText("Label", input_buf, sizeof(input_buf), ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
        // Enter нажат — тоже добавляем
        if (input_buf[0] != '\0') {
            add_label(input_buf);
            input_buf[0] = '\0'; 
        }
    }

    return added;
}

bool search_input_widget() {
    int searched = 0;
    ImVec2_c button_size = { .x = 0.0f, .y = 0.0f };
    
    float pick_min = 0.1;
    float pick_max = 3.0;
    igSliderScalar("Pick radius",ImGuiDataType_Float, &pick_range, &pick_min, &pick_max, NULL, 0.1f);
    if (igIsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        igBeginTooltip(); 
        igText("Use Ctrl+LMouse to select nearest"); 
        igEndTooltip(); 
    }
    // Группируем текстовое поле и кнопку на одной строке
    // igSetNextItemWidth(-button_size.x - igGetStyle()->ItemSpacing.x);
    if (igInputText("Search word", search_buf, sizeof(search_buf), ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
        // Enter нажат — тоже добавляем
        if (search_buf[0] != '\0') {
            searched = on_search(search_buf);
            if (searched) {
                search_buf[0] = '\0'; // очищаем после успешного добавления
            }
        }
    }
    if(igButton("Reset results", (ImVec2){0.0,0.0})) reset_search_results();
    
    
    return searched;
}

static void 
search_nearest(int pid) {
    found_cnt = 0; 
    float* row = &data->data[pid*data->cols];
    kdres* results = kd_nearest_range3f(data->index, row[0], row[1], row[2], pick_range);
    if(results) {
        int n = kd_res_size(results);
        printf("Found %d\n", n);
        
        kd_res_rewind(results);
        while (!kd_res_end(results)) {
            float x, y, z;
            int i = (uint)kd_res_item3f(results, &x, &y, &z);
            if(i) {
                char* msg = data->messages[i];
                data->dynamic[i] = 1.0;
                data->max[data->cols] = 1.0;
                if(found_cnt<MAX_SEARCH_RESULTS) {
                    found_ids[found_cnt] = i;
                    found_messages[found_cnt] = msg;
                    found_cnt++;
                }
            }
            kd_res_next(results);
        }
        kd_res_free(results);

        gui_col_id = data->cols;
        data->max[data->cols] = 1.0;
        update_min_max();
        dynamic_data_updated = 1;
    }
}


static void 
search_same_category(int pid) {
    found_cnt = 0; 
    float* row = &data->data[pid*data->cols];
    float cid = row[CID_COL];
    if(cid < 0) return;
    printf("search cid %d\n", cid);
    for (int i = 0; i < data->rows; i++) {
        row = &data->data[i*data->cols];
        if(row[CID_COL] == cid) {
            data->dynamic[i] = 1.0;
            if(found_cnt<MAX_SEARCH_RESULTS) {
                char* msg = data->messages[i];
                found_ids[found_cnt] = i;
                found_messages[found_cnt] = msg;
                found_cnt++;
            }
        }
    }
    data->max[data->cols] = 1.0;
    update_min_max();
    dynamic_data_updated = 1;
}


void
search_window() {

    char buf[256];

    // --- Настройки ---
    float window_width = 400.0f;  // Фиксированная ширина окна
    float padding = 10.0f;        // Отступ от краев (по желанию)
    ImGuiViewport* viewport = igGetMainViewport(); // Получаем основной вьюпорт (окно GLFW)

    float window_x = viewport->WorkPos.x + viewport->WorkSize.x - padding; // Координата правого края вьюпорта
    float window_y = viewport->WorkPos.y + padding;                       // Отступ сверху (pivot.y = 0 -> верх)

    ImVec2_c pos = { .x = window_x-400 - padding, .y = window_y };
    ImVec2_c pivot = { .x = 1.0f, .y = 0.0f }; // Привязка к правому верхнему углу окна

    igSetNextWindowPos(pos, ImGuiCond_Always, pivot);

    // Устанавливаем размер
    ImVec2_c size = { .x = window_width, .y = viewport->WorkSize.y - 2 * padding }; // Например, высота = высота вьюпорта - отступы
    igSetNextWindowSize(size, ImGuiCond_Always); // Используем Always, чтобы размер был фиксированным

    // ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    ImGuiWindowFlags flags = 0;

    if (igBegin("Search", NULL, flags)) {
        search_input_widget();
        igSeparator();
        for(int i = 0; i<found_cnt; i++) {
            char* msg = found_messages[i];
            igTextWrapped(msg);
        }
    }
    igEnd();
}


void
copy_found_to_buffer(char* buffer, int buffer_size, int reset_first) {
    if(reset_first) buffer[0]=0;
    int s = 0;
    int i;
    for (i=0; i<found_cnt; i++) {
        int l = strlen(found_messages[i])+1;
        // Проверяем, что вместится
        if (s + l > buffer_size) break; 
        strcat(buffer, found_messages[i]);
        strcat(buffer, "\n");
        s += l;
    }
    set_clipboard_text(buffer);
    printf("Copied %d/%d messages %d+Kb\n", i, found_cnt, s>>10);
}

void on_classify_complete(const char* response, int status_code) {
    // printf("===\n%s\n---\n%s\n", classify_request_buf, classify_response_buf);
    classify_in_progress = 0;
}

void classify_found() {
    // сбрасываем request_buf
    classify_request_buf[0] = 0;
    // копируем в буффер промпт
    strcat(classify_request_buf, classify_prompt);
    // копируем найденные сообщения, без сброса
    copy_found_to_buffer(classify_request_buf, CLASSIFY_BUFFER_SIZE - strlen(classify_request_buf), 0);

    curl_request_async(
        "http://localhost:5000/ask",
        classify_request_buf,
        classify_response_buf,
        on_classify_complete);
}

void
// Game-style keybinding UI: click button, press key to rebind (uses scancodes, not layout-dependent)
void
keyboard_settings_window() {
    if (igBegin("Keyboard Settings", NULL, 0)) {
        igText("Click a button and press a key to rebind:");
        igSeparator();

        char buf[32];
        int* rebinding = get_rebinding_target();

        // Each button shows the current scancode and waits for user input when clicked
        sprintf(buf, "Forward: %d%s", key_move_forward, rebinding == &key_move_forward ? " [WAITING]" : "");
        if (igButton(buf, (ImVec2){150, 0})) {
            interactive_start_rebind(&key_move_forward);
        }

        sprintf(buf, "Left: %d%s", key_move_left, rebinding == &key_move_left ? " [WAITING]" : "");
        if (igButton(buf, (ImVec2){150, 0})) {
            interactive_start_rebind(&key_move_left);
        }

        sprintf(buf, "Back: %d%s", key_move_back, rebinding == &key_move_back ? " [WAITING]" : "");
        if (igButton(buf, (ImVec2){150, 0})) {
            interactive_start_rebind(&key_move_back);
        }

        sprintf(buf, "Right: %d%s", key_move_right, rebinding == &key_move_right ? " [WAITING]" : "");
        if (igButton(buf, (ImVec2){150, 0})) {
            interactive_start_rebind(&key_move_right);
        }

        sprintf(buf, "Up: %d%s", key_move_up, rebinding == &key_move_up ? " [WAITING]" : "");
        if (igButton(buf, (ImVec2){150, 0})) {
            interactive_start_rebind(&key_move_up);
        }

        sprintf(buf, "Down: %d%s", key_move_down, rebinding == &key_move_down ? " [WAITING]" : "");
        if (igButton(buf, (ImVec2){150, 0})) {
            interactive_start_rebind(&key_move_down);
        }
    }
    igEnd();
}

void
clusters_window() {
    char buf[256];

    float window_width = 400.0f;
    float padding = 10.0f;
    ImGuiViewport* viewport = igGetMainViewport();

    float window_x = viewport->WorkPos.x + viewport->WorkSize.x - padding;
    float window_y = viewport->WorkPos.y + padding;

    ImVec2_c pos = { .x = window_x, .y = window_y };
    ImVec2_c pivot = { .x = 1.0f, .y = 0.0f };

    igSetNextWindowPos(pos, ImGuiCond_Always, pivot);
    ImVec2_c size = { .x = window_width, .y = viewport->WorkSize.y - 2 * padding };
    igSetNextWindowSize(size, ImGuiCond_Always);

    // === ОТКЛЮЧАЕМ скроллбары и скролл мышью в основном окне ===
    // ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
    ImGuiWindowFlags flags = 0;
    sprintf(buf, "Picked:%d-%d", (int)picked_cluster, picked_id);
    if (igBegin(buf, NULL, flags)) {
        // === 1. Выводим заголовок (picked message) ===
        igPushFont(NULL, 20);
        igTextWrapped("%d-%d\n%s", (int)picked_cluster, picked_id, data->messages[picked_id]);
        igPopFont();

        // === 2. Вычисляем доступную высоту для двух списков ===
        float available_height = igGetContentRegionAvail().y;
        float list_height = (available_height - igGetStyle()->ItemSpacing.y);
        
        // === 3. Первый список: "Соседи" (БЕЗ горизонтального скролла) ===
        if(found_cnt>0) {
            igSeparatorText("Classification");

            if(!classify_in_progress) {

                igInputTextMultiline(
                    "##prompt", 
                    classify_prompt, 
                    PROMPT_SIZE, 
                    (ImVec2){-1.0, igGetTextLineHeight()*4}, 
                    ImGuiInputTextFlags_WordWrap, 
                    0, NULL);
                if(igButton("Classify##classify", (ImVec2){0.0, 0.0})){
                    classify_found();
                    classify_in_progress = 1;
                };
                // igInputTextMultiline(const char *label, char *buf, size_t buf_size, const ImVec2_c size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *user_data)
            } else {
                igProgressBar(-1.0*igGetTime(), (ImVec2){0.0, 0.0}, "Classify");
            }
            
            if(classify_response_buf[0]!=0) {
                igSeparatorText("Classification result");
                igTextWrapped(classify_response_buf);
                if (igButton("Add label", (ImVec2){0.0,0.0})) {
                   add_label(classify_response_buf);
                }
                igSeparator();
            }
            
            igSeparatorText("Selected / Search result");
            if(igButton("Copy##copy_search", (ImVec2){0.0,0.0})){
                copy_found_to_buffer(copy_buffer, COPY_BUFFER_SIZE, 1);
            };
            for (int i = 0; i < found_cnt; i++) {
                igTextWrapped("%s", found_messages[i]);
            }
        }
    
        // Флаги БЕЗ горизонтального скролла
        // ImGuiWindowFlags child_flags = 0; // ← просто 0
        // if (igBeginChild_Str("NeighborsList", (ImVec2_c){0, list_height}, false, child_flags)) {
        //     for (int i = 0; i < found_cnt; i++) {
        //         igTextWrapped("%s", found_messages[i]);
        //     }
        // }
        // igEndChild();

        // // === 4. Второй список: "Сообщения кластера" ===
        // igSeparatorText("Сообщения кластера");
        // if(igButton("Copy##copy_cluster", (ImVec2){0.0,0.0})){};
        // if (igBeginChild_Str("ClusterList", (ImVec2_c){0, list_height}, false, child_flags)) {
     
        //     for (int i = 0; i < picked_cluster_count; i++) {
        //         igTextWrapped("%s", cluster_messages[i]);
        //     }
        // }
        // igEndChild();
    }
    igEnd();
}

void 
columns_window() {

    char buf[2048];

    float alpha_slider_min = 0.0;
    float alpha_slider_max = 1.0;
    float point_size_min   = 1.0;
    float point_size_max   = 9.0;

    igBegin("columns", NULL, 0);
    char* col_name = data->header[gui_col_id]; 
    sprintf(buf, "%s\nPid:\t%d\nCnt:\t%5.0f\nSum:\t%5.2f\nMin:\t%5.2f\nMax:\t%5.2f", 
            col_name,
            picked_id,
            data->notzero[gui_col_id], data->sum[gui_col_id],
            data->min[gui_col_id], data->max[gui_col_id]);
    igText(buf);
    // igText(data->messages[picked_id]);
    igSeparatorText("Add marker");
    label_input_widget();

    igSeparatorText("Search");
    search_input_widget();
   
    igSeparatorText("Visualisation settings");

    igCheckbox("Debug: Show ID render", (bool*)&debug_show_picking);

    // float gui_camera_rx = 30.0;
    // float gui_camera_ry = 30.0;
    float r_min = -180.0;
    float r_max = 180.0;
    float s_min = 1.0;
    float s_max = 50.0;
    
    // CIMGUI_API bool igSliderInt(const char* label,int* v,int v_min,int v_max,const char* format,ImGuiSliderFlags flags);
    // igSliderInt("render_id", &render_id,0,1, NULL,0);

    // igSliderScalar("scale",ImGuiDataType_Float, &gui_camera_radius, &s_min, &s_max, NULL, 1.f);
    // igSliderScalar("rx",ImGuiDataType_Float, &gui_camera_rx, &r_min, &r_max, NULL, 1.f);
    // igSliderScalar("ry",ImGuiDataType_Float, &gui_camera_ry, &r_min, &r_max, NULL, 1.f);

    // igSliderScalar("u",ImGuiDataType_Float, &gui_rot_u, &r_min, &r_max, NULL, 1.f);
    // igSliderScalar("v",ImGuiDataType_Float, &gui_rot_v, &r_min, &r_max, NULL, 1.f);
    
    static int cluster_id=0;
    if(igSliderInt("Cluster id", &cluster_id,data->min[CID_COL],data->max[CID_COL], NULL,0)) {
        gui_min = cluster_id;
        gui_max = cluster_id+1;
    }

    igSliderScalar("min",ImGuiDataType_Float, &gui_min, &data->min[gui_col_id], &data->max[gui_col_id], NULL, 1.f);
    if(gui_max<gui_min) gui_max = gui_min+1.0;
    igSliderScalar("max",ImGuiDataType_Float, &gui_max, &data->min[gui_col_id], &data->max[gui_col_id], NULL, 1.f);
    if(gui_min>gui_max) gui_min = gui_max-1.0;
    igSliderScalar("point size",ImGuiDataType_Float, &gui_point_size, &point_size_min, &point_size_max, NULL, 1.f);
    igSliderScalar("alpha 1",ImGuiDataType_Float, &gui_alpha_1, &alpha_slider_min, &alpha_slider_max, NULL, 1.f);
    igSliderScalar("alpha 2",ImGuiDataType_Float, &gui_alpha_2, &alpha_slider_min, &alpha_slider_max, NULL, 1.f);

    int min = 0; int max = data->cols-1;
    bool gui_col_changed = igSliderScalar("col #", ImGuiDataType_U32, &gui_col_id, &min, &max,  "%u", 1.f);
    gui_col_changed = gui_col_changed || igListBox_Str_arr("col", &gui_col_id, (const char* const*)data->header, data->cols+1, 10);
    if(gui_col_changed) update_min_max();

    igSeparatorText("Help");
    igTextWrapped(
        "Controls:\n"
        " - LMouse         - Select one sample\n"
        " - LMouse + Ctrl  - Select nearest\n"
        " - LMouse + Shift - Select cluster\n"
        " - RMouse         - Rotate\n"
    );

    igEnd();
}


void
draw_axis(scene_t* scene) {
    // CIMGUI_API ImDrawList* igGetBackgroundDrawList(ImGuiViewport* viewport)
    ImGuiViewport* vp = igGetMainViewport();
    ImDrawList* idl = igGetBackgroundDrawList(vp);
    // ImDrawList* idl = igGetForegroundDrawList_WindowPtr();
    float s = 10.0;
    vec4 w0 = {   0.0,   0.0,   0.0, 1.0}; 
    vec4 wx = {     s,   0.0,   0.0, 1.0};
    vec4 wy = {   0.0,     s,   0.0, 1.0};
    vec4 wz = {   0.0,   0.0,     s, 1.0};
    
    vec4 p0, px, py, pz;    
    world2screen(w0, scene->mvp, p0);
    world2screen(wx, scene->mvp, px);
    world2screen(wy, scene->mvp, py);
    world2screen(wz, scene->mvp, pz);

    ImDrawList_AddLine(idl, (ImVec2) {p0[0], p0[1]}, (ImVec2){px[0], px[1]}, 0x7FFF0000,1.0);
    ImDrawList_AddLine(idl, (ImVec2) {p0[0], p0[1]}, (ImVec2){py[0], py[1]}, 0x7F00FF00,1.0);
    ImDrawList_AddLine(idl, (ImVec2) {p0[0], p0[1]}, (ImVec2){pz[0], pz[1]}, 0x7F0000FF,1.0);
}

void
draw_markers(scene_t* scene) {
    
    char buf[2048]={0}; 
    
    ImGuiViewport* vp = igGetMainViewport();
    // ImDrawList* idl = igGetBackgroundDrawList(vp);
    ImDrawList* idl = igGetBackgroundDrawList(vp);

    // unsigned int row = data->max_id[gui_col_id];
    // float* max_row = &data->data[row*data->cols];
    // vec4 pos = {max_row[0], max_row[1], max_row[2], 1.0};
    float* picked = &data->data[picked_id * data->cols];
    vec4 pos = {picked[0], picked[1], picked[2], 1.0};

    vec4 prj; 
    // w2s(prj, scene->mvp, pos);
    world2screen(pos, scene->mvp, prj);
    
    ImVec2 c = {prj[0], prj[1]};

    float r = 20.0;
    ImDrawList_AddCircle(idl, c, r, 0x7fFFFFFF,16,1.0);
    
    // sprintf(buf, "Максимум: %5.2f", data->messages[picked_id]);
    // printf("%s\n", buf);
    // void ImDrawList_AddText_FontPtr(ImDrawList* self,ImFont* font,float font_size,const ImVec2_c pos,ImU32 col,const char* text_begin,const char* text_end,float wrap_width,const ImVec4* cpu_fine_clip_rect);
    // void ImDrawList_AddText_Vec2(
    // ImDrawList* self,
    // const ImVec2_c pos,
    // ImU32 col,
    // const char* text_begin,
    // const char* text_end);
    sprintf(buf, "%d-%d", (int)picked_cluster, picked_id);
    ImVec2 ts = igCalcTextSize(buf, NULL, 0, 500.0);
    ImDrawList_AddText_Vec2(
        idl, (ImVec2) {c.x-ts.x/2.0, c.y+r+5.0}, 
        0xAFFFFFFF, buf, NULL);

    // ts = igCalcTextSize(data->header[gui_col_id], NULL, 0, 500.0);
    // ImDrawList_AddText(
    //     idl, (ImVec2) {c.x-ts.x/2.0, c.y-ts.y-r-5.0}, 
    //     0xAFFFFFFF, data->header[gui_col_id], NULL);
}


void
draw_labels(scene_t* scene) {
    
    char buf[2048]={0}; 
    
    ImGuiViewport* vp = igGetMainViewport();
    // ImDrawList* idl = igGetBackgroundDrawList(vp);
    ImDrawList* idl = igGetBackgroundDrawList(vp);
    igPushFont(NULL, labels_size);
    for(int i=0; i<data->labels->cnt; i++) { 
        vec4 prj;
        label_t* l = &data->labels->list[i];
        vec3 pos = {l->x, l->y, l->z};
        world2screen(pos, scene->mvp, prj);
        
        ImVec2 c = {prj[0], prj[1]};

        float r = 5.0;
        ImDrawList_AddCircle(idl, c, r, 0x7FFFFFFF,16,1.0);
        
        ImVec2 ts = igCalcTextSize(l->label, NULL, 0, 500.0);
        ImDrawList_AddText_Vec2(
            idl, (ImVec2) {c.x-ts.x/2.0, c.y+r+5.0}, 
            0x7FFFFFFF, l->label, NULL);
    }
    igPopFont();
}

// https://www.songho.ca/opengl/gl_transform.html#wincoord
static void world2screen(vec4 in, mat4 mvp, vec4 out) {
    vec4 vp = {0.0, 0.0, screen_width, screen_height };
    glm_project(in, mvp, vp, out);
    out[1] =screen_height - out[1];
}

static char* format_float(const char* format, float f) {
    static char tmp[128];
    char src[128];
    sprintf(src, format, f);
    int l = strlen(src);
    char* s = &src[l];
    char* d = &tmp[127];

    while(1) {
        *d = *s; d--; s--;
        if(d == tmp-1 || s == src-1) return d++;
        if(d[1] == '.') break;
    }

    int i = 0;
    while(1) {
        *d = *s; d--; s--; i++;
        if(s == src-1) break;
        if(i%3==0) {*d=' '; d--;};
        if(d == tmp-1) return d++;
    }
    d++;
    return d;
}

void gui_render() {
    igRender();
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    // // ImGui_ImplOpenGL3_RenderDrawData(idd);
};


void gui_terminate(){
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(ctx);
};
