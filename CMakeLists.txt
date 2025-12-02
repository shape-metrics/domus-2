cmake_minimum_required(VERSION 3.16)
project(DOMUS VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)

#======================================
# Detect Emscripten
#======================================
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS ">>> Building DOMUS + KISSAT for WebAssembly")

    set(BUILDING_WASM TRUE)
    # JS output (main.js + main.wasm)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")

    # compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

else()
    set(BUILDING_WASM FALSE)
endif()

#====================================================
# Kissat source files
#====================================================

file(GLOB KISSAT_SRC
    ${CMAKE_SOURCE_DIR}/src/sat/kissat/src/*.c
)

add_library(kissat STATIC ${KISSAT_SRC})
target_include_directories(kissat PUBLIC
    ${CMAKE_SOURCE_DIR}/src/sat/kissat/src
)
target_compile_definitions(kissat PRIVATE NDEBUG QUIET)

#======================================
# Common core sources
#======================================

set(COMMON_SRCS
    src/sat/kissat.cpp
    src/sat/cnf.cpp
    src/orthogonal/shape/shape.cpp
    src/orthogonal/shape/shape_builder.cpp
    src/orthogonal/shape/variables_handler.cpp
    src/orthogonal/shape/clauses_functions.cpp
    src/orthogonal/area_compacter.cpp
    src/orthogonal/equivalence_classes.cpp
    src/drawing/polygon.cpp
    src/core/graph/graphs_algorithms.cpp
    src/core/graph/graph.cpp
    src/core/graph/cycle.cpp
    src/core/graph/attributes.cpp
    src/core/graph/file_loader.cpp
    src/core/tree/tree.cpp
    src/core/tree/tree_algorithms.cpp
    src/core/graph/segment.cpp
    src/drawing/svg_drawer.cpp
    src/orthogonal/drawing_builder.cpp
    src/orthogonal/drawing_stats.cpp
    src/config/config.cpp
    src/core/graph/generators.cpp
    src/orthogonal/file_loader.cpp
    src/core/utils.cpp
    src/core/csv.cpp
    src/planarity/auslander_parter.cpp
    src/planarity/embedding.cpp
    src/planarity/interlacement.cpp
    src/core/graph/palm_tree.cpp
)

add_library(core STATIC ${COMMON_SRCS})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)

#======================================
# Warning flags
#======================================

function(apply_warnings target)
    if (BUILDING_WASM)
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow -Wconversion
            -Wsign-conversion
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wconversion -Wsign-conversion
            -Wshadow -Wold-style-cast
            -Woverloaded-virtual
            -Wnon-virtual-dtor
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
            -Werror
        )
    endif()
endfunction()

apply_warnings(core)

#======================================
# Executables
#======================================

if (BUILDING_WASM)
    add_executable(domus src/domus.cpp)
    target_link_libraries(domus PRIVATE core kissat)
    apply_warnings(domus)
    target_link_options(domus PRIVATE
        "-sEXPORTED_FUNCTIONS=['_compute_orthogonal_drawing']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap', 'FS', 'UTF8ToString']"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sWASM=1"
        "--preload-file" "${CMAKE_SOURCE_DIR}/example-graphs@/example-graphs"
    )
else()
    add_executable(main src/main.cpp)
    add_executable(stats src/stats.cpp)
    add_executable(gen src/gen.cpp)

    foreach(target main stats gen)
        target_link_libraries(${target} PRIVATE core kissat)
        apply_warnings(${target})
    endforeach()
endif()
