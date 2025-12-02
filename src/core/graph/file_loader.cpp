#include "core/graph/file_loader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::unique_ptr<UndirectedSimpleGraph> load_graph_from_txt_file(const std::string& filename) {
    auto graph = std::make_unique<UndirectedSimpleGraph>();
    load_graph_from_txt_file(filename, *graph);
    return graph;
}

void load_graph_from_txt_file(const std::string& filename, UndirectedSimpleGraph& graph) {
    if (graph.size() > 0)
        throw std::runtime_error("Graph is not empty. Please use a new graph.");
    // example: "../domus/rome_graphs/grafo1848.18.txt"
    // load the file now
    std::ifstream infile(filename);
    if (!infile)
        throw std::runtime_error("Could not open file: " + filename);
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
                if (iss >> node_id)
                    graph.add_node(node_id);
            } else if (section == EDGES) {
                int from, to;
                if (iss >> from >> to)
                    graph.add_edge(from, to);
            }
        }
    }
}

void save_graph_to_file(const UndirectedSimpleGraph& graph, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile)
        throw std::runtime_error("Could not write to file: " + filename);
    outfile << "nodes:\n";
    for (const int node_id : graph.get_nodes_ids())
        outfile << node_id << '\n';
    outfile << "edges:\n";
    for (const GraphEdge& edge : graph.get_edges())
        outfile << edge.get_from_id() << ' ' << edge.get_to_id() << '\n';
}

std::string any_to_string(const std::any& value) {
    if (!value.has_value()) {
        return "";
    }
    return std::any_cast<std::string>(value);
}

void write_data_tag(std::ostream& os, const std::string& key_id, const std::string& value) {
    os << "    <data key=\"" << key_id << "\">" << value << "</data>\n";
}

void save_to_graphml(std::ostream& os, const Graph& graph, const GraphAttributes& attributes) {
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    os << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"\n";
    os << "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n";
    os << "         xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns\n";
    os << "         http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">\n\n";
    if (attributes.has_attribute(Attribute::NODES_COLOR))
        os << "  <key id=\"d0\" for=\"node\" attr.name=\"color\" attr.type=\"string\"/>\n";
    
    if (attributes.has_attribute(Attribute::NODES_POSITION)) {
        os << "  <key id=\"d1\" for=\"node\" attr.name=\"pos_x\" attr.type=\"int\"/>\n";
        os << "  <key id=\"d2\" for=\"node\" attr.name=\"pos_y\" attr.type=\"int\"/>\n";
    }
    if (attributes.has_attribute(Attribute::EDGES_ANY_LABEL)) {
        os << "  <key id=\"d3\" for=\"edge\" attr.name=\"label\" attr.type=\"string\"/>\n";
    }
    os << "\n";
    os << "  <graph id=\"G\" edgedefault=\"undirected\">\n";
    for (int node_id : graph.get_nodes_ids()) {
        os << "    <node id=\"n" << node_id << "\">\n";
        if (attributes.has_attribute(Attribute::NODES_COLOR)) {
            const Color color = attributes.get_node_color(node_id);
            write_data_tag(os, "d0", color_to_string(color));
        }
        if (attributes.has_attribute(Attribute::NODES_POSITION)) {
            write_data_tag(os, "d1", std::to_string(attributes.get_position_x(node_id)));
            write_data_tag(os, "d2", std::to_string(attributes.get_position_y(node_id)));
        }
        os << "    </node>\n";
    }
    for (const auto& edge : graph.get_edges()) {
        const int edge_id = edge.get_id();
        const int source_id = edge.get_from_id();
        const int target_id = edge.get_to_id();
        os << "    <edge id=\"e" << edge_id << "\" source=\"n" << source_id << "\" target=\"n" << target_id << "\">\n";
        if (attributes.has_attribute(Attribute::EDGES_ANY_LABEL)) {
            const std::any& label = attributes.get_edge_any_label(edge_id);
            if (label.has_value())
                write_data_tag(os, "d3", any_to_string(label));
        }        
        os << "    </edge>\n";
    }
    os << "\n";
    os << "  </graph>\n";
    os << "</graphml>\n";
}

void save_graph_to_graphml_file(const UndirectedSimpleGraph& graph,
                                const GraphAttributes& attributes,
                                const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile)
        throw std::runtime_error("Could not write to file: " + filename);
    save_to_graphml(outfile, graph, attributes);
}