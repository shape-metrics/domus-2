#ifndef MY_GRAPH_FILE_LOADER_H
#define MY_GRAPH_FILE_LOADER_H

#include <memory>
#include <string>

#include "core/graph/graph.hpp"

std::unique_ptr<UndirectedSimpleGraph> load_graph_from_txt_file(
    const std::string &filename);

void load_graph_from_txt_file(const std::string &filename,
                              UndirectedSimpleGraph &graph);

void save_graph_to_file(const UndirectedSimpleGraph &graph,
                        const std::string &filename);

#endif