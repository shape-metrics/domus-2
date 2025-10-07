#ifndef MY_EMBEDDER_H
#define MY_EMBEDDER_H

#include <optional>

#include "core/graph/graph.hpp"
#include "planarity/embedding.hpp"

std::optional<Embedding> embed_graph(const UndirectedSimpleGraph &graph);

#endif