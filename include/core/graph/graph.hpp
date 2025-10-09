#ifndef MY_GRAPH_H
#define MY_GRAPH_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/utils.hpp"

using GraphEdgeHashSet = std::unordered_set<std::pair<int, int>, int_pair_hash>;

template <typename T>
using GraphEdgeHashMap = std::unordered_map<std::pair<int, int>, T, int_pair_hash>;

class GraphNode;
class Graph;

struct GraphEdge {
  private:
    int m_id;
    int m_from_id;
    int m_to_id;

  public:
    GraphEdge(int id, const GraphNode& from, const GraphNode& to);
    GraphEdge(int id, int from_id, int to_id);
    [[nodiscard]] int get_id() const;
    [[nodiscard]] int get_from_id() const;
    [[nodiscard]] int get_to_id() const;
    [[nodiscard]] std::string to_string() const;
    void print() const;
    [[nodiscard]] GraphEdge reversed() const;
    void reverse();
};

class Graph {
    int m_next_node_id = 0;
    int m_next_edge_id = 0;
    size_t m_total_edges = 0;
    std::unordered_map<int, std::unique_ptr<GraphNode>> m_nodeid_to_node_map;
    std::unordered_map<int, std::unique_ptr<GraphEdge>> m_edgeid_to_edge_map;
    std::unordered_map<int, std::unordered_set<int>> m_nodeid_to_incident_edgeids;
    GraphEdgeHashMap<std::unordered_set<int>> m_edge_to_edgeids;

  protected:
    size_t get_edge_count(int from_id, int to_id) const;
    const std::unordered_set<int>& get_edgeids(int from_id, int to_id) const;

  public:
    virtual ~Graph() = default;
    bool has_node(int node_id) const;
    const GraphNode& get_node_by_id(int id) const;
    std::vector<const GraphNode*> get_nodes() const;
    std::vector<int> get_nodes_ids() const;
    virtual std::vector<GraphEdge> get_edges_of_node(int node_id) const;
    virtual const GraphNode& add_node(int id);
    const GraphNode& add_node();
    virtual size_t get_degree_of_node(int node_id) const = 0;
    virtual int add_edge(int from_id, int to_id);
    virtual bool has_edge(int from_id, int to_id) const = 0;
    std::vector<GraphEdge> get_edges() const;
    size_t size() const;
    size_t get_number_of_edges() const;
    const GraphEdge& get_edge_by_id(int edge_id) const;
    virtual void remove_node(int node_id);
    virtual void remove_edge(int edge_id);
    std::string to_string() const;
    void print() const;
};

class DirectedMultiGraph : public Graph {
    std::unordered_map<int, std::unordered_set<int>> m_nodeid_to_incoming_edgeids;
    std::unordered_map<int, std::unordered_set<int>> m_nodeid_to_outgoing_edgeids;

  public:
    DirectedMultiGraph() = default;
    const GraphNode& add_node(int node_id) override;
    int add_edge(int from_id, int to_id) override;
    std::vector<GraphEdge> get_edges_of_node(int node_id) const override;
    std::vector<GraphEdge> get_in_edges_of_node(int node_id) const;
    size_t get_degree_of_node(int node_id) const override;
    size_t get_in_degree_of_node(int node_id) const;
    void remove_node(int node_id) override;
    void remove_edge(int edge_id) override;
    bool has_edge(int from_id, int to_id) const override;
    // delete move constructor and move assignment operator
    DirectedMultiGraph(const DirectedMultiGraph&) = delete;
    DirectedMultiGraph& operator=(const DirectedMultiGraph&) = delete;
    // delete copy constructor and copy assignment operator
    DirectedMultiGraph(DirectedMultiGraph&&) = delete;
    DirectedMultiGraph& operator=(DirectedMultiGraph&&) = delete;
};

class DirectedSimpleGraph final : public DirectedMultiGraph {
  public:
    DirectedSimpleGraph() = default;
    int add_edge(int from_id, int to_id) override;
    int get_edge_id(int from_id, int to_id) const;
    GraphEdge get_edge(int from_id, int to_id) const;
    void remove_edge(int edge_id) override;
    void remove_edge(int from_id, int to_id);
    // delete move constructor and move assignment operator
    DirectedSimpleGraph(const DirectedSimpleGraph&) = delete;
    DirectedSimpleGraph& operator=(const DirectedSimpleGraph&) = delete;
    // delete copy constructor and copy assignment operator
    DirectedSimpleGraph(DirectedSimpleGraph&&) = delete;
    DirectedSimpleGraph& operator=(DirectedSimpleGraph&&) = delete;
};

class UndirectedMultiGraph : public Graph {
  public:
    UndirectedMultiGraph() = default;
    size_t get_degree_of_node(int node_id) const override;
    bool has_edge(int from_id, int to_id) const override;
    std::vector<GraphEdge> get_edges_of_node(int node_id) const override;
    // delete move constructor and move assignment operator
    UndirectedMultiGraph(const DirectedMultiGraph&) = delete;
    UndirectedMultiGraph& operator=(const UndirectedMultiGraph&) = delete;
    // delete copy constructor and copy assignment operator
    UndirectedMultiGraph(UndirectedMultiGraph&&) = delete;
    DirectedMultiGraph& operator=(DirectedMultiGraph&&) = delete;
};

class UndirectedSimpleGraph final : public UndirectedMultiGraph {
  public:
    UndirectedSimpleGraph() = default;
    int add_edge(int from_id, int to_id) override;
    int get_edge_id(int from_id, int to_id) const;
    void remove_edge(int edge_id) override;
    void remove_edge(int from_id, int to_id);
    // delete move constructor and move assignment operator
    UndirectedSimpleGraph(const UndirectedSimpleGraph&) = delete;
    UndirectedSimpleGraph& operator=(const UndirectedSimpleGraph&) = delete;
    // delete copy constructor and copy assignment operator
    UndirectedSimpleGraph(UndirectedSimpleGraph&&) = delete;
    UndirectedSimpleGraph& operator=(UndirectedSimpleGraph&&) = delete;
};

class GraphNode {
    int m_id;
    const Graph& m_graph_owner;

  public:
    GraphNode(int id, const Graph& owner);
    [[nodiscard]] int get_id() const;
    [[nodiscard]] std::vector<GraphEdge> get_edges() const {
        return m_graph_owner.get_edges_of_node(m_id);
    }
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] size_t get_degree() const;
    void print() const;
    [[nodiscard]] const Graph& get_owner() const;
};

#endif
