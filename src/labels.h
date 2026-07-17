// src/labels.h
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LABELS_MAX 1000
#define LABEL_TEXT_MAX 128

typedef struct {
    float x, y, z;
    char label[LABEL_TEXT_MAX];
} label_t;

typedef struct {
    label_t list[LABELS_MAX];
    int cnt;
} labels_t;

// Загружает метки из файла filename. Возвращает указатель на labels_t или NULL при ошибке.
labels_t* labels_load(char* filename);

// Добавляет метку в структуру и сразу сохраняет в файл.
// Возвращает 0 при успехе, -1 при ошибке (например, переполнение).
int labels_add(labels_t* labels, float x, float y, float z, const char* text);

// Освобождает память (в данном случае ничего не нужно — структура не содержит динамической памяти).
void labels_free(labels_t* labels);
