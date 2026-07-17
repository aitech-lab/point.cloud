#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "globals.h"
#include "data.h"
#include "labels.h"
#include "kdtree.h"

#define MAX_COLS 1024
#define LINE_SIZE 1024*1024

static void parse_header(data_t* data, char* header);
static void parse_clusters(data_t* data);


void data_free(data_t* data) {
    // cleanup header
    if(data) {
        for(size_t i = 0; i<=data->cols; i++) free(data->header[i]);
        free(data->header);
        free(data->data);
        free(data->dynamic);
        kd_free(data->index);
        free(data->min);
        free(data->max);
        free(data->min_id);
        free(data->max_id);
        free(data->sum);
        free(data->notzero);
        
        for(int i=0; i<data->clusters_cnt; i++) free(data->clusters[i].cat_sum);
        free(data->clusters);

        free_messages(data->messages);
        labels_free(data->labels);

        free(data);
    }
}

// Load gzipped TSV dataset: flatten into single float array and build spatial index
data_t*
data_load(char* prefix) {

    char filename[1024];

    sprintf(filename, "%s.floats.tsv.gz", prefix);

    printf("Loading %s\n", filename);

    // Open gzipped TSV file
    gzFile fp = gzopen(filename, "rb");
    if (!fp) return NULL;

    // Count total rows (for preallocation)
    data_t* data = calloc(1, sizeof(data_t));
    char line[LINE_SIZE];
    while(gzgets(fp, line, LINE_SIZE) != Z_NULL) {
        if(strchr(line, '\n')) data->rows++;
    }

    data->rows--;  // Subtract header
    printf("Counted: %d rows\n", data->rows);
    gzrewind(fp);

    // Parse TSV header to detect cluster column and data column count
    gzgets(fp, line, LINE_SIZE);
    parse_header(data, line);
    
    data->data    = calloc((data->rows+1) * data->cols, sizeof(float));
    data->dynamic = calloc((data->rows+1), sizeof(float));
    data->min     = calloc(data->cols+1, sizeof(float));
    data->max     = calloc(data->cols+1, sizeof(float));
    data->sum     = calloc(data->cols+1, sizeof(float));
    data->notzero = calloc(data->cols+1, sizeof(float));
    data->min_id  = calloc(data->cols+1, sizeof(unsigned int));
    data->max_id  = calloc(data->cols+1, sizeof(unsigned int));

    // Create 3D spatial index for nearest-neighbor and frustum queries
    data->index = kd_create(3);

    // Parse each row: flatten into data array, track min/max/sum, and index XYZ coords
    size_t row_id = 0;
    while(gzgets(fp, line, LINE_SIZE) != Z_NULL) {
        char* tmp = line;
        int col;
        char* t;
        float* row = &data->data[row_id * data->cols];
        for(col=0, t=strtok(tmp,"\t");
            t && *t;
            col++, t = strtok(NULL, "\t\n")) {
            float f = atof(t);
            row[col] = f;
            if (f != 0.0) {
                data->notzero[col]++;
                data->sum[col]+= f;
            }
            if (data->min[col] > f) {
                data->min[col] = f;
                data->min_id[col] = row_id;
            }
            if (data->max[col] < f) {
                data->max[col] = f;
                data->max_id[col] = row_id;
            }
            if(col>=data->cols) break;
        }
        // Index first 3 columns (XYZ) in spatial tree for picking and nearest-neighbor search
        kd_insert3f(data->index, row[0], row[1], row[2], (void*)row_id);
        row_id++;
    }

    // Aggregate per-cluster statistics (member counts, category sums)
    parse_clusters(data);
    
    printf("loaded: %ld rows\n", row_id);
    printf("data->cols: %d\n", data->cols);
    printf("data->rows: %d\n", data->rows);

    gzclose(fp); // Закрываем сжатый файл

    sprintf(filename, "%s.messages.gz", prefix);
    data->messages = load_messages_from_gz(filename, &data->messages_count);
    printf("Loaded %d messages\n", data->messages_count);

    sprintf(filename, "%s.labels.txt", prefix);
    data->labels = labels_load(filename);

    // Initialize dynamic payload array with normalized row indices
    // for(int i=0; i<data->rows; i++) {
    //     data->dynamic[i] = (float)i/data->rows;
    // }
    data->min[data->cols] = 0.0;
    data->max[data->cols] = 1.0;

    return data;
}


