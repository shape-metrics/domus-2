#include "core/graph/file_loader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::unique_ptr<Graph> load_graph_from_txt_file(const std::string& filename) {
  auto graph = std::make_unique<Graph>();
  load_graph_from_txt_file(filename, *graph);
  return graph;
}

void load_graph_from_txt_file(const std::string& filename, Graph& graph) {
  if (graph.size() > 0)
    throw std::runtime_error("Graph is not empty. Please use a new graph.");
  // example: "../domus/rome_graphs/grafo1848.18.txt"
  // load the file now
  std::ifstream infile(filename);
  if (!infile) throw std::runtime_error("Could not open file: " + filename);
  std::string line;
  enum Section { NONE, NODES, EDGES } section = NONE;
  while (std::getline(infile, line)) {
    if (line == "nodes:") {
      section = NODES;
    } else if (line == "edges:") {
      section = EDGES;
    } else if (!line.empty()) {
      std::istringstream iss(line);
      if (section == NODES) {
        int node_id;
        if (iss >> node_id) graph.add_node(node_id);
      } else if (section == EDGES) {
        int from, to;
        if (iss >> from >> to) graph.add_edge(from, to);
      }
    }
  }
  infile.close();
}

void save_graph_to_file(const Graph& graph, const std::string& filename) {
  std::ofstream outfile(filename);
  if (!outfile)
    throw std::runtime_error("Could not write to file: " + filename);
  outfile << "nodes:\n";
  for (auto node : graph.get_nodes()) outfile << node.get_id() << '\n';
  outfile << "edges:\n";
  for (const auto& edge : graph.get_edges())
    outfile << edge.get_from().get_id() << ' ' << edge.get_to().get_id()
            << '\n';
  outfile.close();
}
