#pragma once

#include <GL/gl3w.h>

typedef struct {
	GLuint prog;       // shader id
	GLuint mvp;        // model view projection
	GLuint render_id;  // render id
	GLuint rot;        // rotation matrix
	GLuint off;        // u axis offset
	GLuint point_size; // size of point
	GLuint alpha_1;    // out alpha
	GLuint alpha_2;    // in alpha
	GLuint min;        // min in value 
	GLuint max;        // max in value
} shader_t;
typedef shader_t* shader_p;

shader_p shader_ctor(char* shader_name);
void shader_dtor(shader_p shader);

void shader_start(shader_p shader);
void shader_stop (shader_p shader);
