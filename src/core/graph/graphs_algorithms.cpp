#include "core/graph/graphs_algorithms.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/tree/tree.hpp"
#include "core/tree/tree_algorithms.hpp"

bool is_graph_connected(const UndirectedSimpleGraph &graph) {
  if (graph.size() == 0) return true;
  std::unordered_set<int> visited;
  std::vector<int> stack;
  stack.push_back(graph.get_nodes_ids()[0]);
  while (!stack.empty()) {
    const int node_id = stack.back();
    stack.pop_back();
    const GraphNode &node = graph.get_node_by_id(node_id);
    visited.insert(node.get_id());
    for (const GraphEdge &edge : node.get_edges())
      if (!visited.contains(edge.get_to_id()))
        stack.push_back(edge.get_to_id());
  }
  for (const int node_id : graph.get_nodes_ids())
    if (!visited.contains(node_id)) return false;
  return true;
}

std::vector<Cycle> compute_all_cycles_with_node_in_graph(
    const UndirectedSimpleGraph &graph, const GraphNode &node,
    const std::unordered_set<int> &taboo_nodes) {
  std::vector<Cycle> cycles;
  std::unordered_set<int> visited;
  std::function<void(int, int, std::vector<int> &)> dfs =
      [&](const int current, const int start, std::vector<int> &path) {
        visited.insert(current);
        path.push_back(current);
        for (const GraphEdge &edge :
             graph.get_node_by_id(current).get_edges()) {
          const int neighbor = edge.get_to_id();
          if (taboo_nodes.contains(neighbor)) continue;  // skip taboo nodes
          if (neighbor == start && path.size() > 2) {    // found a cycle
            cycles.emplace_back(path);
          } else if (!visited.contains(neighbor)) {
            dfs(neighbor, start, path);
          }
        }
        path.pop_back();
        visited.erase(current);
      };
  std::vector<int> path;
  dfs(node.get_id(), node.get_id(), path);
  return cycles;
}

std::vector<Cycle> compute_all_cycles_in_graph(
    const UndirectedSimpleGraph &graph) {
  std::vector<Cycle> all_cycles;
  std::unordered_set<int> taboo_nodes;
  for (const GraphNode *node : graph.get_nodes()) {
    std::vector<Cycle> cycles =
        compute_all_cycles_with_node_in_graph(graph, *node, taboo_nodes);
    for (Cycle &cycle : cycles) all_cycles.push_back(cycle);
    taboo_nodes.insert(node->get_id());
  }
  return all_cycles;
}

bool dfs_find_cycle(const GraphNode &node, const DirectedSimpleGraph &graph,
                    std::unordered_map<int, int> &state,
                    std::unordered_map<int, int> &parent,
                    std::optional<int> &cycle_start,
                    std::optional<int> &cycle_end) {
  state[node.get_id()] = 1;  // mark as visiting (gray)
  for (const GraphEdge &edge : node.get_edges()) {
    const int neighbor_id = edge.get_to_id();
    const GraphNode &neighbor = graph.get_node_by_id(neighbor_id);
    if (!state.contains(neighbor.get_id())) {  // unvisited
      parent[neighbor.get_id()] = node.get_id();
      if (dfs_find_cycle(neighbor, graph, state, parent, cycle_start,
                         cycle_end))
        return true;
    } else if (state[neighbor.get_id()] == 1) {
      cycle_start = neighbor.get_id();
      cycle_end = node.get_id();
      return true;
    }
  }
  state[node.get_id()] = 2;  // mark as fully processed (black)
  return false;
}

std::optional<Cycle> find_a_cycle_in_graph(const DirectedSimpleGraph &graph) {
  std::unordered_map<int, int> state;
  std::unordered_map<int, int> parent;
  std::optional<int> cycle_start = std::nullopt;
  std::optional<int> cycle_end = std::nullopt;
  for (const GraphNode *node : graph.get_nodes())
    if (!state.contains(node->get_id()))
      if (dfs_find_cycle(*node, graph, state, parent, cycle_start, cycle_end))
        break;
  if (!cycle_start.has_value()) return std::nullopt;
  std::vector<int> cycle;
  for (int v = cycle_end.value(); v != cycle_start; v = parent[v])
    cycle.push_back(v);
  cycle.push_back(cycle_start.value());
  std::ranges::reverse(cycle.begin(), cycle.end());
  return Cycle(cycle);
}

