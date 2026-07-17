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


// Set up VAO: attribute 0 is always XYZ position, attribute 1 switches between data columns
// If col==cols, use dynamic buffer for per-point payload; otherwise read from static buffer column
static void
init_vao(obj_p obj, int col) {
    glBindVertexArray(obj->vao);

    // Attribute 0: position (XYZ from first 3 floats of each row, stride to skip other columns)
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
                          obj->cols * sizeof(float), (void*)0);

    // Attribute 1: color source (either a static column or dynamic buffer)
    if (col == obj->cols) {
        // Use dynamic buffer (updated per-frame for selection highlighting, search results)
        glBindBuffer(GL_ARRAY_BUFFER, obj->vbo_dynamic);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
    } else {
        // Use a column from the static data buffer (with stride to skip other columns)
        glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                              obj->cols * sizeof(float),
                              (void*)(col * sizeof(float)));
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}


// Create point cloud VBO and dynamic payload buffer
// Static buffer holds all point data; dynamic buffer holds per-frame state (search, selection)
obj_p
obj_cloud() {

    obj_p obj = obj_ctor();
    obj->cols = data->cols;
    obj->rows = data->rows;

    // Static VBO: entire flattened dataset (rows × cols floats)
    glGenBuffers(1, &obj->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 obj->rows * obj->cols * sizeof(float),
                 data->data,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Dynamic buffer: per-point payload updated each frame (e.g., search result highlight)
    glGenBuffers(1, &obj->vbo_dynamic);
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo_dynamic);
    glBufferData(GL_ARRAY_BUFFER,
                 obj->rows * sizeof(float),
                 data->dynamic,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // VAO initialized lazily in obj_render (after shader is bound)
    glGenVertexArrays(1, &obj->vao);

    return obj;
}


// Render point cloud: rebind VAO if coloring column changed, update dynamic buffer if needed
void
obj_render(obj_p obj) {
    // Rebind VAO only when user changes which column to color by
    static int col_id = -1;
    if(gui_col_id!=col_id) {
        col_id = gui_col_id;
        init_vao(obj, col_id);
    }

    // Update dynamic buffer if search results or selection changed this frame
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