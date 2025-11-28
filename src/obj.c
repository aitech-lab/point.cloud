#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GL/gl3w.h>

#include "globals.h"
#include "obj.h"
#include "data.h"
#include "gui.h"


obj_p
obj_ctor() {
    obj_p obj = calloc(1, sizeof(obj_t));
    glm_mat4_identity(obj->m);
	return obj;
}


void
obj_dtor(obj_p obj) {
    glDeleteBuffers(1, &obj->vbo);
    glDeleteVertexArrays(1, &obj->vao);
	free(obj);
}


static void 
init_vao(obj_p obj, int col) {
    glBindVertexArray(obj->vao);
    
    // Атрибут 0: позиции (всегда из статического буфера)
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 
                          obj->cols * sizeof(float), (void*)0);

    // Атрибут 1: data — либо из колонки, либо из dynamic буфера
    if (col == obj->cols) {
        // Используем ДИНАМИЧЕСКИЙ буфер
        glBindBuffer(GL_ARRAY_BUFFER, obj->vbo_dynamic);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
    } else {
        // Используем КОЛОНКУ из статического буфера
        glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                              obj->cols * sizeof(float),
                              (void*)(col * sizeof(float)));
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}


obj_p 
obj_cloud() {
	
    obj_p obj = obj_ctor();
    obj->cols = data->cols;
    obj->rows = data->rows;

    // --- Оригинальный статический буфер (НЕ ТРОГАЕМ) ---
    glGenBuffers(1, &obj->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 obj->rows * obj->cols * sizeof(float),
                 data->data,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // --- НОВЫЙ: динамический буфер для data->dynamic ---
    glGenBuffers(1, &obj->vbo_dynamic);
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo_dynamic);
    glBufferData(GL_ARRAY_BUFFER,
                 obj->rows * sizeof(float),
                 data->dynamic,        // исходные значения
                 GL_DYNAMIC_DRAW);     // ← позволяет обновлять
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // --- VAO (временно не инициализируем; делаем это в obj_render) ---
    glGenVertexArrays(1, &obj->vao);

    return obj;
}


void 
obj_render(obj_p obj) {
    static int col_id = -1;
    if(gui_col_id!=col_id) {
        col_id = gui_col_id;
        init_vao(obj, col_id);
    }
    
    if(dynamic_data_updated){
        obj_update_dynamic(obj);
        dynamic_data_updated = 0;
    }

    glBindVertexArray(obj->vao);
    glDrawArrays(GL_POINTS, 0, obj->rows);
    glBindVertexArray(0);
}

void 
obj_update_dynamic(obj_p obj) {
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo_dynamic);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data->rows * sizeof(float), data->dynamic);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}