std::vector<Cycle> compute_cycle_basis(const UndirectedSimpleGraph &graph) {
  const std::unique_ptr<Tree> spanning = build_spanning_tree(graph);
  std::vector<Cycle> cycles;
  for (const GraphNode *node : graph.get_nodes()) {
    const int node_id = node->get_id();
    for (const GraphEdge &edge : node->get_edges()) {
      const int neighbor_id = edge.get_to_id();
      if (node_id > neighbor_id) continue;
      if (spanning->has_edge(node_id, neighbor_id)) continue;
      int common_ancestor =
          compute_common_ancestor(*spanning, node_id, neighbor_id);
      std::vector<int> path1 = get_path_from_root(*spanning, node_id);
      std::vector<int> path2 = get_path_from_root(*spanning, neighbor_id);
      std::ranges::reverse(path1.begin(), path1.end());
      std::ranges::reverse(path2.begin(), path2.end());
      while (path1.back() != common_ancestor) path1.pop_back();
      while (path2.back() != common_ancestor) path2.pop_back();
      std::ranges::reverse(path1.begin(), path1.end());
      path1.insert(path1.end(), path2.begin(), path2.end());
      path1.pop_back();
      cycles.emplace_back(path1);
    }
  }
  return cycles;
}

std::vector<int> make_topological_ordering(const DirectedSimpleGraph &graph) {
  std::unordered_map<int, int> in_degree;
  const std::vector<const GraphNode *> nodes = graph.get_nodes();
  for (const GraphNode *node : nodes) {
    for (const GraphEdge &edge : node->get_edges()) {
      const GraphNode &neighbor = graph.get_node_by_id(edge.get_to_id());
      if (!in_degree.contains(neighbor.get_id()))
        in_degree[neighbor.get_id()] = 0;
      in_degree[neighbor.get_id()]++;
    }
  }
  std::queue<const GraphNode *> queue;
  std::vector<int> topological_order;
  for (const GraphNode *node : nodes)
    if (in_degree[node->get_id()] == 0) queue.push(node);
  size_t count = 0;
  while (!queue.empty()) {
    const GraphNode *node = queue.front();
    ++count;
    queue.pop();
    topological_order.push_back(node->get_id());
    for (const GraphEdge &edge : node->get_edges()) {
      const GraphNode &neighbor = graph.get_node_by_id(edge.get_to_id());
      if (--in_degree[neighbor.get_id()] == 0) queue.push(&neighbor);
    }
  }
  if (count != graph.size()) throw std::runtime_error("Graph contains cycle");
  return topological_order;
}

bool are_cycles_equivalent(const Cycle &cycle1, const Cycle &cycle2) {
  if (cycle1.size() != cycle2.size()) return false;
  const int v = cycle1[0];
  if (!cycle2.has_node(v)) return false;
  int current_1 = v;
  int current_2 = v;
  for (size_t i = 1; i < cycle1.size(); ++i) {
    current_1 = cycle1.next_of_node(current_1);
    current_2 = cycle2.next_of_node(current_2);
    if (current_1 != current_2) return false;
  }
  // Check the reverse
  current_1 = v;
  current_2 = v;
  for (size_t i = 1; i < cycle1.size(); ++i) {
    current_1 = cycle1.next_of_node(current_1);
    current_2 = cycle2.prev_of_node(current_2);
    if (current_1 != current_2) return false;
  }
  return true;
}

