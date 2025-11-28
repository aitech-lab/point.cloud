#pragma once

#include <GL/gl3w.h>
#include <cglm/cglm.h>

typedef struct obj_t {
    GLuint vao;
    GLuint vbo;
    GLuint vbo_dynamic;
    mat4 m;
    size_t cols;
    size_t rows;
    
} obj_t;
typedef obj_t* obj_p;

obj_p obj_ctor();
void obj_dtor(obj_p obj);

obj_p obj_cloud();
void obj_render(obj_p obj);
void obj_update_dynamic(obj_p obj);


