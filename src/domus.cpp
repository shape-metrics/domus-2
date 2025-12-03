#include <emscripten/emscripten.h>

#include "core/graph/file_loader.hpp"
#include "core/graph/graphs_algorithms.hpp"
#include "orthogonal/drawing_builder.hpp"

extern "C" {
EMSCRIPTEN_KEEPALIVE
int compute_orthogonal_drawing() {
    const auto graph = load_graph_from_txt_file("input.txt");
    if (graph->size() > 30) // graph too large
        return -1;
    try {
        const DrawingResult result = make_orthogonal_drawing(*graph);
        make_svg(*result.augmented_graph, result.attributes, "output.svg");
        save_graph_to_graphml_file(*result.augmented_graph, result.attributes, "output.graphml");
        return 0;
    } catch (const DisconnectedGraphError& e) { // graph is not connected
        return -2;
    } catch (const std::exception& e) { // other errors
        return -3;
    }
}
}