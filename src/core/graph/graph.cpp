#include "core/graph/graph.hpp"

#include <iostream>
#include <ranges>

#include "core/utils.hpp"

GraphEdge::GraphEdge(const int id, const GraphNode& from, const GraphNode& to)
    : m_id(id), m_from_id(from.get_id()), m_to_id(to.get_id()) {
    if (&from.get_owner() != &to.get_owner())
        throw std::runtime_error("GraphEdge constructor: nodes do not belong to the same graph");
}

GraphEdge::GraphEdge(const int id, const int from_id, const int to_id)
    : m_id(id), m_from_id(from_id), m_to_id(to_id) {}

void GraphEdge::print() const { std::cout << to_string() << std::endl; }

std::string GraphEdge::to_string() const {
    return "Edge[" + std::to_string(m_from_id) + " -> " + std::to_string(m_to_id) + "]";
}

int GraphEdge::get_id() const { return m_id; }

int GraphEdge::get_from_id() const { return m_from_id; }

int GraphEdge::get_to_id() const { return m_to_id; }

GraphEdge GraphEdge::reversed() const { return {m_id, m_to_id, m_from_id}; }

void GraphEdge::reverse() {
    const int temp = m_from_id;
    m_from_id = m_to_id;
    m_to_id = temp;
}

const GraphNode& Graph::add_node(const int id) {
    if (id < 0)
        throw std::runtime_error("Graph::add_node: id must be non-negative");
    if (has_node(id))
        throw std::runtime_error("Graph::add_node: node with this id already exists");
    const auto node = new GraphNode(id, *this);
    m_nodeid_to_node_map[id] = std::unique_ptr<GraphNode>(node);
    m_nodeid_to_incident_edgeids[id] = {};
    return *node;
}

const GraphNode& Graph::get_node_by_id(const int id) const {
    if (!has_node(id))
        throw std::runtime_error("Graph::get_node_by_id: node not found");
    return *m_nodeid_to_node_map.at(id);
}

std::vector<const GraphNode*> Graph::get_nodes() const {
    std::vector<const GraphNode*> nodes;
    for (const auto& node : m_nodeid_to_node_map | std::views::values)
        nodes.push_back(node.get());
    return nodes;
}

std::vector<int> Graph::get_nodes_ids() const {
    std::vector<int> nodes_ids;
    for (const auto& node : m_nodeid_to_node_map | std::views::values)
        nodes_ids.push_back(node->get_id());
    return nodes_ids;
}

std::vector<GraphEdge> Graph::get_edges_of_node(const int node_id) const {
    std::vector<GraphEdge> edges;
    for (const int edge_id : m_nodeid_to_incident_edgeids.at(node_id))
        edges.push_back(get_edge_by_id(edge_id));
    return edges;
}

std::vector<GraphEdge> Graph::get_edges() const {
    std::vector<GraphEdge> edges;
    for (const auto& edge : m_edgeid_to_edge_map | std::views::values)
        edges.push_back(*edge);
    return edges;
}

const GraphNode& Graph::add_node() {
    while (has_node(m_next_node_id))
        m_next_node_id++;
    return add_node(m_next_node_id++);
}

int Graph::add_edge(const int from_id, const int to_id) {
    if (!has_node(from_id))
        throw std::runtime_error("Graph::add_edge: [from node] not found");
    if (!has_node(to_id))
        throw std::runtime_error("Graph::add_edge: [to node] not found");
    const GraphNode& from_node = get_node_by_id(from_id);
    const GraphNode& to_node = get_node_by_id(to_id);
    const int edge_id = m_next_edge_id++;
    m_edgeid_to_edge_map[edge_id] = std::make_unique<GraphEdge>(edge_id, from_node, to_node);
    m_nodeid_to_incident_edgeids[from_id].insert(edge_id);
    m_nodeid_to_incident_edgeids[to_id].insert(edge_id);
    m_total_edges++;
    m_edge_to_edgeids[{from_id, to_id}].insert(edge_id);
    return edge_id;
}

