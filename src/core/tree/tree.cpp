#include "core/tree/tree.hpp"

#include <iostream>

TreeNode& Tree::add_node_with_id(int id) {
    if (id < 0)
        throw std::runtime_error("Tree::add_node: id must be non-negative");
    if (has_node(id))
        throw std::runtime_error("Tree::add_node: node with this id already exists");
    m_nodeid_to_node_map[id] = std::make_unique<TreeNode>(id, *this);
    m_nodeid_to_childrenid[id] = {};
    return *m_nodeid_to_node_map[id];
}

const TreeNode& Tree::get_node_by_id(int node_id) const {
    if (!has_node(node_id))
        throw std::runtime_error("Tree::get_node_by_id: node not found");
    return *m_nodeid_to_node_map.at(node_id);
}

std::string TreeNode::to_string() const {
    std::string result = "Node " + std::to_string(m_id) + ": ";
    if (is_root())
        result += "root";
    else
        result += "parent: " + std::to_string(get_parent().get_id());
    result += ", children: ";
    for (const TreeNode& child : get_children())
        result += std::to_string(child.get_id()) + " ";
    return result;
}

std::string Tree::to_string() const {
    std::string result = "Tree:\n";
    for (const TreeNode& node : get_nodes())
        result += node.to_string() + "\n";
    return result;
}

bool Tree::has_edge(int node_id_1, int node_id_2) const {
    const TreeNode* node_1 = &get_node_by_id(node_id_1);
    const TreeNode* node_2 = &get_node_by_id(node_id_2);
    if (node_1->is_root())
        return node_2->get_parent().get_id() == node_id_1;
    if (node_2->is_root())
        return node_1->get_parent().get_id() == node_id_2;
    return &node_1->get_parent() == node_2 || &node_2->get_parent() == node_1;
}

bool Tree::is_root(const int node_id) const {
    if (!has_node(node_id))
        throw std::runtime_error("Tree::is_root: node not found");
    return !m_nodeid_to_parentid.contains(node_id);
}

const TreeNode& Tree::get_parent(const int node_id) const {
    if (is_root(node_id))
        throw std::runtime_error("Tree::get_parent: root node has no parent");
    return get_node_by_id(m_nodeid_to_parentid.at(node_id));
}

bool Tree::has_node(const int node_id) const { return m_nodeid_to_node_map.contains(node_id); }

void Tree::add_node(const int node_id, const int parent_id) {
    if (!has_node(parent_id))
        throw std::runtime_error("Tree::add_node: parent id does not exists");
    TreeNode& node = add_node_with_id(node_id);
    m_nodeid_to_parentid[node_id] = parent_id;
    m_nodeid_to_childrenid[parent_id].insert(&node);
    m_nodeid_to_childrenid[node_id] = {};
}

void Tree::add_node(int parent_id) {
    while (has_node(m_next_node_id))
        m_next_node_id++;
    add_node(m_next_node_id++, parent_id);
}

void Tree::print() const { std::cout << to_string() << std::endl; }

void TreeNode::print() const { std::cout << to_string() << std::endl; }