std::vector<std::unique_ptr<UndirectedSimpleGraph>>
compute_connected_components(const UndirectedSimpleGraph &graph) {
  std::unordered_set<int> visited;
  std::vector<std::unique_ptr<UndirectedSimpleGraph>> components;
  std::function<void(const GraphNode &, UndirectedSimpleGraph &component)>
      explore_component = [&](const GraphNode &node,
                              UndirectedSimpleGraph &component) {
        visited.insert(node.get_id());
        for (const GraphEdge &edge : node.get_edges()) {
          const GraphNode &neighbor = graph.get_node_by_id(edge.get_to_id());
          if (!component.has_node(neighbor.get_id()))
            component.add_node(neighbor.get_id());
          if (!component.has_edge(node.get_id(), neighbor.get_id()))
            component.add_edge(node.get_id(), neighbor.get_id());
          if (!visited.contains(neighbor.get_id())) {
            explore_component(neighbor, component);
          }
        }
      };
  for (const GraphNode *node : graph.get_nodes())
    if (!visited.contains(node->get_id())) {
      auto new_component = std::make_unique<UndirectedSimpleGraph>();
      new_component->add_node(node->get_id());
      explore_component(*node, *new_component);
      components.push_back(std::move(new_component));
    }
  return components;
}

size_t compute_number_of_connected_components(
    const UndirectedSimpleGraph &graph) {
  std::unordered_set<int> visited;
  size_t components = 0;
  const std::function<void(const GraphNode &)> explore_component =
      [&](const GraphNode &start_node) {
        std::stack<const GraphNode *> stack;
        stack.push(&start_node);
        while (!stack.empty()) {
          const GraphNode *node = stack.top();
          stack.pop();
          if (visited.insert(node->get_id()).second)
            for (const GraphEdge &edge : node->get_edges()) {
              const GraphNode &neighbor =
                  graph.get_node_by_id(edge.get_to_id());
              if (!visited.contains(neighbor.get_id())) stack.push(&neighbor);
            }
        }
      };
  for (const GraphNode *node : graph.get_nodes())
    if (!visited.contains(node->get_id())) {
      components++;
      explore_component(*node);
    }
  return components;
}

void dfs_bic_com(
    const UndirectedSimpleGraph &graph, const GraphNode &node,
    std::unordered_map<int, int> &old_node_id_to_new_id,
    std::unordered_map<int, int> &prev_of_node, int &next_id_to_assign,
    std::unordered_map<int, int> &low_point, std::list<int> &stack_of_nodes,
    std::list<std::pair<int, int>> &stack_of_edges,
    std::vector<std::unique_ptr<UndirectedSimpleGraph>> &components,
    std::unordered_set<int> &cut_vertices);

BiconnectedComponents compute_biconnected_components(
    const UndirectedSimpleGraph &graph) {
  std::unordered_map<int, int> old_node_id_to_new_id;
  std::unordered_map<int, int> prev_of_node;
  std::unordered_map<int, int> low_point;
  std::unordered_set<int> cut_vertices;
  std::vector<std::unique_ptr<UndirectedSimpleGraph>> components;
  int next_id_to_assign = 0;
  std::list<int> stack_of_nodes{};
  std::list<std::pair<int, int>> stack_of_edges{};
  for (const GraphNode *node : graph.get_nodes())
    if (!old_node_id_to_new_id.contains(node->get_id()))  // node not visited
      dfs_bic_com(graph, *node, old_node_id_to_new_id, prev_of_node,
                  next_id_to_assign, low_point, stack_of_nodes, stack_of_edges,
                  components, cut_vertices);
  if (!stack_of_nodes.empty() || !stack_of_edges.empty())
    throw std::runtime_error(
        "Biconnected components algorithm did not finish correctly");
  BiconnectedComponents result{std::move(cut_vertices), std::move(components)};
  return result;
}

void build_component(UndirectedSimpleGraph &component,
                     const std::list<int> &nodes,
                     const std::list<std::pair<int, int>> &edges) {
  for (const int node : nodes) component.add_node(node);
  for (const auto &[from_id, to_id] : edges) component.add_edge(from_id, to_id);
}

