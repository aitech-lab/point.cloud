#pragma once

#include "labels.h"

typedef struct cluster_stat_t cluster_stat_t;
typedef struct kdtree kdtree;
typedef struct kdres kdres;

typedef struct {
    unsigned int cols;    // number of cols
    unsigned int rows;    // number of rows
    char** header;        // csv header
    float* data;          // data
    float* dynamic;       // dynamic payload
    float* min;           // min of col
    float* max;           // max of col
    unsigned int* min_id; // min row
    unsigned int* max_id; // max row
    float* sum;           // col sum
    float* notzero;       // notzero count
    int clusters_cnt;     // number of cluster, max_id - min_id 
    cluster_stat_t* clusters; // clusters stat

    char** messages;
    int messages_count;

    labels_t* labels;

    kdtree* index;

} data_t;
typedef data_t* data_p;

// category statistics
typedef struct cat_stat_t {
    int id;
    float sum;
} cat_stat_t;

typedef struct cluster_stat_t {
    int     id;          // cluster id
    int     cnt;         // members count
    float   sum;         // cluster categorie sum
    cat_stat_t* cat_sum; // per category sum
} cluster_stat_t;

void data_free(data_p csv);
data_p data_load(char* filename);

char** load_messages_from_gz(const char* filename, int* num_lines);
void free_messages(char** messages);

int data_add_label(const char* label, float x, float y, float z);

extern data_p data;