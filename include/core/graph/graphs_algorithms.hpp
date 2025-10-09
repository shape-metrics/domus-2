#ifndef MY_GRAPHS_ALGORITHMS_H
#define MY_GRAPHS_ALGORITHMS_H

#include <optional>
#include <vector>

#include "core/graph/cycle.hpp"
#include "core/graph/graph.hpp"

bool is_graph_connected(const UndirectedSimpleGraph& graph);

std::vector<Cycle>
compute_all_cycles_with_node_in_graph(const UndirectedSimpleGraph& graph,
                                      const GraphNode& node,
                                      const std::unordered_set<int>& taboo_nodes);

std::vector<Cycle> compute_all_cycles_in_graph(const UndirectedSimpleGraph& graph);

std::optional<Cycle> find_a_cycle_in_graph(const UndirectedSimpleGraph& graph);

std::optional<Cycle> find_a_cycle_in_graph(const DirectedSimpleGraph& graph);

std::vector<Cycle> compute_cycle_basis(const UndirectedSimpleGraph& graph);

std::vector<int> make_topological_ordering(const DirectedSimpleGraph& graph);

bool are_cycles_equivalent(const Cycle& cycle1, const Cycle& cycle2);

std::vector<std::unique_ptr<UndirectedSimpleGraph>>
compute_connected_components(const UndirectedSimpleGraph& graph);

size_t compute_number_of_connected_components(const UndirectedSimpleGraph& graph);

class BiconnectedComponents {
    std::unordered_set<int> m_cutvertices;
    std::vector<std::unique_ptr<UndirectedSimpleGraph>> m_components;

  public:
    std::unordered_set<int>& get_cutvertices();
    const std::unordered_set<int>& get_cutvertices() const;
    const std::vector<std::unique_ptr<UndirectedSimpleGraph>>& get_components() const;
    std::string to_string() const;
    void print() const;
    BiconnectedComponents(std::unordered_set<int>&& cutvertices,
                          std::vector<std::unique_ptr<UndirectedSimpleGraph>>&& components);
};

BiconnectedComponents compute_biconnected_components(const UndirectedSimpleGraph& graph);

std::pair<std::unique_ptr<UndirectedSimpleGraph>, GraphEdgeHashSet>
compute_maximal_degree_4_subgraph(const UndirectedSimpleGraph& graph);

std::optional<std::unordered_map<int, bool>>
compute_bipartition(const UndirectedSimpleGraph& graph);

#endif