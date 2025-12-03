#include "orthogonal/drawing_builder.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "core/graph/graphs_algorithms.hpp"
#include "core/utils.hpp"
#include "drawing/svg_drawer.hpp"
#include "orthogonal/area_compacter.hpp"
#include "orthogonal/equivalence_classes.hpp"
#include "orthogonal/shape/shape_builder.hpp"

std::vector<int> path_in_class(const UndirectedSimpleGraph& graph,
                               const int from,
                               const int to,
                               const Shape& shape,
                               const bool go_horizontal) {
    std::vector<int> path;
    std::unordered_set<int> visited;
    std::function<void(int)> dfs = [&](const int current) {
        if (current == to) {
            path.push_back(current);
            return;
        }
        visited.insert(current);
        for (const GraphEdge& edge : graph.get_node_by_id(current).get_edges()) {
            int neighbor = edge.get_to_id();
            if (visited.contains(neighbor))
                continue;
            if (go_horizontal == shape.is_horizontal(current, neighbor)) {
                dfs(neighbor);
                if (!path.empty()) {
                    path.push_back(current);
                    return;
                }
            }
        }
        visited.erase(current);
    };
    dfs(from);
    std::ranges::reverse(path);
    return path;
}

Cycle build_cycle_in_graph_from_cycle_in_ordering(
    const UndirectedSimpleGraph& graph,
    const Shape& shape,
    const Cycle& cycle_in_ordering,
    const DirectedSimpleGraph& ordering,
    const GraphAttributes& ordering_edge_to_graph_edge,
    const bool go_horizontal) {
    std::vector<int> cycle;
    for (size_t i = 0; i < cycle_in_ordering.size(); ++i) {
        const int class_id = cycle_in_ordering[i];
        const int next_class_id = cycle_in_ordering.next_of_node(class_id);
        const GraphEdge& edge = ordering.get_edge(class_id, next_class_id);
        const std::any& edge_label = ordering_edge_to_graph_edge.get_edge_any_label(edge.get_id());
        const int from = std::any_cast<std::pair<int, int>>(edge_label).first;
        const int to = std::any_cast<std::pair<int, int>>(edge_label).second;
        cycle.push_back(from);
        const int next_next_class_id = cycle_in_ordering.next_of_node(next_class_id);
        const GraphEdge& next_edge = ordering.get_edge(next_class_id, next_next_class_id);
        const std::any& next_edge_label =
            ordering_edge_to_graph_edge.get_edge_any_label(next_edge.get_id());
        const int next_from = std::any_cast<std::pair<int, int>>(next_edge_label).first;
        if (to != next_from) {
            std::vector<int> path = path_in_class(graph, to, next_from, shape, go_horizontal);
            const auto end = static_cast<size_t>(static_cast<int>(path.size()) - 1);
            for (size_t j = 0; j < end; ++j)
                cycle.push_back(path[j]);
        }
    }
    return Cycle(cycle);
}

void make_svg(const UndirectedSimpleGraph& graph,
              const GraphAttributes& attributes,
              const std::string& filename) {
    int max_x = -INT_MAX;
    int max_y = -INT_MAX;
    const std::vector<const GraphNode*> nodes = graph.get_nodes();
    for (const GraphNode* node : nodes) {
        max_x = std::max(max_x, attributes.get_position_x(node->get_id()));
        max_y = std::max(max_y, attributes.get_position_y(node->get_id()));
    }
    int min_x = INT_MAX;
    int min_y = INT_MAX;
    for (const GraphNode* node : nodes) {
        min_x = std::min(min_x, attributes.get_position_x(node->get_id()));
        min_y = std::min(min_y, attributes.get_position_y(node->get_id()));
    }
    const double ratio = 1.0 * (max_x - min_x) / (max_y - min_y);
    const int width = static_cast<int>(ratio * 900.0);
    constexpr int height = 900;
    SvgDrawer drawer{width, height};
    auto scale_x = ScaleLinear(min_x - 100, max_x + 100, 0, width);
    auto scale_y = ScaleLinear(min_y - 100, max_y + 100, 0, height);
    std::unordered_map<int, Point2D> points;
    for (const GraphNode* node : nodes) {
        const double x = scale_x.map(attributes.get_position_x(node->get_id()));
        const double y = scale_y.map(attributes.get_position_y(node->get_id()));
        points.emplace(node->get_id(), Point2D(x, y));
    }
    for (const GraphNode* node : nodes) {
        const int i = node->get_id();
        for (const GraphEdge& edge : node->get_edges()) {
            int j = edge.get_to_id();
            Line2D line(points.at(i), points.at(j));
            drawer.add(line);
        }
    }
    for (const GraphNode* node : graph.get_nodes()) {
        const Color color = attributes.get_node_color(node->get_id());
        if (color == Color::RED)
            continue;
        if (color == Color::GREEN)
            continue;
        if (color == Color::BLUE)
            continue;
        if (color == Color::RED_SPECIAL)
            continue;
        if (color == Color::BLUE_DARK)
            continue;
        if (color == Color::GREEN_DARK)
            continue;
        const size_t side =
            node->get_degree() <= 4
                ? 25
                : static_cast<size_t>(ceil(25 * sqrt(static_cast<double>(node->get_degree() - 3))));
        Square2D square{points.at(node->get_id()), static_cast<double>(side)};
        square.setColor(color_to_string(color));
        square.setLabel(std::to_string(node->get_id()));
        drawer.add(square);
    }
    drawer.save_to_file(filename);
}

