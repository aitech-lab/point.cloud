// src/labels.c
#include "labels.h"
#include <string.h>

static char labels_filename[1024];

labels_t* labels_load(char* filename) {
    
    strcpy(labels_filename, filename);

    FILE* fp = fopen(labels_filename, "r");
    if (!fp) {
        // File doesn't exist - this is normal, return empty structure
        labels_t* labels = calloc(1, sizeof(labels_t));
        return labels;
    }

    labels_t* labels = calloc(1, sizeof(labels_t));
    char line[512];
    while (labels->cnt < LABELS_MAX && fgets(line, sizeof(line), fp)) {
        // Remove newline character
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        // Skip empty lines
        if (line[0] == '\0') continue;

        label_t* l = &labels->list[labels->cnt];
        // Format: x y z "label_text"
        // Parse 3 floats, remainder is text
        char* rest = NULL;
        l->x = strtof(line, &rest);
        if (!rest || *rest != ' ') goto error;
        l->y = strtof(rest, &rest);
        if (!rest || *rest != ' ') goto error;
        l->z = strtof(rest, &rest);
        if (!rest) goto error;

        // Skip whitespace
        while (*rest == ' ') rest++;
        // Copy remaining text, removing quotes if present
        size_t len = strlen(rest);
        if (len > 0) {
            char* start = rest;
            char* end = rest + len - 1;
            if (*start == '"' && *end == '"') {
                start++;
                *end = '\0';
            }
            strncpy(l->label, start, LABEL_TEXT_MAX - 1);
            l->label[LABEL_TEXT_MAX - 1] = '\0';
        } else {
            l->label[0] = '\0';
        }

        labels->cnt++;
    }
    fclose(fp);
    return labels;

error:
    fprintf(stderr, "Error parsing line: %s\n", line);
    fclose(fp);
    free(labels);
    return NULL;
}

int labels_add(labels_t* labels, float x, float y, float z, const char* text) {
    if (!labels || !labels_filename || !text) return -1;
    if (labels->cnt >= LABELS_MAX) return -1;

    label_t* l = &labels->list[labels->cnt];
    l->x = x;
    l->y = y;
    l->z = z;
    strncpy(l->label, text, LABEL_TEXT_MAX - 1);
    l->label[LABEL_TEXT_MAX - 1] = '\0';
    labels->cnt++;

    // Save immediately
    FILE* fp = fopen(labels_filename, "w");
    if (!fp) {
        perror("Cannot open label file for writing");
        return -1;
    }
    for (int i = 0; i < labels->cnt; ++i) {
        fprintf(fp, "%.6f %.6f %.6f \"%s\"\n",
                labels->list[i].x,
                labels->list[i].y,
                labels->list[i].z,
                labels->list[i].label);
    }
    fclose(fp);
    return 0;
}

void labels_free(labels_t* labels) {
    free(labels);
}