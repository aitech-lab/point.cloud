#include "globals.h"

// Central rendering context
render_context_t app_ctx = {
    // Camera
    .camera_quat = {0.0f, 0.0f, 0.0f, 1.0f},
    .camera_radius = 40.0f,
    .camera_tx = 0.0f, .camera_ty = 0.0f, .camera_tz = 0.0f,
    .camera_target_tx = 0.0f, .camera_target_ty = 0.0f, .camera_target_tz = 0.0f,
    // Rendering
    .col_id = 0,
    .min = 0.0f, .max = 50.0f,
    .alpha_1 = 0.10f, .alpha_2 = 0.75f,
    .point_size = 1.0f,
    .off_u = 10.0f, .rot_u = 0.0f, .rot_v = 0.0f,
    // Input
    .render_id = 0, .picked_id = 0, .picked_cluster = -1.0f,
    .dynamic_data_updated = 0,
    // Search
    .pick_range = 0.1f,
    .do_search_nearest = 0, .do_search_cluster = 0,
    // UI
    .focused = 0,
    .debug_show_picking = 0,
    // Data config
    .cluster_col = 0, .categories_start = 6,
};

// Legacy accessors for gradual migration
int cluster_col;
int categories_start;

// Deprecated: use ctx directly
float gui_camera_radius;
versor gui_camera_quat;
float gui_camera_tx, gui_camera_ty, gui_camera_tz;
float gui_camera_target_tx, gui_camera_target_ty, gui_camera_target_tz;
float gui_off_u, gui_rot_u, gui_rot_v;
float gui_min, gui_max;
float gui_alpha_1, gui_alpha_2;
float gui_point_size;
int gui_col_id, gui_focused;
int render_id, picked_id;
float picked_cluster;
float pick_range;
int dynamic_data_updated;
int do_search_nearest, do_search_cluster;
int debug_show_picking;