// useless bends are red nodes with two horizontal or vertical edges
void remove_useless_bends(UndirectedSimpleGraph& graph,
                          const GraphAttributes& attributes,
                          Shape& shape) {
    std::vector<int> nodes_to_remove;
    for (const GraphNode* node : graph.get_nodes()) {
        const int node_id = node->get_id();
        if (attributes.get_node_color(node_id) == Color::BLACK)
            continue;
        std::vector<GraphEdge> edges = node->get_edges();
        const int j_1 = edges[0].get_to_id();
        const int j_2 = edges[1].get_to_id();
        // if the added corner is flat, remove it
        if (shape.is_horizontal(node_id, j_1) == shape.is_horizontal(node_id, j_2))
            nodes_to_remove.push_back(node_id);
    }
    for (const int node_id : nodes_to_remove) {
        const GraphNode& node = graph.get_node_by_id(node_id);
        std::vector<GraphEdge> edges = node.get_edges();
        const int j_1 = edges[0].get_to_id();
        const int j_2 = edges[1].get_to_id();
        const Direction direction = shape.get_direction(j_1, node_id);
        graph.remove_node(node_id);
        graph.add_edge(j_1, j_2);
        shape.remove_direction(node_id, j_1);
        shape.remove_direction(node_id, j_2);
        shape.remove_direction(j_1, node_id);
        shape.remove_direction(j_2, node_id);
        shape.set_direction(j_1, j_2, direction);
        shape.set_direction(j_2, j_1, opposite_direction(direction));
    }
}

DrawingResult make_orthogonal_drawing_incremental(const UndirectedSimpleGraph& graph,
                                                  std::vector<Cycle>& cycles);

DrawingResult make_orthogonal_drawing(const UndirectedSimpleGraph& graph) {
    std::vector<Cycle> cycles = compute_cycle_basis(graph);
    return make_orthogonal_drawing_incremental(graph, cycles);
}

std::optional<Cycle> check_if_metrics_exist(Shape& shape, UndirectedSimpleGraph& graph) {
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ordering_x_edge_to_graph_edge, ordering_y_edge_to_graph_edge] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    std::optional<Cycle> cycle_x = find_a_cycle_in_graph(*ordering_x);
    std::optional<Cycle> cycle_y = find_a_cycle_in_graph(*ordering_y);
    if (cycle_x.has_value()) {
        return build_cycle_in_graph_from_cycle_in_ordering(graph,
                                                           shape,
                                                           cycle_x.value(),
                                                           *ordering_x,
                                                           ordering_x_edge_to_graph_edge,
                                                           false);
    }
    if (cycle_y.has_value()) {
        return build_cycle_in_graph_from_cycle_in_ordering(graph,
                                                           shape,
                                                           cycle_y.value(),
                                                           *ordering_y,
                                                           ordering_y_edge_to_graph_edge,
                                                           true);
    }
    return std::nullopt;
}

void build_nodes_positions(UndirectedSimpleGraph& graph, GraphAttributes& attributes, Shape& shape);

bool has_graph_degree_more_than_4(const UndirectedSimpleGraph& graph) {
    for (const GraphNode* node : graph.get_nodes())
        if (node->get_degree() > 4)
            return true;
    return false;
}

void add_green_blue_nodes(UndirectedSimpleGraph& graph, GraphAttributes& attributes, Shape& shape);

void make_shifts_overlapped_edges(UndirectedSimpleGraph& graph,
                                  GraphAttributes& attributes,
                                  Shape& shape);

void fix_negative_positions(const UndirectedSimpleGraph& graph, GraphAttributes& attributes);