const GraphEdge& Graph::get_edge_by_id(const int edge_id) const {
    if (!m_edgeid_to_edge_map.contains(edge_id))
        throw std::runtime_error("Graph::get_edge: edge not found");
    return *m_edgeid_to_edge_map.at(edge_id);
}

void Graph::remove_edge(const int edge_id) {
    const GraphEdge& edge = get_edge_by_id(edge_id);
    m_nodeid_to_incident_edgeids[edge.get_from_id()].erase(edge_id);
    m_nodeid_to_incident_edgeids[edge.get_to_id()].erase(edge_id);
    m_edge_to_edgeids.at({edge.get_from_id(), edge.get_to_id()}).erase(edge.get_id());
    m_edgeid_to_edge_map.erase(edge_id);
    m_total_edges--;
}

void Graph::remove_node(const int node_id) {
    if (!has_node(node_id))
        throw std::runtime_error("Graph::remove_node: node not found");
    std::vector<int> edges_to_remove;
    for (const int edge_id : m_nodeid_to_incident_edgeids.at(node_id))
        edges_to_remove.push_back(edge_id);
    for (const int edge_id : edges_to_remove)
        remove_edge(edge_id);
    m_nodeid_to_node_map.erase(node_id);
    m_nodeid_to_incident_edgeids.erase(node_id);
}

size_t Graph::size() const { return m_nodeid_to_node_map.size(); }

size_t Graph::get_number_of_edges() const { return m_total_edges; }

size_t Graph::get_edge_count(const int from_id, const int to_id) const {
    if (!m_edge_to_edgeids.contains({from_id, to_id}))
        return 0;
    return m_edge_to_edgeids.at({from_id, to_id}).size();
}

const std::unordered_set<int>& Graph::get_edgeids(const int from_id, const int to_id) const {
    return m_edge_to_edgeids.at({from_id, to_id});
}

bool Graph::has_node(const int node_id) const { return m_nodeid_to_node_map.contains(node_id); }

std::string Graph::to_string() const {
    std::string result = "Graph:\n";
    for (const GraphNode* node : get_nodes())
        result += node->to_string() + "\n";
    return result;
}

void Graph::print() const { std::cout << to_string() << std::endl; }

void GraphNode::print() const { std::cout << to_string() << std::endl; }

GraphNode::GraphNode(const int id, const Graph& owner) : m_id(id), m_graph_owner(owner) {}

int GraphNode::get_id() const { return m_id; }

std::string GraphNode::to_string() const {
    std::string result = "Node " + std::to_string(m_id) + " neighbors:";
    for (const GraphEdge& edge : get_edges())
        result += " " + std::to_string(edge.get_to_id());
    return result;
}

size_t GraphNode::get_degree() const { return get_edges().size(); }

const Graph& GraphNode::get_owner() const { return m_graph_owner; }

const GraphNode& DirectedMultiGraph::add_node(const int node_id) {
    const GraphNode& node = Graph::add_node(node_id);
    m_nodeid_to_incoming_edgeids[node_id] = {};
    m_nodeid_to_outgoing_edgeids[node_id] = {};
    return node;
}

int DirectedMultiGraph::add_edge(const int from_id, const int to_id) {
    const int edge_id = Graph::add_edge(from_id, to_id);
    m_nodeid_to_incoming_edgeids[to_id].insert(edge_id);
    m_nodeid_to_outgoing_edgeids[from_id].insert(edge_id);
    return edge_id;
}

std::vector<GraphEdge> DirectedMultiGraph::get_edges_of_node(const int node_id) const {
    if (!has_node(node_id))
        throw std::runtime_error("DirectedMultiGraph::get_edges_of_node: node not found");
    std::vector<GraphEdge> edges;
    for (const int edge_id : m_nodeid_to_outgoing_edgeids.at(node_id))
        edges.push_back(get_edge_by_id(edge_id));
    return edges;
}

