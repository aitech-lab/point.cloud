#include <stdio.h>
#include <stdlib.h>
#include <ketopt.h>

#include "globals.h"
#include "bbgl.h"
#include "data.h"

data_p data;
int usage() {
    printf("Uasge:\n");
    printf("\t./point_cloud [--cluster_col=col_id] [--categories_start=col_id] [csv_file.csv]\n");
    printf("\n");

    printf("Params:\n");
    printf("\t--cluster_col - id of cluster column\n"); 
    printf("\t--categories_start - first category col id\n");
    printf("\n");

    printf("csv file format:\n");
    printf("\t x, y, z, u, [clust], [product 1], ..., [product n]\n\n");
    printf("First 4 columns of csv file, x y z u, will we used as point coordinates\n");
    printf("All cols, except the header, must be convertible to float\n");
    return 0;
}

// Entry point: parse args, load data, initialize OpenGL, run main loop
int main(int argc, char** argv) {

    static ko_longopt_t longopts[] = {
        { "help",             ko_no_argument,       301 },
        { "cluster_col",      ko_required_argument, 302 },
        { "categories_start", ko_required_argument, 303 },
        { NULL, 0, 0 }
    };

    cluster_col = -1;
    categories_start = 6;

    // Parse command-line arguments
    ketopt_t opt = KETOPT_INIT;
    int c;
    while ((c = ketopt(&opt, argc, argv, 1, "h", longopts)) >= 0) {
        if (c == 301 || c == 'h') return usage();
        else if (c == 302) cluster_col      = opt.arg ? atoi(opt.arg) :-1;
        else if (c == 303) categories_start = opt.arg ? atoi(opt.arg) : 0;
    }
    char* datafile =  (opt.ind < argc) ? argv[opt.ind] : "data.tsv.gz";

    printf("datafile: %s\n"        , datafile);
    printf("cluster_col: %d\n"     , cluster_col);
    printf("categories_start: %d\n", categories_start);

    // Load gzipped TSV dataset into flat array with spatial index
    data = data_load(datafile);

    // Initialize OpenGL context, shaders, and UI (ImGui)
    bbgl_init();
    // Main render loop: input → gui update → scene render → swap buffers
    bbgl_loop();

    // Cleanup
    data_free(data);
}