void dfs_bic_com(
    const UndirectedSimpleGraph &graph, const GraphNode &node,
    std::unordered_map<int, int> &old_node_id_to_new_id,
    std::unordered_map<int, int> &prev_of_node, int &next_id_to_assign,
    std::unordered_map<int, int> &low_point, std::list<int> &stack_of_nodes,
    std::list<std::pair<int, int>> &stack_of_edges,
    std::vector<std::unique_ptr<UndirectedSimpleGraph>> &components,
    std::unordered_set<int> &cut_vertices) {
  int node_id = node.get_id();
  old_node_id_to_new_id[node_id] = next_id_to_assign;
  low_point[node_id] = next_id_to_assign;
  ++next_id_to_assign;
  int children_number = 0;
  for (const GraphEdge &edge : node.get_edges()) {
    const GraphNode &neighbor = graph.get_node_by_id(edge.get_to_id());
    const int neighbor_id = neighbor.get_id();
    if (prev_of_node.contains(node_id) && prev_of_node[node_id] == neighbor_id)
      continue;
    if (!old_node_id_to_new_id.contains(
            neighbor_id)) {  // means the node is not visited
      std::list<int> new_stack_of_nodes{};
      std::list<std::pair<int, int>> new_stack_of_edges{};
      ++children_number;
      prev_of_node[neighbor_id] = node_id;
      new_stack_of_nodes.push_back(neighbor_id);
      new_stack_of_edges.emplace_back(node_id, neighbor_id);
      dfs_bic_com(graph, neighbor, old_node_id_to_new_id, prev_of_node,
                  next_id_to_assign, low_point, new_stack_of_nodes,
                  new_stack_of_edges, components, cut_vertices);
      if (low_point[neighbor_id] < low_point[node_id])
        low_point[node_id] = low_point[neighbor_id];
      if (low_point[neighbor_id] >= old_node_id_to_new_id[node_id]) {
        new_stack_of_nodes.push_back(node_id);
        components.push_back(std::make_unique<UndirectedSimpleGraph>());
        build_component(*components.back(), new_stack_of_nodes,
                        new_stack_of_edges);
        if (prev_of_node.contains(
                node_id))  // the root needs to be handled differently
          // (handled at the end of the function)
          cut_vertices.insert(node_id);
      } else {
        stack_of_nodes.splice(stack_of_nodes.end(), new_stack_of_nodes);
        stack_of_edges.splice(stack_of_edges.end(), new_stack_of_edges);
      }
    } else {  // node got already visited
      const int neighbor_node_id = old_node_id_to_new_id[neighbor_id];
      if (neighbor_node_id < old_node_id_to_new_id[node_id]) {
        stack_of_edges.emplace_back(node_id, neighbor_id);
        if (neighbor_node_id < low_point[node_id])
          low_point[node_id] = neighbor_node_id;
      }
    }
  }
  if (!prev_of_node.contains(
          node_id)) {  // handling of node with no parents (the root)
    if (children_number >= 2)
      cut_vertices.insert(node_id);
    else if (children_number == 0) {  // node is isolated
      components.push_back(std::make_unique<UndirectedSimpleGraph>());
      components.back()->add_node(node_id);
    }
  }
}

std::string BiconnectedComponents::to_string() const {
  std::string result = "Biconnected Components:\n";
  result += "Cut vertices: ";
  for (const int cv : m_cutvertices) result += std::to_string(cv) + " ";
  result += "\nComponents:\n";
  for (const auto &component : m_components)
    result += component->to_string() + "\n";
  return result;
}

void BiconnectedComponents::print() const {
  std::cout << to_string() << std::endl;
}

std::pair<std::unique_ptr<UndirectedSimpleGraph>, GraphEdgeHashSet>
compute_maximal_degree_4_subgraph(const UndirectedSimpleGraph &graph) {
  auto subgraph = std::make_unique<UndirectedSimpleGraph>();
  GraphEdgeHashSet removed_edges;
  for (const int node_id : graph.get_nodes_ids()) subgraph->add_node(node_id);
  for (const GraphNode *node : graph.get_nodes()) {
    int node_id = node->get_id();
    for (const GraphEdge &edge : node->get_edges()) {
      int neighbor_id = edge.get_to_id();
      if (subgraph->has_edge(node_id, neighbor_id)) continue;
      if (removed_edges.contains({node_id, neighbor_id})) continue;
      if (subgraph->get_node_by_id(node_id).get_degree() < 4 &&
          subgraph->get_node_by_id(neighbor_id).get_degree() < 4) {
        subgraph->add_edge(node_id, neighbor_id);
      } else {
        removed_edges.insert({node_id, neighbor_id});
        removed_edges.insert({neighbor_id, node_id});
      }
    }
  }
  return std::make_pair(std::move(subgraph), std::move(removed_edges));
}