DrawingResult make_orthogonal_drawing_incremental(const UndirectedSimpleGraph& graph,
                                                  std::vector<Cycle>& cycles) {
    if (!is_graph_connected(graph))
        throw DisconnectedGraphError();
    auto augmented_graph = std::make_unique<UndirectedSimpleGraph>();
    GraphAttributes attributes;
    attributes.add_attribute(Attribute::NODES_COLOR);
    for (const int node_id : graph.get_nodes_ids()) {
        augmented_graph->add_node(node_id);
        attributes.set_node_color(node_id, Color::BLACK);
    }
    for (const GraphNode* node : graph.get_nodes())
        for (const GraphEdge& edge : node->get_edges())
            if (node->get_id() < edge.get_to_id())
                augmented_graph->add_edge(node->get_id(), edge.get_to_id());
    Shape shape = build_shape(*augmented_graph, attributes, cycles);
    std::optional<Cycle> cycle_to_add = check_if_metrics_exist(shape, *augmented_graph);
    size_t number_of_added_cycles = 0;
    while (cycle_to_add.has_value()) {
        cycles.push_back(std::move(*cycle_to_add));
        number_of_added_cycles++;
        shape = build_shape(*augmented_graph, attributes, cycles);
        cycle_to_add = check_if_metrics_exist(shape, *augmented_graph);
    }
    const size_t old_size = augmented_graph->size();
    remove_useless_bends(*augmented_graph, attributes, shape);
    // from now on cycles are not valid anymore
    const size_t number_of_cycles = cycles.size();
    cycles.clear();
    const size_t number_of_useless_bends = old_size - augmented_graph->size();
    if (has_graph_degree_more_than_4(*augmented_graph)) {
        add_green_blue_nodes(*augmented_graph, attributes, shape);
        build_nodes_positions(*augmented_graph, attributes, shape);
        make_shifts_overlapped_edges(*augmented_graph, attributes, shape);
        fix_negative_positions(*augmented_graph, attributes);
    } else {
        build_nodes_positions(*augmented_graph, attributes, shape);
    }
    compact_area(*augmented_graph, attributes);
    return {std::move(augmented_graph),
            std::move(attributes),
            std::move(shape),
            number_of_cycles - number_of_added_cycles,
            number_of_added_cycles,
            number_of_useless_bends};
}

void find_inconsistencies(UndirectedSimpleGraph& graph, Shape& shape, GraphAttributes& attributes);

void build_nodes_positions(UndirectedSimpleGraph& graph,
                           GraphAttributes& attributes,
                           Shape& shape) {
    find_inconsistencies(graph, shape, attributes);
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ignored_1, ignored_2] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    auto new_classes_x_ordering = make_topological_ordering(*ordering_x);
    auto new_classes_y_ordering = make_topological_ordering(*ordering_y);
    int current_position_x = -100;
    std::unordered_map<int, int> node_id_to_position_x;
    for (const int class_id : new_classes_x_ordering) {
        int next_position_x = current_position_x + 100;
        for (const int node_id : classes_x.get_elems_of_class(class_id))
            if (attributes.get_node_color(node_id) == Color::BLUE)
                next_position_x = current_position_x + 100;
        for (const int node_id : classes_x.get_elems_of_class(class_id))
            node_id_to_position_x[node_id] = next_position_x;
        current_position_x = next_position_x;
    }
    int current_position_y = -100;
    std::unordered_map<int, int> node_id_to_position_y;
    for (const int class_id : new_classes_y_ordering) {
        int next_position_y = current_position_y + 100;
        for (const int node_id : classes_y.get_elems_of_class(class_id))
            if (attributes.get_node_color(node_id) == Color::GREEN)
                next_position_y = current_position_y + 100;
        for (const int node_id : classes_y.get_elems_of_class(class_id))
            node_id_to_position_y[node_id] = next_position_y;
        current_position_y = next_position_y;
    }
    attributes.add_attribute(Attribute::NODES_POSITION);
    for (int node_id : graph.get_nodes_ids()) {
        const int x = node_id_to_position_x[node_id];
        const int y = node_id_to_position_y[node_id];
        attributes.set_position(node_id, x, y);
    }
}

int min_coordinate(std::unordered_map<int, std::unordered_set<int>> coordinate_to_nodes) {
    int min_c = INT_MAX;
    for (const int coord : coordinate_to_nodes | std::views::keys)
        if (coord < min_c)
            min_c = coord;
    return min_c;
}