int data_add_label(const char* label, float x, float y, float z) {
    return labels_add(data->labels, x, y, z, label);
}

static int clusters_comp(const void* a, const void* b) {
    cluster_stat_t* c1 = a;
    cluster_stat_t* c2 = b;
    return c2->sum - c1->sum;
}


static int cat_stat_comp(const void* a, const void* b) {
    cat_stat_t* c1 = a;
    cat_stat_t* c2 = b;
    return c2->sum - c1->sum;
}


static void parse_clusters(data_t* data) {
    // search default cluster column if need
    if (app_ctx.cluster_col<0) {
        for(int col=0; col < data->cols; col++) {
            if(strcmp(data->header[col], "clust"  ) == 0 || 
               strcmp(data->header[col], "кластер") == 0 ) {
                app_ctx.cluster_col = col;
                break;
            }
        }
    }

    // cluster col not set
    if(app_ctx.cluster_col < 0) return;
    printf("app_ctx.cluster_col found: %d\n", app_ctx.cluster_col);
    
    // min / max values of cluster id
    int min = (int)data->min[app_ctx.cluster_col];
    int max = (int)data->max[app_ctx.cluster_col];
    printf("Cluster ids %d - %d\n", min, max);
    
    // init clusters stat sturcts 
    data->clusters_cnt = max - min + 1;
    data->clusters = calloc(data->clusters_cnt, sizeof(cluster_stat_t));
    for(int cls=0; cls<data->clusters_cnt; cls++) { 
        data->clusters[cls].cat_sum = calloc(data->cols, sizeof(cat_stat_t));
        // init col ids
        for(int col=0; col < data->cols; col++) {
            data->clusters[cls].cat_sum[col].id = col;
        }
    }

    // aggregate clusters stats
    for(int row = 0; row < data->rows; row++) {
        int cluster = (int) (data->data[row*data->cols + app_ctx.cluster_col]);
        int cid = cluster - min; // min = -1 by deafult
        // count members of cluster
        data->clusters[cid].cnt++;
        data->clusters[cid].id = cluster; 
        for(int col = 0; col < data->cols; col++) {
            // get category float
            float f = data->data[row*data->cols + col];
            if(f>0.0) {
                // integrate per category sum
                data->clusters[cid].cat_sum[col].sum += f;
                // integrate only categories cols
                if (col >= app_ctx.categories_start)
                    data->clusters[cid].sum += f;
            }
        }
    }

    // sort clusters by total sum
    qsort(data->clusters, 
        data->clusters_cnt, 
        sizeof(cluster_stat_t), 
        clusters_comp);
    // sort cluster stat by category sum
    for(int cls=0; cls<data->clusters_cnt; cls++) { 
        qsort(data->clusters[cls].cat_sum, 
            data->cols, 
            sizeof(cat_stat_t), 
            cat_stat_comp); 
    }
}