std::vector<GraphEdge> DirectedMultiGraph::get_in_edges_of_node(const int node_id) const {
    if (!has_node(node_id))
        throw std::runtime_error("DirectedMultiGraph::get_in_edges_of_node: node not found");
    std::vector<GraphEdge> edges;
    for (const int edge_id : m_nodeid_to_incoming_edgeids.at(node_id))
        edges.push_back(get_edge_by_id(edge_id));
    return edges;
}

size_t DirectedMultiGraph::get_in_degree_of_node(const int node_id) const {
    return m_nodeid_to_incoming_edgeids.at(node_id).size();
}

size_t DirectedMultiGraph::get_degree_of_node(int node_id) const {
    return m_nodeid_to_outgoing_edgeids.at(node_id).size();
}

void DirectedMultiGraph::remove_node(const int node_id) {
    Graph::remove_node(node_id);
    m_nodeid_to_incoming_edgeids.erase(node_id);
    m_nodeid_to_outgoing_edgeids.erase(node_id);
}

void DirectedMultiGraph::remove_edge(const int edge_id) {
    Graph::remove_edge(edge_id);
    const GraphEdge& edge = get_edge_by_id(edge_id);
    m_nodeid_to_incoming_edgeids[edge.get_to_id()].erase(edge_id);
    m_nodeid_to_outgoing_edgeids[edge.get_from_id()].erase(edge_id);
}

bool DirectedMultiGraph::has_edge(const int from_id, const int to_id) const {
    return get_edge_count(from_id, to_id) > 0;
}

int DirectedSimpleGraph::add_edge(const int from_id, const int to_id) {
    if (has_edge(from_id, to_id))
        throw std::runtime_error("DirectedSimpleGraph::add_edge: edge already exists");
    return DirectedMultiGraph::add_edge(from_id, to_id);
}

void DirectedSimpleGraph::remove_edge(int edge_id) { DirectedMultiGraph::remove_edge(edge_id); }

void DirectedSimpleGraph::remove_edge(const int from_id, const int to_id) {
    DirectedMultiGraph::remove_edge(get_edge_id(from_id, to_id));
}

int DirectedSimpleGraph::get_edge_id(const int from_id, const int to_id) const {
    return *get_edgeids(from_id, to_id).begin();
}

GraphEdge DirectedSimpleGraph::get_edge(const int from_id, const int to_id) const {
    return get_edge_by_id(get_edge_id(from_id, to_id));
}

size_t UndirectedMultiGraph::get_degree_of_node(const int node_id) const {
    return get_edges_of_node(node_id).size();
}

bool UndirectedMultiGraph::has_edge(const int from_id, const int to_id) const {
    return get_edge_count(from_id, to_id) + get_edge_count(to_id, from_id) > 0;
}

std::vector<GraphEdge> UndirectedMultiGraph::get_edges_of_node(const int node_id) const {
    std::vector<GraphEdge> edges = Graph::get_edges_of_node(node_id);
    for (GraphEdge& edge : edges)
        if (edge.get_to_id() == node_id)
            edge.reverse();
    return edges;
}

int UndirectedSimpleGraph::add_edge(const int from_id, const int to_id) {
    if (has_edge(from_id, to_id))
        throw std::runtime_error("UndirectedSimpleGraph::add_edge: edge already exists");
    return UndirectedMultiGraph::add_edge(from_id, to_id);
}

void UndirectedSimpleGraph::remove_edge(int edge_id) { UndirectedMultiGraph::remove_edge(edge_id); }

void UndirectedSimpleGraph::remove_edge(const int from_id, const int to_id) {
    const int edge_id = get_edge_id(from_id, to_id);
    UndirectedMultiGraph::remove_edge(edge_id);
}

int UndirectedSimpleGraph::get_edge_id(const int from_id, const int to_id) const {
    if (get_edge_count(from_id, to_id) == 1)
        return *get_edgeids(from_id, to_id).begin();
    if (get_edge_count(to_id, from_id) == 1)
        return *get_edgeids(to_id, from_id).begin();
    throw std::runtime_error("UndirectedSimpleGraph::get_edge_id: edge not found");
}
