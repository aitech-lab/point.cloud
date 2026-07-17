#pragma once

#include <cglm/cglm.h>

// Central context holding all rendering and UI state
typedef struct {
    // Camera state
    versor camera_quat;
    float camera_radius;
    float camera_tx, camera_ty, camera_tz;
    float camera_target_tx, camera_target_ty, camera_target_tz;

    // Rendering parameters
    int col_id;              // Which column to color by
    float min, max;          // Min/max values for current column
    float alpha_1, alpha_2;  // Point alpha range
    float point_size;
    float off_u;             // Offset (unused)
    float rot_u, rot_v;      // Rotation (unused)

    // Input state
    int render_id;           // Trigger GPU picking
    int picked_id;           // Last picked point
    float picked_cluster;    // Cluster of picked point
    int dynamic_data_updated; // Flag: recalculate dynamic buffer

    // Search state
    float pick_range;
    int do_search_nearest;
    int do_search_cluster;

    // UI state
    int focused;
    int debug_show_picking;

    // Data configuration
    int cluster_col;
    int categories_start;
} render_context_t;

extern render_context_t app_ctx;

// Legacy accessors (for gradual migration)
extern int cluster_col;
extern int categories_start;

extern float gui_camera_radius;
extern versor gui_camera_quat;

extern float gui_camera_tx;
extern float gui_camera_ty;
extern float gui_camera_tz;
extern float gui_camera_target_tx;
extern float gui_camera_target_ty;
extern float gui_camera_target_tz;

extern float gui_off_u;
extern float gui_rot_u;
extern float gui_rot_v;
extern int gui_focused;
extern int gui_col_id;
extern float gui_min;
extern float gui_max;
extern float gui_alpha_1;
extern float gui_alpha_2;
extern float gui_point_size;

extern int render_id;
extern int picked_id;
extern float picked_cluster;

extern int dynamic_data_updated;

extern float pick_range;
extern int do_search_nearest;
extern int do_search_cluster;
extern int debug_show_picking;