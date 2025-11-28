#include "globals.h"

int cluster_col;
int categories_start;

float gui_camera_radius = 40.0;

float gui_camera_rx     = 30.0;
float gui_camera_ry     = 30.0;

float gui_camera_tx     = 0.0f;
float gui_camera_ty     = 0.0f;
float gui_camera_tz     = 0.0f;
float gui_camera_target_tx     = 0.0f;
float gui_camera_target_ty     = 0.0f;
float gui_camera_target_tz     = 0.0f;

float gui_off_u = 10.0;
float gui_rot_u = 0.0;
float gui_rot_v = 0.0;

float gui_min  = 0.0;
float gui_max  = 50.0;

float gui_alpha_1    = 0.10;
float gui_alpha_2    = 0.75;

float gui_point_size = 1.0;

int gui_col_id = 0;
int gui_focused = 0;

int render_id;
int picked_id;
float picked_cluster = -1.0;
float pick_range = 0.1;

int dynamic_data_updated = 0;