std::pair<std::unordered_map<int, int>, std::unordered_map<int, int>>
compute_node_to_index_position(const UndirectedSimpleGraph& graph,
                               const GraphAttributes& attributes) {
    std::unordered_map<int, std::unordered_set<int>> coordinate_y_to_nodes;
    for (const GraphNode* node : graph.get_nodes()) {
        const int node_id = node->get_id();
        const int y = attributes.get_position_y(node_id);
        coordinate_y_to_nodes[y].insert(node_id);
    }
    std::unordered_map<int, std::unordered_set<int>> coordinate_x_to_nodes;
    for (const GraphNode* node : graph.get_nodes()) {
        const int node_id = node->get_id();
        const int x = attributes.get_position_x(node_id);
        coordinate_x_to_nodes[x].insert(node_id);
    }
    int y_index = 0;
    std::unordered_map<int, int> node_to_coordinate_y;
    int min_y = min_coordinate(coordinate_y_to_nodes);
    while (true) {
        for (const int node_id : coordinate_y_to_nodes[min_y])
            node_to_coordinate_y[node_id] = y_index;
        coordinate_y_to_nodes.erase(min_y);
        if (coordinate_y_to_nodes.empty())
            break;
        const int next_min_y = min_coordinate(coordinate_y_to_nodes);
        if (next_min_y - min_y == 100)
            ++y_index;
        min_y = next_min_y;
    }
    int x_index = 0;
    std::unordered_map<int, int> node_to_coordinate_x;
    int min_x = min_coordinate(coordinate_x_to_nodes);
    while (true) {
        for (const int node_id : coordinate_x_to_nodes[min_x])
            node_to_coordinate_x[node_id] = x_index;
        coordinate_x_to_nodes.erase(min_x);
        if (coordinate_x_to_nodes.empty())
            break;
        const int next_min_x = min_coordinate(coordinate_x_to_nodes);
        if (next_min_x - min_x == 100)
            ++x_index;
        min_x = next_min_x;
    }
    return std::make_pair(node_to_coordinate_x, node_to_coordinate_y);
}

auto find_edges_to_fix(const UndirectedSimpleGraph& graph,
                       const Shape& shape,
                       const GraphAttributes& attributes) {
    std::unordered_map<int, int> node_to_leftest_up;
    std::unordered_map<int, int> node_to_leftest_down;
    std::unordered_map<int, int> node_to_downest_left;
    std::unordered_map<int, int> node_to_downest_right;
    for (const GraphNode* node : graph.get_nodes()) {
        if (node->get_degree() <= 4)
            continue;
        const int node_id = node->get_id();
        std::optional<int> downest_left = std::nullopt;
        std::optional<int> downest_right = std::nullopt;
        std::optional<int> leftest_up = std::nullopt;
        std::optional<int> leftest_down = std::nullopt;
        for (const GraphEdge& edge : node->get_edges()) {
            const GraphNode& added = graph.get_node_by_id(edge.get_to_id());
            const int added_id = added.get_id();
            if (shape.is_horizontal(node_id, added_id)) {
                if (shape.is_left(node_id, added_id))
                    throw std::runtime_error("wtf 0");
                int other_neighbor_id = 0;
                bool found = false;
                for (const GraphEdge& added_edge : added.get_edges()) {
                    int neighbor_id = added_edge.get_to_id();
                    if (neighbor_id == node_id)
                        continue;
                    found = true;
                    other_neighbor_id = neighbor_id;
                }
                if (!found)
                    throw std::runtime_error("wtf 1");
                if (shape.is_up(added_id, other_neighbor_id)) {
                    if (!leftest_up.has_value())
                        leftest_up = added_id;
                    else if (attributes.get_position_x(added_id) <
                             attributes.get_position_x(leftest_up.value()))
                        leftest_up = added_id;
                } else {
                    if (!leftest_down.has_value())
                        leftest_down = added_id;
                    else if (attributes.get_position_x(added_id) <
                             attributes.get_position_x(leftest_down.value()))
                        leftest_down = added_id;
                }
            } else {
                if (shape.is_down(node_id, added_id))
                    throw std::runtime_error("wtf 2");
                int other_neighbor_id = 0;
                bool found = false;
                for (const GraphEdge& green_edge : added.get_edges()) {
                    int neighbor_id = green_edge.get_to_id();
                    if (neighbor_id == node_id)
                        continue;
                    found = true;
                    other_neighbor_id = neighbor_id;
                }
                if (!found)
                    throw std::runtime_error("wtf 3");
                if (shape.is_left(added_id, other_neighbor_id)) {
                    if (!downest_left.has_value())
                        downest_left = added_id;
                    else if (attributes.get_position_y(added_id) <
                             attributes.get_position_y(downest_left.value()))
                        downest_left = added_id;
                } else {
                    if (!downest_right.has_value())
                        downest_right = added_id;
                    else if (attributes.get_position_y(added_id) <
                             attributes.get_position_y(downest_right.value())) {
                        downest_right = added_id;
                    }
                }
            }
        }
        node_to_leftest_up[node_id] = leftest_up.value();
        node_to_leftest_down[node_id] = leftest_down.value();
        node_to_downest_left[node_id] = downest_left.value();
        node_to_downest_right[node_id] = downest_right.value();
    }
    return std::make_tuple(std::move(node_to_leftest_up),
                           std::move(node_to_leftest_down),
                           std::move(node_to_downest_left),
                           std::move(node_to_downest_right));
}