static void parse_header(data_t* data, char* header) {
    char* s = strdup(header);
    char* t; // token
    char* n; // tail

    // names cache
    char* names[MAX_COLS];
    // Check if first name is quoted
    t = (s[0] == '"') ? strtok_r(s, "\"", &n) : strtok_r(s, "\t\n", &n);
    while (t && *t) {
        // trim leading spaces
        while (t[0] == ' ') t++;
        names[data->cols] = strdup(t);
        data->cols++;
        if (data->cols >= MAX_COLS) break;

        // skip delimiters
        while (n[0] == ' ' || n[0] == ',') n++;
        if (n[0] == 0) break;
        // check if next name is quoted
        t = (n[0] == '"') ? strtok_r(NULL, "\"", &n) : strtok_r(NULL, "\t\n", &n);
    }

    // Allocate memory for (data->cols + 1) pointers
    size_t l = sizeof(char*) * (data->cols + 1);
    data->header = malloc(l);
    if (!data->header) {
        free(s);
        return;
    }

    // Copy original column names
    memcpy(data->header, names, sizeof(char*) * data->cols);

    // Add "dynamic" as additional header (doesn't increase data->cols)
    data->header[data->cols] = strdup("dynamic");

    free(s);
}


// Load and parse gzipped file into array of strings
// Returns array of message strings (char**), must be freed with free_messages
char** load_messages_from_gz(const char* filename, int* num_lines) {
    gzFile file = gzopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open %s\n", filename);
        return NULL;
    }

    // Read entire content into buffer
    const size_t chunk_size = 8192;
    char* buffer = NULL;
    size_t buffer_size = 0;
    size_t total_read = 0;

    while (1) {
        buffer = realloc(buffer, buffer_size + chunk_size);
        if (!buffer) {
            fprintf(stderr, "Error: Out of memory while reading %s\n", filename);
            gzclose(file);
            return NULL;
        }

        int bytes_read = gzread(file, buffer + total_read, chunk_size);
        if (bytes_read < 0) {
            fprintf(stderr, "Error: Failed to read from %s\n", filename);
            gzclose(file);
            free(buffer);
            return NULL;
        }

        if (bytes_read == 0) {
            break; // End of file
        }

        total_read += bytes_read;
        buffer_size += chunk_size;
    }

    gzclose(file);

    // Ensure buffer is null-terminated
    buffer = realloc(buffer, total_read + 1);
    buffer[total_read] = '\0';

    // Count total lines
    int line_count = 0;
    for (size_t i = 0; i < total_read; i++) {
        if (buffer[i] == '\n') {
            line_count++;
        }
    }
    // If last character is not newline, count it as additional line
    if (total_read > 0 && buffer[total_read - 1] != '\n') {
        line_count++;
    }

    // Allocate array of pointers (+1 for NULL terminator)
    char** lines = malloc((line_count + 1) * sizeof(char*));
    if (!lines) {
        fprintf(stderr, "Error: Out of memory for line pointers\n");
        free(buffer);
        return NULL;
    }

    // Fill array with pointers to line starts
    int current_line = 0;
    lines[current_line] = buffer; // First line starts at buffer beginning

    for (size_t i = 0; i < total_read; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0'; // Replace newline with null terminator
            current_line++;
            if (current_line < line_count) {
                lines[current_line] = buffer + i + 1; // Следующая строка начинается после \0
            }
        }
    }
    lines[line_count] = NULL; // Завершаем массив NULL

    *num_lines = line_count;
    return lines;
}

/**
 * @brief Освобождает память, выделенную load_messages_from_gz.
 * @param messages Массив строк, возвращённый load_messages_from_gz.
 */
void free_messages(char** messages) {
    if (messages) {
        free(messages[0]); // Буфер с содержимым
        free(messages);    // Массив указателей
    }
}

#ifdef TEST_CSV
int main() {
    data_t* data = data_load("data.basket.data");
    
    printf("cols: %zu\n", data->cols);
    printf("rows: %zu\n", data->rows);

    for(int i=0; i< data->cols; i++) {
        printf("%s min: %0.0f %0.0f\n", data->header[i], data->min[i], data->max[i]);
    }
    /*
    for(int j=0; j< data->rows; j++) {
        for(int i=0; i< data->cols; i++) {
            if(data->data[data->cols*j+i] >0.0)
            printf("%s: %0.0f ", data->header[i], data->data[data->cols*j+i]);
        }
        printf("\n");
    }
    */
    data_free(data); 
    return 0;
}
#endif