bool bfs_bipartition(const UndirectedSimpleGraph &graph, const GraphNode &node,
                     std::unordered_map<int, bool> &bipartition) {
  bipartition[node.get_id()] = false;
  std::list<const GraphNode *> queue;
  queue.push_back(&node);
  while (!queue.empty()) {
    const GraphNode &current = *queue.front();
    const int current_id = current.get_id();
    queue.pop_front();
    for (const GraphEdge &edge : current.get_edges()) {
      const int neighbor_id = edge.get_to_id();
      if (!bipartition.contains(neighbor_id)) {
        bipartition[neighbor_id] = !bipartition[current_id];
        queue.push_back(&graph.get_node_by_id(edge.get_to_id()));
      } else if (bipartition[neighbor_id] == bipartition[current_id])
        return false;
    }
  }
  return true;
}

std::optional<std::unordered_map<int, bool>> compute_bipartition(
    const UndirectedSimpleGraph &graph) {
  std::unordered_map<int, bool> bipartition{};
  for (const GraphNode *node : graph.get_nodes())
    if (!bipartition.contains(node->get_id()))
      if (!bfs_bipartition(graph, *node, bipartition)) return std::nullopt;
  return bipartition;
}

std::optional<Cycle> find_a_cycle_in_graph(const UndirectedSimpleGraph &graph) {
  std::unordered_set<int> visited;
  std::unordered_map<int, int> parent;
  for (const GraphNode *start_node : graph.get_nodes()) {
    const int start_id = start_node->get_id();
    if (visited.contains(start_id)) continue;
    std::vector<const GraphNode *> stack;
    stack.push_back(start_node);
    while (!stack.empty()) {
      const GraphNode &current = *stack.back();
      stack.pop_back();
      int current_id = current.get_id();
      visited.insert(current_id);
      for (const GraphEdge &edge : current.get_edges()) {
        int neighbor_id = edge.get_to_id();
        if (!visited.contains(neighbor_id)) {
          parent[neighbor_id] = current_id;
          stack.push_back(&graph.get_node_by_id(edge.get_to_id()));
        } else if (neighbor_id != parent[current_id]) {
          std::vector<int> cycle;
          int x = current_id;
          int y = neighbor_id;
          std::unordered_set<int> path_x;
          while (true) {
            path_x.insert(x);
            if (!parent.contains(x)) break;
            x = parent[x];
          }
          std::vector<int> path_to_lca;
          while (!path_x.contains(y)) {
            path_to_lca.push_back(y);
            if (!parent.contains(y)) break;
            y = parent[y];
          }
          cycle.push_back(y);
          x = current_id;
          while (x != y) {
            cycle.push_back(x);
            x = parent[x];
          }
          std::ranges::reverse(path_to_lca);
          cycle.insert(cycle.end(), path_to_lca.begin(), path_to_lca.end());
          return Cycle{cycle};
        }
      }
    }
  }
  return std::nullopt;  // No cycle found
}

std::unordered_set<int> &BiconnectedComponents::get_cutvertices() {
  return m_cutvertices;
}
const std::unordered_set<int> &BiconnectedComponents::get_cutvertices() const {
  return m_cutvertices;
}

const std::vector<std::unique_ptr<UndirectedSimpleGraph>> &
BiconnectedComponents::get_components() const {
  return m_components;
}

BiconnectedComponents::BiconnectedComponents(
    std::unordered_set<int> &&cutvertices,
    std::vector<std::unique_ptr<UndirectedSimpleGraph>> &&components)
    : m_cutvertices(std::move(cutvertices)),
      m_components(std::move(components)) {}