int get_other_neighbor_id(const UndirectedSimpleGraph& graph,
                          const int node_id,
                          const int neighbor_id) {
    for (const GraphEdge& edge : graph.get_node_by_id(node_id).get_edges()) {
        if (edge.get_to_id() != neighbor_id) {
            return edge.get_to_id();
        }
    }
    throw std::runtime_error("No other neighbor found for node " + std::to_string(node_id));
}

void fix_edge(UndirectedSimpleGraph& graph,
              int node_id,
              int other_node_id,
              Shape& shape,
              GraphAttributes& attributes,
              Direction direction) {
    int other_neighbor_id = get_other_neighbor_id(graph, other_node_id, node_id);
    graph.remove_node(other_node_id);
    attributes.remove_position(other_node_id);
    attributes.remove_nodes_attribute(other_node_id);
    graph.add_edge(node_id, other_neighbor_id);
    shape.remove_direction(node_id, other_node_id);
    shape.remove_direction(other_node_id, node_id);
    shape.remove_direction(other_node_id, other_neighbor_id);
    shape.remove_direction(other_neighbor_id, other_node_id);
    shape.set_direction(node_id, other_neighbor_id, direction);
    shape.set_direction(other_neighbor_id, node_id, opposite_direction(direction));
}

// at the moment, a node with degree > 4 doesn't have all its "ports" used,
// this method takes some of its neighbors and places them in the unused
// "ports"
void fix_useless_green_blue_nodes(UndirectedSimpleGraph& graph,
                                  GraphAttributes& attributes,
                                  Shape& shape) {
    auto [node_to_leftest_up, node_to_leftest_down, node_to_downest_left, node_to_downest_right] =
        find_edges_to_fix(graph, shape, attributes);
    for (auto [node, leftest_up] : node_to_leftest_up)
        fix_edge(graph, node, leftest_up, shape, attributes, Direction::UP);
    for (auto [node, leftest_down] : node_to_leftest_down)
        fix_edge(graph, node, leftest_down, shape, attributes, Direction::DOWN);
    for (auto [node, downest_left] : node_to_downest_left)
        fix_edge(graph, node, downest_left, shape, attributes, Direction::LEFT);
    for (auto [node, downest_right] : node_to_downest_right)
        fix_edge(graph, node, downest_right, shape, attributes, Direction::RIGHT);
}

