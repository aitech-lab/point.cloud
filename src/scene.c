
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

// Resize off-screen FBO when window changes size
static void
scene_resize_pick_fbo(scene_p scene, int width, int height) {
    scene->fbo_width = width;
    scene->fbo_height = height;

    glBindFramebuffer(GL_FRAMEBUFFER, scene->pick_fbo);

    // Resize color texture for GPU picking (encodes vertex IDs as colors)
    glBindTexture(GL_TEXTURE_2D, scene->pick_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    // Resize depth buffer for picking render
    glBindRenderbuffer(GL_RENDERBUFFER, scene->pick_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

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

    glGenFramebuffers(1, &scene->pick_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, scene->pick_fbo);

    glGenTextures(1, &scene->pick_texture);
    glBindTexture(GL_TEXTURE_2D, scene->pick_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->pick_texture, 0);

    glGenRenderbuffers(1, &scene->pick_rbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, scene->pick_rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    scene_resize_pick_fbo(scene, screen_width, screen_height);
	return scene;
}

void
scene_dtor(scene_p scene) {
    // free objects
    for(size_t i=0; i<scene->objects.n; i++) {
        obj_dtor(scene->objects.a[i]);
    }
	kv_destroy(scene->objects);
	glDeleteFramebuffers(1, &scene->pick_fbo);
	glDeleteTextures(1, &scene->pick_texture);
	glDeleteRenderbuffers(1, &scene->pick_rbo);
	free(scene);
}

void
scene_add_obj(scene_p scene, obj_p obj) {
    kv_push(obj_p, scene->objects, obj);
}

// GPU picking: render point cloud to off-screen FBO with vertex IDs encoded as colors
// Read pixel at mouse position to extract picked point ID and focus camera on it
static void
scene_do_picking(scene_p scene) {
    glBindFramebuffer(GL_FRAMEBUFFER, scene->pick_fbo);
    glViewport(0, 0, screen_width, screen_height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_start(shader);
    glUniformMatrix4fv(shader->mvp, 1, GL_FALSE, (const GLfloat*) scene->mvp);
    glUniform1f(shader->off, app_ctx.off_u);
    glUniform1i(shader->render_id, 1);  // Tell shader to encode vertex ID instead of coloring
    glUniform1f(shader->min, app_ctx.min);
    glUniform1f(shader->max, app_ctx.max);
    glUniform1f(shader->alpha_1, app_ctx.alpha_1);
    glUniform1f(shader->alpha_2, app_ctx.alpha_2);
    glUniform1f(shader->point_size, app_ctx.point_size);

    for(size_t i=0; i<scene->objects.n; i++) {
        obj_render(scene->objects.a[i]);
    }

    shader_stop(shader);
    glDisable(GL_BLEND);

    // Read pixel at mouse cursor and decode it to get the point's vertex index
    unsigned char rgba[4] = {0};
    glReadPixels(mouse_x, screen_height - 1 - mouse_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    if(rgba[3]!=0) {
        // Decode 24-bit vertex ID from RGBA (24 bits fit in RGB channels)
        picked_id = (rgba[0] << 24) | (rgba[1] << 16) | (rgba[2] << 8) | rgba[3];
        picked_id >>= 8;
        if(picked_id<0) picked_id = 0;
        printf("%x %x %x %x = %09d\n", rgba[0], rgba[1], rgba[2], rgba[3], picked_id);

        // Move camera target to the picked point's XYZ coordinates
        float* picked = &data->data[picked_id*data->cols];
        app_ctx.camera_target_tx = picked[0];
        app_ctx.camera_target_ty = picked[1];
        app_ctx.camera_target_tz = picked[2];
        picked_cluster = picked[3];
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
scene_render(scene_p scene) {

    if (scene->fbo_width != screen_width || scene->fbo_height != screen_height) {
        scene_resize_pick_fbo(scene, screen_width, screen_height);
    }

    app_ctx.camera_tx += (app_ctx.camera_target_tx - app_ctx.camera_tx)/10.0;
    app_ctx.camera_ty += (app_ctx.camera_target_ty - app_ctx.camera_ty)/10.0;
    app_ctx.camera_tz += (app_ctx.camera_target_tz - app_ctx.camera_tz)/10.0;

    // Compute camera position: rotate offset backward along the view
    vec3 cam_offset = {0.0f, 0.0f, app_ctx.camera_radius};
    vec3 cam_pos;
    glm_quat_rotatev(app_ctx.camera_quat, cam_offset, cam_pos);

    vec3 camera_world_pos = {app_ctx.camera_tx+cam_pos[0], app_ctx.camera_ty+cam_pos[1], app_ctx.camera_tz+cam_pos[2]};
    vec3 target_pos = {app_ctx.camera_tx, app_ctx.camera_ty, app_ctx.camera_tz};
    vec3 view_dir;
    glm_vec3_sub(target_pos, camera_world_pos, view_dir);
    glm_vec3_normalize(view_dir);

    // Gimbal lock fix: compute stable up vector that handles vertical viewing angles
    // When camera looks straight up/down, the usual up may be parallel to view_dir
    vec3 local_up = {0.0f, 1.0f, 0.0f};
    vec3 rotated_up;
    glm_quat_rotatev(app_ctx.camera_quat, local_up, rotated_up);

    vec3 right;
    glm_vec3_cross(view_dir, rotated_up, right);
    float right_len = glm_vec3_norm(right);

    vec3 final_up;
    if (right_len < 0.01f) {
        // Degenerate case: view direction is nearly parallel to rotated_up
        // Use world right instead to compute stable perpendicular up
        vec3 world_right = {1.0f, 0.0f, 0.0f};
        glm_vec3_cross(world_right, view_dir, final_up);
        glm_vec3_normalize(final_up);
    } else {
        // Normal case: compute orthonormal up from view_dir and right
        glm_vec3_normalize(right);
        glm_vec3_cross(view_dir, right, final_up);
        glm_vec3_normalize(final_up);
    }

    glm_perspective(scene->fov, ratio, scene->n, scene->f, scene->p);
    glm_lookat(camera_world_pos, target_pos, final_up, scene->v);

    glm_mat4_mul(scene->p, scene->v, scene->mvp);
    glm_mat4_identity(scene->rot);

    if (app_ctx.debug_show_picking) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screen_width, screen_height);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        shader_start(shader);
        for(size_t i=0; i<scene->objects.n; i++) {
            obj_p o = scene->objects.a[i];

            glUniformMatrix4fv(shader->mvp, 1, GL_FALSE, (const GLfloat*) scene->mvp);
            glUniform1f(shader->off,        (const GLfloat) app_ctx.off_u);
            glUniform1i(shader->render_id,  1);
            glUniform1f(shader->min,        (const GLfloat) app_ctx.min);
            glUniform1f(shader->max,        (const GLfloat) app_ctx.max);
            glUniform1f(shader->alpha_1,    (const GLfloat) app_ctx.alpha_1);
            glUniform1f(shader->alpha_2,    (const GLfloat) app_ctx.alpha_2);
            glUniform1f(shader->point_size, 10.0);

            obj_render(o);
        }
        shader_stop(shader);
        glDisable(GL_BLEND);
    } else {
        if (render_id) {
            scene_do_picking(scene);
            render_id = 0;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screen_width, screen_height);

        shader_start(shader);
        for(size_t i=0; i<scene->objects.n; i++) {
            obj_p o = scene->objects.a[i];

            glUniformMatrix4fv(shader->mvp, 1, GL_FALSE, (const GLfloat*) scene->mvp);
            glUniform1f(shader->off,        (const GLfloat) app_ctx.off_u);
            glUniform1i(shader->render_id,  0);
            glUniform1f(shader->min,        (const GLfloat) app_ctx.min);
            glUniform1f(shader->max,        (const GLfloat) app_ctx.max);
            glUniform1f(shader->alpha_1,    (const GLfloat) app_ctx.alpha_1);
            glUniform1f(shader->alpha_2,    (const GLfloat) app_ctx.alpha_2);
            glUniform1f(shader->point_size, (const GLfloat) app_ctx.point_size);

            obj_render(o);
        }
        shader_stop(shader);
    }
}

/*
uint32_t
scene_pick(scene_p s, int x, int y) {
    glBindFramebuffer(GL_FRAMEBUFFER, s->pick_fbo);

    // Set viewport size for current window
    glViewport(0, 0, screen_width, screen_height);

    // Clear buffers
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Disable blending for picking
    glDisable(GL_BLEND);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    // Use picking shader
    shader_start(s->pick_shader);

    glUniformMatrix4fv(s->pick_mvp_loc, 1, GL_FALSE, (const GLfloat*)s->mvp);

    // Render point cloud
    for(size_t i = 0; i < s->objects.n; i++) {
        obj_render(s->objects.a[i]);
    }

    shader_stop(s->pick_shader);

    // Read pixel at click position
    unsigned char pixel[4] = {0};
    glReadPixels(x, screen_height - 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    // Re-enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Return to main framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Decode vertex ID from pixel color
    uint32_t id = decode_color(pixel);

    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0 && pixel[3] == 0) {
        return UINT32_MAX; // No hit
    }

    return id;
}
*/