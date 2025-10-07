#include "orthogonal/shape/shape_builder.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "core/tree/tree.hpp"
#include "core/tree/tree_algorithms.hpp"
#include "orthogonal/shape/clauses_functions.hpp"
#include "orthogonal/shape/variables_handler.hpp"
#include "sat/cnf_builder.hpp"
#include "sat/glucose.hpp"

const std::string unit_clauses_logs_file = "unit_clauses_logs.txt";
std::mutex unit_clauses_logs_mutex;

Shape result_to_shape(const UndirectedSimpleGraph &graph,
                      const std::vector<int> &numbers,
                      VariablesHandler &handler) {
  for (const int var : numbers) {
    if (var > 0)
      handler.set_variable_value(var, true);
    else
      handler.set_variable_value(-var, false);
  }
  Shape shape;
  for (const GraphNode *node : graph.get_nodes()) {
    const int node_id = node->get_id();
    for (const GraphEdge &edge : node->get_edges()) {
      const int neighbor_id = edge.get_to_id();
      shape.set_direction(node_id, neighbor_id,
                          handler.get_direction_of_edge(node_id, neighbor_id));
    }
  }
  return shape;
}

std::pair<int, int> find_edges_to_split(
    const std::vector<std::string> &proof_lines, std::mt19937 &random_engine,
    const VariablesHandler &handler) {
  std::vector<int> unit_clauses;
  for (size_t i = proof_lines.size(); i > 0; i--) {
    const std::string &line = proof_lines[i - 1];
    // split line based on " "
    std::vector<int> tokens;
    std::string token;
    for (char c : line) {
      if (c == 'd') continue;
      if (c == ' ') {
        if (token.empty()) continue;
        tokens.push_back(std::stoi(token));
        token = "";
      } else
        token += c;
    }
    if (token != "0") throw std::runtime_error("Invalid proof line");
    if (tokens.size() == 1) unit_clauses.push_back(tokens[0]);
  }
  if (unit_clauses.empty())
    throw std::runtime_error("Could not find the edge to remove");
  // pick one of the first two unit clauses
  size_t random_index =
      random_engine() % std::min(unit_clauses.size(), static_cast<size_t>(2));
  const int variable = std::abs(unit_clauses[random_index]);
  std::lock_guard lock(unit_clauses_logs_mutex);
  std::ofstream log_file(unit_clauses_logs_file, std::ios_base::app);
  if (log_file) {
    log_file << "units " << unit_clauses.size() << "\n";
    log_file.close();
  } else {
    throw std::runtime_error("Error: Could not open log file for writing: " +
                             unit_clauses_logs_file);
  }
  return handler.get_edge_of_variable(variable);
}

std::optional<Shape> build_shape_or_add_corner(UndirectedSimpleGraph &graph,
                                               GraphAttributes &attributes,
                                               std::vector<Cycle> &cycles,
                                               std::mt19937 &random_engine);

Shape build_shape(UndirectedSimpleGraph &graph, GraphAttributes &attributes,
                  std::vector<Cycle> &cycles, const bool randomize) {
  const size_t seed = randomize ? std::random_device{}() : 42;
  std::mt19937 random_engine(seed);
  std::optional<Shape> shape =
      build_shape_or_add_corner(graph, attributes, cycles, random_engine);
  while (!shape.has_value())
    shape = build_shape_or_add_corner(graph, attributes, cycles, random_engine);
  return std::move(shape.value());
}

void add_corner_inside_edge(const int from_id, const int to_id,
                            UndirectedSimpleGraph &graph,
                            GraphAttributes &attributes,
                            std::vector<Cycle> &cycles) {
  if (!graph.has_edge(from_id, to_id))
    throw std::runtime_error("Error: The edge is not in the graph");
  const int new_node_id = graph.add_node().get_id();
  attributes.set_node_color(new_node_id, Color::RED);
  graph.remove_edge(from_id, to_id);
  graph.add_edge(from_id, new_node_id);
  graph.add_edge(to_id, new_node_id);
  for (Cycle &cycle : cycles) {
    if (!cycle.has_node(from_id) || !cycle.has_node(to_id)) continue;
    const size_t from_pos = cycle.node_position(from_id);
    const size_t to_pos = cycle.node_position(to_id);
    if (cycle.next_of_node(from_id) == to_id)
      cycle.insert(to_pos, new_node_id);
    else if (cycle.next_of_node(to_id) == from_id)
      cycle.insert(from_pos, new_node_id);
  }
}

std::optional<Shape> build_shape_or_add_corner(UndirectedSimpleGraph &graph,
                                               GraphAttributes &attributes,
                                               std::vector<Cycle> &cycles,
                                               std::mt19937 &random_engine) {
  VariablesHandler handler(graph);
  CnfBuilder cnf_builder{};
  cnf_builder.add_comment("constraints one direction per edge");
  add_constraints_one_direction_per_edge(graph, cnf_builder, handler);
  cnf_builder.add_comment("constraints nodes");
  add_nodes_constraints(graph, cnf_builder, handler);
  cnf_builder.add_comment("constraints cycles");
  add_cycles_constraints(cnf_builder, cycles, handler);
  const std::string cnf = get_unique_filename("cnf");
  cnf_builder.convert_to_cnf(cnf);
  const auto [result, numbers, proof_lines] = launch_glucose(cnf, false);
  remove(cnf.c_str());
  if (result == GlucoseResultType::UNSAT) {
    const auto [from_id, to_id] =
        find_edges_to_split(proof_lines, random_engine, handler);
    add_corner_inside_edge(from_id, to_id, graph, attributes, cycles);
    return std::nullopt;
  }
  return result_to_shape(graph, numbers, handler);
}