void add_green_blue_nodes(UndirectedSimpleGraph& graph, GraphAttributes& attributes, Shape& shape) {
    std::vector<const GraphNode*> nodes;
    for (const GraphNode* node : graph.get_nodes())
        if (node->get_degree() > 4)
            nodes.push_back(node);
    std::unordered_set<int> added_nodes_ids;
    for (const GraphNode* node : nodes) {
        int node_id = node->get_id();
        std::vector<std::pair<int, int>> edges_to_remove;
        std::vector<std::pair<int, int>> edges_to_add;
        for (const GraphEdge& edge : node->get_edges()) {
            const GraphNode& neighbor = graph.get_node_by_id(edge.get_to_id());
            const int neighbor_id = neighbor.get_id();
            const GraphNode& added = graph.add_node();
            const int added_id = added.get_id();
            added_nodes_ids.insert(added_id);
            edges_to_add.emplace_back(added_id, node_id);
            edges_to_add.emplace_back(added_id, neighbor_id);
            shape.set_direction(added_id, neighbor_id, shape.get_direction(node_id, neighbor_id));
            shape.set_direction(neighbor_id, added_id, shape.get_direction(neighbor_id, node_id));
            if (shape.is_horizontal(node_id, neighbor_id)) {
                attributes.set_node_color(added_id, Color::GREEN);
                shape.set_direction(node_id, added_id, Direction::UP);
                shape.set_direction(added_id, node_id, Direction::DOWN);
            } else {
                attributes.set_node_color(added_id, Color::BLUE);
                shape.set_direction(node_id, added_id, Direction::RIGHT);
                shape.set_direction(added_id, node_id, Direction::LEFT);
            }
            shape.remove_direction(node_id, neighbor_id);
            shape.remove_direction(neighbor_id, node_id);
            edges_to_remove.emplace_back(node_id, neighbor_id);
        }
        for (auto [from_id, to_id] : edges_to_add)
            graph.add_edge(from_id, to_id);
        for (auto [from_id, to_id] : edges_to_remove)
            graph.remove_edge(from_id, to_id);
    }
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto ordering = equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    std::unique_ptr<DirectedSimpleGraph>& ordering_x = std::get<0>(ordering);
    std::unique_ptr<DirectedSimpleGraph>& ordering_y = std::get<1>(ordering);
    std::vector<int> classes_x_ordering = make_topological_ordering(*ordering_x);
    std::vector<int> classes_y_ordering = make_topological_ordering(*ordering_y);
    int current_position_x = 0;
    std::unordered_map<int, int> node_id_to_position_x;
    for (const int class_id : classes_x_ordering) {
        for (const int node_id : classes_x.get_elems_of_class(class_id))
            node_id_to_position_x[node_id] = 100 * current_position_x;
        ++current_position_x;
    }
    int current_position_y = 0;
    std::unordered_map<int, int> node_id_to_position_y;
    for (const int class_id : classes_y_ordering) {
        for (const int node_id : classes_y.get_elems_of_class(class_id))
            node_id_to_position_y[node_id] = 100 * current_position_y;
        ++current_position_y;
    }
    attributes.add_attribute(Attribute::NODES_POSITION);
    for (int node_id : graph.get_nodes_ids()) {
        const int x = node_id_to_position_x[node_id];
        const int y = node_id_to_position_y[node_id];
        attributes.set_position(node_id, x, y);
    }
    fix_useless_green_blue_nodes(graph, attributes, shape);
    attributes.remove_attribute(Attribute::NODES_POSITION);
}

void fix_inconsistency(const Cycle& cycle,
                       GraphAttributes& attributes,
                       const UndirectedSimpleGraph& graph,
                       Shape& shape,
                       const Color color_to_find) {
    const Direction direction = color_to_find == Color::GREEN ? Direction::UP : Direction::RIGHT;
    const Color dark_color = color_to_find == Color::GREEN ? Color::GREEN_DARK : Color::BLUE_DARK;
    std::optional<int> colored_node;
    for (const int node_id : cycle) {
        if (attributes.get_node_color(node_id) != color_to_find)
            continue;
        colored_node = node_id;
    }
    if (!colored_node.has_value())
        throw std::runtime_error("fix_inconsistency: no colored node found");
    const int colored_node_id = colored_node.value();
    int neighbors_ids[2];
    int i = 0;
    for (const GraphEdge& edge : graph.get_node_by_id(colored_node_id).get_edges()) {
        neighbors_ids[i] = edge.get_to_id();
        ++i;
    }
    if (shape.is_up(neighbors_ids[0], colored_node_id)) {
        shape.remove_direction(colored_node_id, neighbors_ids[0]);
        shape.remove_direction(neighbors_ids[0], colored_node_id);
        shape.set_direction(colored_node_id, neighbors_ids[0], direction);
        shape.set_direction(neighbors_ids[0], colored_node_id, opposite_direction(direction));
    } else {
        shape.remove_direction(colored_node_id, neighbors_ids[1]);
        shape.remove_direction(neighbors_ids[1], colored_node_id);
        shape.set_direction(colored_node_id, neighbors_ids[1], direction);
        shape.set_direction(neighbors_ids[1], colored_node_id, opposite_direction(direction));
    }
    attributes.change_node_color(colored_node_id, dark_color);
}

