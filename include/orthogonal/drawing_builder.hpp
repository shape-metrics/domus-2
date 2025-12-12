#ifndef MY_DRAWING_BUILDER_H
#define MY_DRAWING_BUILDER_H

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "core/graph/attributes.hpp"
#include "core/graph/graph.hpp"
#include "orthogonal/shape/shape.hpp"

class DisconnectedGraphError : public std::runtime_error {
  public:
    DisconnectedGraphError() : std::runtime_error("The graph is not connected.") {}
};

void make_svg(const UndirectedSimpleGraph& graph,
              const GraphAttributes& attributes,
              const std::string& filename);

struct DrawingResult {
    std::unique_ptr<UndirectedSimpleGraph> augmented_graph;
    GraphAttributes attributes;
    Shape shape;
    size_t initial_number_of_cycles;
    size_t number_of_added_cycles;
    size_t number_of_useless_bends;
};

DrawingResult make_orthogonal_drawing(const UndirectedSimpleGraph& graph);

std::pair<std::unordered_map<int, int>, std::unordered_map<int, int>>
compute_node_to_index_position(const UndirectedSimpleGraph& graph,
                               const GraphAttributes& attributes);

#endif
