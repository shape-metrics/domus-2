cmake_minimum_required(VERSION 3.16)
project(DOMUS VERSION 1.0 LANGUAGES CXX)

# Enable compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# External project for Kissat SAT solver
include(ExternalProject)

ExternalProject_Add(kissat_project
    PREFIX ${CMAKE_BINARY_DIR}/kissat
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/sat/kissat
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make -C ${CMAKE_SOURCE_DIR}/src/sat/kissat
    INSTALL_COMMAND ""
)

# Common source files
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

# Core library
add_library(core STATIC ${COMMON_SRCS})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Tell CMake about the library file produced by the Makefile
add_library(kissat STATIC IMPORTED)
set_target_properties(kissat PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/src/sat/kissat/build/libkissat.a
)
add_dependencies(kissat kissat_project)

# Include headers
target_include_directories(kissat INTERFACE
    ${CMAKE_SOURCE_DIR}/src/sat/kissat/src
)

# Optimization flags for core
target_compile_options(core PRIVATE)

# Strict warnings setup
if (MSVC)
    target_compile_options(core PRIVATE /W4 /WX)
else ()
    target_compile_options(core PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wold-style-cast
            -Woverloaded-virtual
            -Wnon-virtual-dtor
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
            -Werror
    )
endif ()

# Executables
add_executable(main src/main.cpp)
add_executable(stats src/stats.cpp)
add_executable(gen src/gen.cpp)

# Link core library
target_link_libraries(main PRIVATE core kissat)
target_link_libraries(stats PRIVATE core kissat)
target_link_libraries(gen PRIVATE core kissat)

# Also apply the same warnings to the executables
foreach (target main stats gen)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${target} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
                -Wold-style-cast
                -Woverloaded-virtual
                -Wnon-virtual-dtor
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wnull-dereference
                -Werror
        )
    endif ()
endforeach ()