void find_inconsistencies(UndirectedSimpleGraph& graph, Shape& shape, GraphAttributes& attributes) {
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ordering_x_edge_to_graph_edge, ordering_y_edge_to_graph_edge] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    std::optional<Cycle> cycle_x = find_a_cycle_in_graph(*ordering_x);
    std::optional<Cycle> cycle_y = find_a_cycle_in_graph(*ordering_y);
    if (cycle_x.has_value() || cycle_y.has_value()) {
        if (cycle_x.has_value()) {
            Cycle cycle = build_cycle_in_graph_from_cycle_in_ordering(graph,
                                                                      shape,
                                                                      cycle_x.value(),
                                                                      *ordering_x,
                                                                      ordering_x_edge_to_graph_edge,
                                                                      false);
            fix_inconsistency(cycle, attributes, graph, shape, Color::BLUE);
        } else {
            Cycle cycle = build_cycle_in_graph_from_cycle_in_ordering(graph,
                                                                      shape,
                                                                      cycle_y.value(),
                                                                      *ordering_y,
                                                                      ordering_y_edge_to_graph_edge,
                                                                      true);
            fix_inconsistency(cycle, attributes, graph, shape, Color::GREEN);
        }
        find_inconsistencies(graph, shape, attributes);
    }
}

template <typename Func>
void shifting_order(const int node_id,
                    UndirectedSimpleGraph& graph,
                    Shape& shape,
                    std::vector<int>& nodes_at_direction,
                    GraphAttributes& attributes,
                    const Direction increasing_direction,
                    Func get_position) {
    const Direction decreasing_direction = opposite_direction(increasing_direction);
    std::sort(nodes_at_direction.begin(), nodes_at_direction.end(), [&](int a, int b) {
        if (attributes.get_node_color(a) == Color::BLACK) {
            const int b_other_neighbor_id = get_other_neighbor_id(graph, b, node_id);
            return shape.get_direction(b, b_other_neighbor_id) == increasing_direction;
        }
        if (attributes.get_node_color(b) == Color::BLACK) {
            const int a_other_neighbor_id = get_other_neighbor_id(graph, a, node_id);
            return shape.get_direction(a, a_other_neighbor_id) == decreasing_direction;
        }
        const int a_other_neighbor = get_other_neighbor_id(graph, a, node_id);
        const int b_other_neighbor = get_other_neighbor_id(graph, b, node_id);
        if (shape.get_direction(a, a_other_neighbor) == increasing_direction &&
            shape.get_direction(b, b_other_neighbor) == decreasing_direction) {
            return false;
        }
        if (shape.get_direction(a, a_other_neighbor) == decreasing_direction &&
            shape.get_direction(b, b_other_neighbor) == increasing_direction) {
            return true;
        }
        if (shape.get_direction(a, a_other_neighbor) == increasing_direction &&
            shape.get_direction(b, b_other_neighbor) == increasing_direction) {
            return get_position(attributes, a) > get_position(attributes, b);
        }
        return get_position(attributes, a) < get_position(attributes, b);
    });
}

size_t find_fixed_index_node(const GraphAttributes& attributes,
                             const std::vector<int>& nodes_at_direction) {
    for (size_t i = 0; i < nodes_at_direction.size(); ++i) {
        const int node_id = nodes_at_direction[i];
        if (attributes.get_node_color(node_id) == Color::BLACK)
            return i;
    }
    return nodes_at_direction.size() / 2;
}

enum class Axis { X, Y };

