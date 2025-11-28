set(PROJECT_NAME point_cloud)

Project(${PROJECT_NAME})
cmake_minimum_required(VERSION 3.11)

# # GLFW
# find_package(PkgConfig REQUIRED)
# pkg_search_module(GLFW REQUIRED glfw3)

# set (CMAKE_CXX_STANDARD 11)

# #CIMGUI
# set(cimgui lib/cimgui)
# set(imgui_impl ${cimgui}/imgui/examples)

# set(gl3w lib/gl3w)
# set(klib lib/klib)
# set(linmath lib/linmath/)
# set(cglm lib/cglm)

# # library search path
# set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig")
# set(ENV{LD_LIBRARY_PATH} "ENV{LD_LIBRARY_PATH}:/usr/local/lib")

# # build dir
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -g")

# # Include dirs
# include_directories(
# 	src 
# 	${cimgui} 
# 	${cimgui}/imgui ${imgui_impl}
#     ${gl3w}
#     ${klib}
# 	${cglm}/include
#     ${linmath})

# file(GLOB source_files "src/*.c")
# file(GLOB gl3w_files "${gl3w}/GL/*.c")

# # cimgui compilation
# file(GLOB 
# 	cimgui_files 
# 	"${cimgui}/*.cpp" 
# 	"${cimgui}/imgui/*.cpp"
# 	"${imgui_impl}/imgui_impl_glfw.cpp"
# 	"${imgui_impl}/imgui_impl_opengl3.cpp")
# add_library(cimgui ${cimgui_files})
# if (WIN32)
# 	target_link_libraries(cimgui ${GLFW_LIBRARIES})
# endif (WIN32)

# target_compile_definitions(cimgui PRIVATE 
# 	IMGUI_IMPL_API=extern\ \"C\"
# 	IMGUI_IMPL_OPENGL_LOADER_GL3W)

cmake_minimum_required(VERSION 3.11)
if(WIN32) # to mingw work as all the others
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif(WIN32)

#run in build dir
set (CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})

set (CMAKE_CXX_STANDARD 11)

# build dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# general settings
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/cimgui/imgui/backends)
	set(BAKENDS_FOLDER "lib/cimgui/imgui/backends/")
else()
	set(BAKENDS_FOLDER "lib/cimgui/imgui/examples/")
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/cimgui/imgui/imgui_tables.cpp)
	set(TABLES_SOURCE "lib/cimgui/imgui/imgui_tables.cpp")
else()
	set(TABLES_SOURCE "")
endif()

include_directories(lib/cimgui/imgui)
add_definitions("-DIMGUI_USER_CONFIG=\"${CMAKE_CURRENT_SOURCE_DIR}/lib/cimgui/cimconfig.h\"")
add_definitions("-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1")

include_directories(lib/cimgui/)
set(IMGUI_SOURCES 
lib/cimgui/cimgui.cpp 
lib/cimgui/imgui/imgui.cpp 
lib/cimgui/imgui/imgui_draw.cpp 
lib/cimgui/imgui/imgui_demo.cpp 
lib/cimgui/imgui/imgui_widgets.cpp
${TABLES_SOURCE} 
)

set(IMGUI_LIBRARIES )


add_compile_definitions("IMGUI_IMPL_OPENGL_LOADER_GL3W")

# optional adding freetype
option(IMGUI_FREETYPE "add Freetype2" OFF)

if(IMGUI_FREETYPE)
	message("building with freetype")
	FIND_PACKAGE(freetype REQUIRED PATHS ${FREETYPE_PATH})
	list(APPEND IMGUI_LIBRARIES freetype)
	list(APPEND IMGUI_SOURCES lib/cimgui/imgui/misc/freetype/imgui_freetype.cpp)
	#add_definitions("-DCIMGUI_FREETYPE=1")
	add_definitions("-DIMGUI_ENABLE_FREETYPE=1")
	add_definitions("-DIMGUI_ENABLE_STB_TRUETYPE=1")
endif(IMGUI_FREETYPE)

# opengl3
list(APPEND IMGUI_SOURCES ${BAKENDS_FOLDER}imgui_impl_opengl3.cpp)
include_directories(lib/cimgui/imgui/examples/libs/gl3w)
if(WIN32)
       list(APPEND IMGUI_LIBRARIES opengl32)
else(WIN32) # Unix
       list(APPEND IMGUI_LIBRARIES GL)
endif(WIN32)

# GLFW
list(APPEND IMGUI_SOURCES ${BAKENDS_FOLDER}imgui_impl_glfw.cpp)

set(GLFW_VERSION 3.4)
include(FetchContent)
	FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/${GLFW_VERSION}.tar.gz)
	#GIT_REPOSITORY https://github.com/glfw/glfw )

FetchContent_GetProperties(glfw)
if (NOT glfw_POPULATED)
	set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(glfw)
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	if (NOT STATIC_BUILD)
	set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
	endif()
	add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

#if dynamic glfw then install
install(TARGETS glfw RUNTIME DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
					LIBRARY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
#FIND_PACKAGE(glfw3 PATHS "C:/LuaGL/gitsources/BUILDS/GLFW/install" REQUIRED)


if (NOT STATIC_BUILD)
  add_library(cimgui SHARED ${IMGUI_SOURCES})
else()
  add_library(cimgui STATIC ${IMGUI_SOURCES})
endif()

if (WIN32)
    target_compile_definitions(cimgui PUBLIC "-DIMGUI_IMPL_API=extern \"C\" __declspec\(dllexport\)")
else(WIN32)
    target_compile_definitions(cimgui PUBLIC "-DIMGUI_IMPL_API=extern \"C\" ")
endif(WIN32)

target_link_libraries(cimgui ${IMGUI_LIBRARIES} glfw)

# ---------------------------------------

# add_executable(
# 	${project_name} 
# 	${source_files}
# 	${gl3w_files})

# target_link_libraries(
# 	${project_name}
# 	${GLFW_LIBRARIES}
# 	m GL dl cimgui z)	

# Указать папку с исходниками ./src
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm/include
	${CMAKE_CURRENT_SOURCE_DIR}/lib/klib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/kdtree
	${CMAKE_CURRENT_SOURCE_DIR}/lib/gl3w
)

set(SOURCES 
	src/main.c
	src/bbgl.c
	src/data.c
	src/globals.c
	src/gui.c
	src/obj.c
	src/interactive.c
	src/shader.c
	src/scene.c
	src/labels.c
	lib/gl3w/GL/gl3w.c
	lib/kdtree/kdtree.c
)

# using library
add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_definitions(${PROJECT_NAME}
	PUBLIC 
	-DCIMGUI_USE_OPENGL3 
	-DCIMGUI_USE_GLFW)
target_link_libraries(${PROJECT_NAME}
	cimgui GL dl m z pthread)

