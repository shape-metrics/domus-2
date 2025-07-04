#include "core/graph/segment.hpp"

Segment::Segment() {
  this->segment = std::make_unique<Graph>();
  this->attachments = {};
}

Graph& Segment::get_segment() { return *segment; }

const Graph& Segment::get_segment() const { return *segment; }

const std::unordered_set<int>& Segment::get_attachments() const {
  return attachments;
}

bool Segment::has_attachment(const int attachment_id) const {
  return attachments.contains(attachment_id);
}

void Segment::add_attachment(const int attachment_id) {
  attachments.insert(attachment_id);
}

std::string Segment::to_string() const {
  std::string result = "Segment:\n";
  result += segment->to_string();
  result += "Attachments: ";
  for (const int attachment_id : attachments)
    result += std::to_string(attachment_id) + " ";
  result += "\n";
  return result;
}

void Segment::print() const { std::cout << to_string(); }

bool is_segment_a_path(const Segment& segment) {
  for (const GraphNode& node : segment.get_segment().get_nodes()) {
    if (segment.has_attachment(node.get_id())) continue;
    if (node.get_degree() > 2) return false;
  }
  return true;
}

std::list<int> compute_path_between_attachments(const Segment& segment,
                                                const int attachment_1,
                                                const int attachment_2) {
  std::unordered_map<int, int> prev_of_node;
  std::list<int> queue{};
  queue.push_back(attachment_1);
  while (!queue.empty()) {
    const int node_id = queue.front();
    const GraphNode& node = segment.get_segment().get_node_by_id(node_id);
    queue.pop_front();
    for (const GraphEdge& edge : node.get_edges()) {
      const int neighbor_id = edge.get_to().get_id();
      if (neighbor_id == attachment_2) {
        if (node_id == attachment_1) continue;
        prev_of_node[neighbor_id] = node_id;
        break;
      }
      if (segment.has_attachment(neighbor_id)) continue;
      if (!prev_of_node.contains(neighbor_id)) {
        prev_of_node[neighbor_id] = node_id;
        queue.push_back(neighbor_id);
      }
    }
    if (prev_of_node.contains(attachment_2)) break;
  }
  std::list<int> path;
  int crawl = attachment_2;
  while (crawl != attachment_1) {
    path.push_front(crawl);
    crawl = prev_of_node[crawl];
  }
  path.push_front(crawl);
  return path;
}

void dfs_find_segments(const GraphNode& node,
                       std::unordered_set<int>& is_node_visited,
                       std::vector<int>& nodes_in_segment, const Cycle& cycle,
                       std::vector<std::pair<int, int>>& edges_in_segment) {
  const int node_id = node.get_id();
  nodes_in_segment.push_back(node_id);
  is_node_visited.insert(node_id);
  for (const GraphEdge& edge : node.get_edges()) {
    const int neighbor_id = edge.get_to().get_id();
    if (cycle.has_node(neighbor_id)) {
      edges_in_segment.emplace_back(node_id, neighbor_id);
      continue;
    }
    if (node_id < neighbor_id)
      edges_in_segment.emplace_back(node_id, neighbor_id);
    if (!is_node_visited.contains(neighbor_id))
      dfs_find_segments(edge.get_to(), is_node_visited, nodes_in_segment, cycle,
                        edges_in_segment);
  }
}

void add_cycle_edges(const Cycle& cycle, Segment& segment) {
  for (const int node_id : cycle) {
    const int next_node_id = cycle.next_of_node(node_id);
    segment.get_segment().add_undirected_edge(node_id, next_node_id);
  }
}

Segment build_segment(const std::vector<int>& nodes,
                      std::vector<std::pair<int, int>>& edges,
                      const Cycle& cycle) {
  Segment segment;
  for (const int node_id : cycle) segment.get_segment().add_node(node_id);
  for (const int node_id : nodes) segment.get_segment().add_node(node_id);
  // adding edges
  for (auto [from_id, to_id] : edges) {
    segment.get_segment().add_undirected_edge(from_id, to_id);
    // adding attachment
    if (cycle.has_node(from_id)) segment.add_attachment(from_id);
    if (cycle.has_node(to_id)) segment.add_attachment(to_id);
  }
  // adding cycle edges
  add_cycle_edges(cycle, segment);
  return segment;
}

void find_segments(const Graph& graph, const Cycle& cycle,
                   std::vector<Segment>& segments) {
  std::unordered_set<int> visited;
  for (const GraphNode& node : graph.get_nodes())
    if (cycle.has_node(node.get_id())) visited.insert(node.get_id());
  for (const GraphNode& node : graph.get_nodes()) {
    const int node_id = node.get_id();
    if (!visited.contains(node_id)) {
      std::vector<int> nodes;  // does NOT contain cycle nodes
      std::vector<std::pair<int, int>>
          edges;  // does NOT contain edges of the cycle
      dfs_find_segments(node, visited, nodes, cycle, edges);
      segments.push_back(build_segment(nodes, edges, cycle));
    }
  }
}

Segment build_chord(const int attachment_1, const int attachment_2,
                    const Cycle& cycle) {
  Segment chord;
  for (const int node_id : cycle) chord.get_segment().add_node(node_id);
  add_cycle_edges(cycle, chord);
  // adding chord edge
  chord.get_segment().add_undirected_edge(attachment_1, attachment_2);
  chord.add_attachment(attachment_1);
  chord.add_attachment(attachment_2);
  return chord;
}

void find_chords(const Graph& graph, const Cycle& cycle,
                 std::vector<Segment>& segments) {
  for (const int node_id : cycle) {
    const GraphNode& node = graph.get_node_by_id(node_id);
    for (const GraphEdge& edge : node.get_edges()) {
      const int neighbor_id = edge.get_to().get_id();
      if (node_id < neighbor_id) continue;
      if (cycle.has_node(neighbor_id))
        if (neighbor_id != cycle.prev_of_node(node_id) &&
            neighbor_id != cycle.next_of_node(node_id)) {
          segments.push_back(build_chord(node_id, neighbor_id, cycle));
        }
    }
  }
}

std::vector<Segment> compute_segments(const Graph& graph, const Cycle& cycle) {
  std::vector<Segment> segments;
  find_segments(graph, cycle, segments);
  find_chords(graph, cycle, segments);
  return segments;
}