void make_shifts(const int node_id,
                 UndirectedSimpleGraph& graph,
                 Shape& shape,
                 GraphAttributes& attributes,
                 std::vector<int>& right_nodes,
                 const Axis axis,
                 const Direction increasing_direction,
                 const Color color) {
    auto position_function =
        axis == Axis::X
            ? [](const GraphAttributes& a, const int id) { return a.get_position_x(id); }
            : [](const GraphAttributes& a, const int id) { return a.get_position_y(id); };
    shifting_order(node_id,
                   graph,
                   shape,
                   right_nodes,
                   attributes,
                   increasing_direction,
                   position_function);
    const auto position_function_other =
        axis == Axis::X
            ? [](const GraphAttributes& a, const int id) { return a.get_position_y(id); }
            : [](const GraphAttributes& a, const int id) { return a.get_position_x(id); };
    const auto change_position_other = axis == Axis::X
                                           ? [](GraphAttributes& a,
                                                const int id,
                                                const int value) { a.change_position_y(id, value); }
                                           : [](GraphAttributes& a, const int id, const int value) {
                                                 a.change_position_x(id, value);
                                             };
    const size_t index_of_fixed_node = find_fixed_index_node(attributes, right_nodes);
    const int initial_position = position_function_other(attributes, node_id);
    for (const GraphNode* node : graph.get_nodes()) {
        const int old_position_y = position_function_other(attributes, node->get_id());
        if (old_position_y > initial_position) {
            const auto node_count = static_cast<int>(right_nodes.size());
            const int offset = node_count - static_cast<int>(index_of_fixed_node) - 1;
            const int new_position_y = old_position_y + 5 * offset;
            change_position_other(attributes, node->get_id(), new_position_y);
        }
        if (old_position_y < initial_position) {
            const int new_position_y = old_position_y - 5 * static_cast<int>(index_of_fixed_node);
            change_position_other(attributes, node->get_id(), new_position_y);
        }
    }
    for (size_t i = 0; i < right_nodes.size(); ++i) {
        if (i == index_of_fixed_node)
            continue;
        const int node_to_shift_id = right_nodes[i];
        const int shift = (static_cast<int>(i) - static_cast<int>(index_of_fixed_node)) * 5;
        const int node_to_shift_neighbor_id =
            get_other_neighbor_id(graph, node_to_shift_id, node_id);
        const Direction direction =
            shape.get_direction(node_to_shift_id, node_to_shift_neighbor_id);
        const int added_node_id = graph.add_node().get_id();
        attributes.set_node_color(added_node_id, color);
        shape.set_direction(node_id, added_node_id, direction);
        shape.set_direction(added_node_id, node_id, opposite_direction(direction));
        shape.set_direction(added_node_id, node_to_shift_id, direction);
        shape.set_direction(node_to_shift_id, added_node_id, opposite_direction(direction));
        shape.remove_direction(node_id, node_to_shift_id);
        shape.remove_direction(node_to_shift_id, node_id);
        graph.remove_edge(node_id, node_to_shift_id);
        graph.add_edge(node_id, added_node_id);
        graph.add_edge(added_node_id, node_to_shift_id);
        if (axis == Axis::X)
            attributes.set_position(added_node_id,
                                    attributes.get_position_x(node_id),
                                    initial_position + shift);
        else
            attributes.set_position(added_node_id,
                                    initial_position + shift,
                                    attributes.get_position_y(node_id));
        change_position_other(attributes,
                              node_to_shift_id,
                              position_function_other(attributes, added_node_id));
    }
}

auto neighbors_at_each_direction(const GraphNode& node, const Shape& shape) {
    std::unordered_map<Direction, std::vector<int>> nodes_at_direction;
    for (const GraphEdge& edge : node.get_edges()) {
        const int neighbor_id = edge.get_to_id();
        const Direction dir = shape.get_direction(node.get_id(), neighbor_id);
        nodes_at_direction[dir].push_back(neighbor_id);
    }
    return nodes_at_direction;
}

void make_shifts_overlapped_edges(UndirectedSimpleGraph& graph,
                                  GraphAttributes& attributes,
                                  Shape& shape) {
    std::vector<const GraphNode*> nodes;
    for (const GraphNode* node : graph.get_nodes())
        if (node->get_degree() > 4)
            nodes.push_back(node);
    for (const GraphNode* node : nodes) {
        const int node_id = node->get_id();
        std::unordered_map<Direction, std::vector<int>> nodes_to_sort =
            neighbors_at_each_direction(*node, shape);
        make_shifts(node_id,
                    graph,
                    shape,
                    attributes,
                    nodes_to_sort[Direction::RIGHT],
                    Axis::X,
                    Direction::UP,
                    Color::GREEN);
        make_shifts(node_id,
                    graph,
                    shape,
                    attributes,
                    nodes_to_sort[Direction::UP],
                    Axis::Y,
                    Direction::RIGHT,
                    Color::BLUE);
        make_shifts(node_id,
                    graph,
                    shape,
                    attributes,
                    nodes_to_sort[Direction::LEFT],
                    Axis::X,
                    Direction::UP,
                    Color::GREEN_DARK);
        make_shifts(node_id,
                    graph,
                    shape,
                    attributes,
                    nodes_to_sort[Direction::DOWN],
                    Axis::Y,
                    Direction::RIGHT,
                    Color::BLUE_DARK);
    }
}

void fix_negative_positions(const UndirectedSimpleGraph& graph, GraphAttributes& attributes) {
    int min_x = INT_MAX;
    int min_y = INT_MAX;
    const std::vector<int>& nodes_ids = graph.get_nodes_ids();
    for (const int node_id : nodes_ids) {
        min_x = std::min(min_x, attributes.get_position_x(node_id));
        min_y = std::min(min_y, attributes.get_position_y(node_id));
    }
    if (min_x < 0) {
        for (const int node_id : nodes_ids)
            attributes.change_position_x(node_id, attributes.get_position_x(node_id) - min_x);
    }
    if (min_y < 0) {
        for (const int node_id : nodes_ids) {
            attributes.change_position_y(node_id, attributes.get_position_y(node_id) - min_y);
        }
    }
}