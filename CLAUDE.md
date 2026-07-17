# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A real-time OpenGL 3.3 point-cloud viewer for large dimensionality-reduction outputs (UMAP, t-SNE). Each point is a row projected to 3D; points can be colored by any data column, filtered by value range, selected via GPU picking, and clustered. It is a single C executable (`point_cloud`) using cimgui (Dear ImGui C bindings) for the UI and cglm for math.

## Build & run

CMake is configured **in-source** — `CMAKE_BINARY_DIR` is the repo root, so build from there:

```bash
./pull_submodules.sh        # first time only: fetch cimgui, cglm, klib submodules
cmake .                     # generate build system (FetchContent downloads GLFW 3.4)
make point_cloud            # build the executable; see note on parallelism below
```

The executable lands at `bin/point_cloud` (196 KB) and must be **run from `bin/`** because it loads `./font.ttf`, `./shaders/`, and data files relative to CWD:

```bash
cd bin
./point_cloud [--cluster_col=N] [--categories_start=N] <dataset-prefix>
./point_cloud --help                  # prints usage
```

Build notes:
- CMake emits deprecation warnings (FetchContent_Populate, CMP0135) but build succeeds; suppress with `-Wno-dev` if annoying.
- Compiler warnings (discarded `const`, pointer-to-int casts) are harmless; source has no errors.
- Parallel build recommended: `make -j4 point_cloud` (compiles imgui + 8 source files concurrently; ~5–10 sec total).
- After editing source code: just run `make point_cloud` again; cmake auto-detects changes.
- After editing `CMakeLists.txt`: run `cmake .` first, then `make point_cloud`.

`<dataset-prefix>` is a path prefix, not a filename. `data_load` appends the suffixes: `<prefix>.floats.tsv.gz` (point data), `<prefix>.messages.gz` (per-point text), `<prefix>.labels.txt` (saved labels). Sample datasets in `bin/data/`:
- `frida.500k.floats.tsv.gz` + `.messages.gz` (500k points; the main demo)
- `frida.500k.unsup.floats.tsv.gz` + variants (unsupervised clustering)
- `al.ru.semisup.floats.tsv.gz` + `.messages.gz` (semi-supervised; smaller)

Example: `./point_cloud data/frida.500k` (run from `bin/`).

There is no test suite in the build. `src/test.c`, `src/test_scene.c`, and `src/main.new.c` are scratch/experimental files excluded from `CMakeLists.txt` — do not assume they compile or are wired in.

## Data format

Input is a gzipped TSV with a header row. The viewer expects at least `x, y, z, [clust], [categories...]` — first three columns are 3D coordinates; cluster and category columns are optional. Every non-header cell must parse as a float. 

`--cluster_col` marks the cluster-id column; auto-detected if a column is named `clust` or `кластер`. `--categories_start` (default 6) is the first category column, used for per-cluster statistics.

Example dataset (`frida.500k`) has columns: `x, y, z, cluster, group` — 5 columns total, no separate categories.

## Architecture

The codebase is small (~10 `src/*.c` files) and coordinates almost entirely through **mutable global state**, not message passing. Understand this before changing anything:

- **`globals.h` / `globals.c`** — declares all the `gui_*` camera and render parameters (radius, rotation, translation, color column, min/max filter, alpha, point size) plus picking/search flags (`render_id`, `picked_id`, `do_search_nearest`, `do_search_cluster`, `dynamic_data_updated`) as `extern` globals. This is the shared bus: the GUI writes them, `scene_render` reads them and forwards them to the shader as uniforms. There is no state object threaded through calls.

- **`main.c`** — parses args (ketopt from klib), calls `data_load`, then `bbgl_init` + `bbgl_loop`.

- **`bbgl.c`** — window/GL/GLFW bootstrap and the frame loop: `gui_update(scene)` → `scene_render(scene)` (point cloud, alpha-blended) → `gui_render()`. Owns the single global `scene`.

- **`data.c` / `data.h`** — loads the gz TSV into a flat `float* data` array (`rows × cols`), computes per-column min/max/sum/notzero, builds a 3D k-d tree spatial index (`lib/kdtree`) over point positions, aggregates `cluster_stat_t` (per-cluster member counts and per-category sums, sorted by total), and loads messages + labels. The global `data` pointer is defined here-adjacent (`main.c`) and used throughout.

- **`scene.c` / `obj.c`** — `scene_render` computes the orbit camera (spherical coords around a target it eases toward) and view/projection with cglm, sets shader uniforms, and draws. `obj_cloud` uploads the whole `data->data` array into one static VBO; attribute 0 reads the first 4 floats of each row (stride = `cols * sizeof(float)`, starting at offset 0), and the shader uses only `.xyz` as position; attribute 1 is the *coloring column* selected by `gui_col_id` (re-bound via `init_vao` only when the column changes). A separate `vbo_dynamic` holds per-point scalar payload updated on `dynamic_data_updated`.

- **GPU picking** — clicking sets `render_id`; on the next frame the shader (`bin/shaders/simple.vert`) encodes `gl_VertexID` into the fragment color instead of the normal HSV ramp, and `scene_render` calls `glReadPixels` at the mouse to decode `picked_id`. The camera then eases its target to the picked point. This is why the shader has two rendering modes keyed on `render_id`.

- **`interactive.c`** — GLFW input callbacks. Mouse drag orbits the camera, scroll zooms (radius), Ctrl/Shift+click set the nearest/same-cluster search flags. All callbacks early-out when ImGui wants the mouse (`io->WantCaptureMouse`).

- **`gui.c`** — the largest file; builds all ImGui windows (columns, clusters, search) and does the k-d tree nearest/same-category searches and label drawing. Coloring, filtering, and camera are all driven by writing the `gui_*` globals.

- **`curl.c`** — async HTTP via `pthread` + `system("curl ...")` (not libcurl). Used by the "Classify" button in `gui.c`, which POSTs selected cluster messages to **`http://localhost:5000/ask`** and shows the response. This requires an external local LLM service running on port 5000; it is optional and unrelated to the core viewer.

- **`shader.c`** — loads/compiles `bin/shaders/simple.{vert,frag}` at runtime (relative to CWD) and caches uniform locations in `shader_t`.

### Shaders

`bin/shaders/simple.vert` colors points with an HSV ramp over the normalized `[min,max]` range of the selected column (out-of-range points go dim gray), and switches to id-encoding mode when `render_id` is set. Editing coloring/filtering/picking behavior means editing this shader, not just C code.

## Dependencies & submodules

Vendored under `lib/`: `cimgui` (submodule, built as a shared/static lib by CMake, bundles ImGui + GLFW/OpenGL3 backends), `cglm` (math), `klib` (`ketopt`, `kvec`), `gl3w` (GL loader), `kdtree` (spatial index). GLFW 3.4 is pulled at configure time via CMake `FetchContent`. Runtime also links `z` (zlib, for gz data) and `pthread`. Set `-DSTATIC_BUILD=ON` to build GLFW/cimgui statically.

## Conventions

- Comments and user-facing strings are frequently in Russian; match the surrounding language when editing a file.
- `CMakeLists.txt` and `CMakeLists.old.txt` contain large commented-out blocks documenting an earlier build setup — the active configuration is the uncommented tail of `CMakeLists.txt`.
