#ifndef MY_INTERLACEMENT_GRAPH_HPP
#define MY_INTERLACEMENT_GRAPH_HPP

#include <memory>

#include "core/graph/cycle.hpp"
#include "core/graph/graph.hpp"
#include "core/graph/segment.hpp"

std::unique_ptr<UndirectedSimpleGraph> compute_interlacement_graph(
    const std::vector<Segment> &segments, const Cycle &cycle);

#endif