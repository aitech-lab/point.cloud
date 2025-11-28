#include <stdlib.h>
#include <stdio.h>

#include "shader.h"

static void check_shader(GLuint sid);
static void load_file( char*  filename,  char** buf);

shader_p 
shader_ctor(char* shader_name) {
    
    shader_p shader = calloc(1, sizeof(shader_t));
    
    char vert_name[256];
    char frag_name[256];
    char* vert_text;
    char* frag_text;
    
    sprintf(vert_name, "shaders/%s.vert", shader_name);
    sprintf(frag_name, "shaders/%s.frag", shader_name);
    
    load_file(vert_name, &vert_text);
    load_file(frag_name, &frag_text);

    GLuint vert_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_id, 1, (const char**)&vert_text, NULL);
    glCompileShader(vert_id);
    check_shader(vert_id);
    
    GLuint frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_id, 1, (const char**)&frag_text, NULL);
    glCompileShader(frag_id);
    check_shader(frag_id);

    shader->prog = glCreateProgram();
    glAttachShader(shader->prog, vert_id);
    glAttachShader(shader->prog, frag_id);
    glLinkProgram (shader->prog);

    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    
    shader->mvp        = glGetUniformLocation(shader->prog, "mvp");
    shader->rot        = glGetUniformLocation(shader->prog, "rot");
    shader->off        = glGetUniformLocation(shader->prog, "off");
    shader->min        = glGetUniformLocation(shader->prog, "min");
    shader->max        = glGetUniformLocation(shader->prog, "max");
    shader->render_id  = glGetUniformLocation(shader->prog, "render_id");
    shader->point_size = glGetUniformLocation(shader->prog, "point_size");
    shader->alpha_1    = glGetUniformLocation(shader->prog, "alpha_1");
    shader->alpha_2    = glGetUniformLocation(shader->prog, "alpha_2");

    // cleanup
    free(frag_text);
    free(vert_text);

    return shader;
}

void
shader_dtor(shader_p shader) {
    glDeleteProgram(shader->prog);
    free(shader);
}

void
shader_start(shader_p shader) {
    // SET SHADER
    glUseProgram(shader->prog);
}

void
shader_stop(shader_p shader) {
	glUseProgram(0);
}


/////////////////////////////////////////////////////////////////

static void 
load_file(
    char*  filename, 
    char** buf) {

    long length = 0;
    FILE* fp = fopen (filename, "rb");
    
    if(fp) {
        fseek (fp, 0, SEEK_END);
        length = ftell(fp);
        fseek (fp, 0, SEEK_SET);
        *buf = malloc(length+1);
        if (*buf) {
            fread(*buf, 1, length, fp);
        }
        fclose (fp);
        (*buf)[length]=0;
    }
}

static void 
check_shader(GLuint sid) {
    GLint isCompiled = 0;
    glGetShaderiv(sid, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(sid, GL_INFO_LOG_LENGTH, &maxLength);
        // The maxLength includes the NULL character
        GLchar error[maxLength];
        glGetShaderInfoLog(sid, maxLength, &maxLength, error);
        fprintf(stderr, "ERR: %s\n", error);

        // Provide the infolog in whatever manor you deem best.
        glDeleteShader(sid); // Don't leak the shader.
        return;
    }
}
