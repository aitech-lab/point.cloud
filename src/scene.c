
#include "bbgl.h"
#include "scene.h"
#include "obj.h"
#include "shader.h"
#include "interactive.h"
#include "gui.h"
#include "globals.h"
#include "data.h"
#include <GL/gl.h>

shader_p shader;

scene_p
scene_ctor() {
    scene_p scene = calloc(1, sizeof(scene_t));
    kv_init(scene->objects);
    scene->fov = 30.0;
    scene->f = 100.0;
    scene->n = 0.001;
    shader = shader_ctor("simple");
    obj_p o = obj_cloud();
    scene_add_obj(scene, o);
    
	return scene;
}

void 
scene_dtor(scene_p scene) { 
    // free objects
    for(size_t i=0; i<scene->objects.n; i++) {
        obj_dtor(scene->objects.a[i]);
    }
	kv_destroy(scene->objects);
	free(scene);
}

void 
scene_add_obj(scene_p scene, obj_p obj) {
    kv_push(obj_p, scene->objects, obj);
}

void 
scene_render(scene_p scene) {

    gui_camera_tx += (gui_camera_target_tx - gui_camera_tx)/10.0;
    gui_camera_ty += (gui_camera_target_ty - gui_camera_ty)/10.0;
    gui_camera_tz += (gui_camera_target_tz - gui_camera_tz)/10.0;

    float x = gui_camera_radius*sin(gui_camera_rx/57.3)*sin(gui_camera_ry/57.3);
    float z = gui_camera_radius*sin(gui_camera_rx/57.3)*cos(gui_camera_ry/57.3);
    float y = gui_camera_radius*cos(gui_camera_rx/57.3);

    glm_perspective(scene->fov, ratio, scene->n, scene->f, scene->p);
    glm_lookat(
    	(vec3){gui_camera_tx+x, gui_camera_ty+y, gui_camera_tz+z}, 
    	(vec3){gui_camera_tx  , gui_camera_ty  , gui_camera_tz  },
    	(vec3){0.0, 1.0, 0.0},
        scene->v);

    shader_start(shader);
    for(size_t i=0; i<scene->objects.n; i++) {
        
        obj_p o = scene->objects.a[i];
        
        glm_mat4_mul(scene->p, scene->v, scene->mvp);
        glm_mat4_identity(scene->rot);

        glUniformMatrix4fv(shader->mvp, 1, GL_FALSE, (const GLfloat*) scene->mvp);
        // glUniformMatrix4fv(shader->rot, 1, GL_FALSE, (const GLfloat*) scene->rot);
        glUniform1f(shader->off,        (const GLfloat) gui_off_u);
        glUniform1i(shader->render_id,  (const GLint) render_id);
        glUniform1f(shader->min,        (const GLfloat) gui_min);
        glUniform1f(shader->max,        (const GLfloat) gui_max);
        glUniform1f(shader->alpha_1,    (const GLfloat) gui_alpha_1);
        glUniform1f(shader->alpha_2,    (const GLfloat) gui_alpha_2);
        glUniform1f(shader->point_size, (const GLfloat) gui_point_size);
            
        obj_render(o);
    }
    shader_stop(shader);

    // Читаем пиксель
    if (render_id) {
        unsigned char rgba[4] = {0};
        glReadPixels(mouse_x, screen_height - 1 - mouse_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        if(rgba[3]!=0) {
            picked_id = (rgba[0] << 24) | (rgba[1] << 16) | (rgba[2] << 8) | rgba[3];
            picked_id >>= 8;
            // picked_id -= 11;
            if(picked_id<0) picked_id = 0;
            printf("%x %x %x %x = %09d\n", rgba[0], rgba[1], rgba[2], rgba[3], picked_id);
            
            float* picked = &data->data[picked_id*data->cols];
            gui_camera_target_tx = picked[0];
            gui_camera_target_ty = picked[1];
            gui_camera_target_tz = picked[2];

            picked_cluster = picked[3];
            // gui_min = picked_cluster;
            // gui_max = picked_cluster;
        }

        render_id = 0;
    }

}

/*
uint32_t 
scene_pick(scene_p s, int x, int y) {
    glBindFramebuffer(GL_FRAMEBUFFER, s->pick_fbo);

    // Установить размер буфера под текущее окно (или использовать постоянный)
    glViewport(0, 0, screen_width, screen_height);

    // Очистить
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Отключить blending!
    glDisable(GL_BLEND);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    // Использовать pick-шейдер
    shader_start(s->pick_shader);

    glUniformMatrix4fv(s->pick_mvp_loc, 1, GL_FALSE, (const GLfloat*)s->mvp);

    // Рисуем облако точек
    for(size_t i = 0; i < s->objects.n; i++) {
        obj_render(s->objects.a[i]); // предполагается, что obj_render использует VAO
    }

    shader_stop(s->pick_shader);

    // Читаем пиксель
    unsigned char pixel[4] = {0};
    glReadPixels(x, screen_height - 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    // Включить blending обратно
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Возврат к основному буферу
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Декодировать
    uint32_t id = decode_color(pixel);

    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0 && pixel[3] == 0) {
        return UINT32_MAX; // ничего не попало
    }

    return id;
}
*/