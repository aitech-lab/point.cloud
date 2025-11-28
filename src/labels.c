// src/labels.c
#include "labels.h"

labels_t* labels_load(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        // Файл не существует — это нормально, вернём пустую структуру.
        labels_t* labels = calloc(1, sizeof(labels_t));
        return labels;
    }

    labels_t* labels = calloc(1, sizeof(labels_t));
    char line[512];
    while (labels->cnt < LABELS_MAX && fgets(line, sizeof(line), fp)) {
        // Убираем символ новой строки
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        // Пропускаем пустые строки
        if (line[0] == '\0') continue;

        label_t* l = &labels->list[labels->cnt];
        // Формат: x y z "label_text"
        // Будем считать, что текст идёт после первых трёх чисел и может содержать пробелы.
        // Простой подход: читаем 3 float, остаток — текст (без кавычек).
        char* rest = NULL;
        l->x = strtof(line, &rest);
        if (!rest || *rest != ' ') goto error;
        l->y = strtof(rest, &rest);
        if (!rest || *rest != ' ') goto error;
        l->z = strtof(rest, &rest);
        if (!rest) goto error;

        // Пропустить пробелы
        while (*rest == ' ') rest++;
        // Копируем остаток как текст, убирая кавычки по краям, если есть
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

int labels_add(labels_t* labels, const char* filename, float x, float y, float z, const char* text) {
    if (!labels || !filename || !text) return -1;
    if (labels->cnt >= LABELS_MAX) return -1;

    label_t* l = &labels->list[labels->cnt];
    l->x = x;
    l->y = y;
    l->z = z;
    strncpy(l->label, text, LABEL_TEXT_MAX - 1);
    l->label[LABEL_TEXT_MAX - 1] = '\0';
    labels->cnt++;

    // Сохраняем немедленно
    FILE* fp = fopen(filename, "w");
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