//
// Created by giordy on 7/7/25.
//

#ifndef PALM_TREE_HPP
#define PALM_TREE_HPP

#include "core/graph/graph.hpp"
#include "core/tree/tree.hpp"

class PalmTree {
    std::unique_ptr<Tree> m_tree;
};

// assumes the graph is biconnected
PalmTree compute_palm_tree(const Graph& graph);

#endif // PALM_TREE_HPP
