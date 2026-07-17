#pragma once

#include <kvec.h>
#include <cglm/cglm.h>

#include "obj.h"

typedef kvec_t(obj_p) objects_v;

typedef struct scene_t {
    objects_v objects;
    mat4 mvp;
    mat4 rot;
    mat4 p,v;
    float n,f; // near far
    float fov;
    GLuint pick_fbo;
    GLuint pick_texture;
    GLuint pick_rbo;
} scene_t;
typedef scene_t* scene_p;

scene_p scene_ctor();
void scene_dtor(scene_p scene);
void scene_render(scene_p scene);
void scene_add_obj(scene_p scene, obj_p obj);

