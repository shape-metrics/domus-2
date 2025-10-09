#include "orthogonal/file_loader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

Shape load_shape_from_file(const std::string& filename) {
    std::ifstream infile(filename);
    if (infile.is_open()) {
        Shape shape;
        std::string line;
        int from_index, to_index;
        std::string direction;
        while (std::getline(infile, line)) {
            if (line.find("//") == 0)
                continue;
            std::istringstream iss(line);
            if (iss >> from_index >> to_index >> direction)
                shape.set_direction(from_index, to_index, string_to_direction(direction));
        }
        infile.close();
        return shape;
    }
    throw std::runtime_error("Unable to open shape file: " + filename);
}

void save_shape_to_file(const DirectedSimpleGraph& graph,
                        const Shape& shape,
                        const std::string& filename) {
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        for (const GraphNode* node : graph.get_nodes()) {
            for (GraphEdge& edge : node->get_edges()) {
                const int neighbor_id = edge.get_to_id();
                outfile << node->get_id() << " " << neighbor_id << " ";
                outfile << direction_to_string(shape.get_direction(node->get_id(), neighbor_id))
                        << std::endl;
            }
        }
        outfile.close();
    } else
        throw std::runtime_error("Unable to save shape: " + filename);
}