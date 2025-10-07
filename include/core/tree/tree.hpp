#ifndef MY_TREE_H
#define MY_TREE_H

#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>

class TreeNode;

class Tree {
  int m_root_id;
  int m_next_node_id = 0;
  int m_next_edge_id = 0;
  std::unordered_map<int, std::unique_ptr<TreeNode>> m_nodeid_to_node_map;
  std::unordered_map<int, std::unordered_set<const TreeNode *>>
      m_nodeid_to_childrenid;
  std::unordered_map<int, int> m_nodeid_to_parentid;
  TreeNode &add_node_with_id(int id);

 public:
  explicit Tree(const int root_id) : m_root_id(root_id) {
    add_node_with_id(root_id);
  }
  const TreeNode &get_node_by_id(int node_id) const;
  auto get_nodes() const {
    return m_nodeid_to_node_map |
           std::views::transform([](const auto &pair) -> const TreeNode & {
             return *pair.second;
           });
  }
  bool is_root(int node_id) const;
  bool has_edge(int node_id_1, int node_id_2) const;
  const TreeNode &get_parent(int node_id) const;
  bool has_node(int id) const;
  void add_node(int id, int parent_id);
  void add_node(int parent_id);
  auto get_children(const int node_id) const {
    if (!has_node(node_id))
      throw std::runtime_error("Tree::get_children: node not found");
    return m_nodeid_to_childrenid.at(node_id) |
           std::views::transform([](const TreeNode *child) -> const TreeNode & {
             return *child;
           });
  }
  size_t size() const { return m_nodeid_to_node_map.size(); }
  std::string to_string() const;
  void print() const;
  // delete move constructor and move assignment operator
  Tree(const Tree &) = delete;
  Tree &operator=(const Tree &) = delete;
  // delete copy constructor and copy assignment operator
  Tree(Tree &&) = delete;
  Tree &operator=(Tree &&) = delete;
};

class TreeNode {
  int m_id;
  const Tree &m_owner;

 public:
  TreeNode(const int node_id, const Tree &owner)
      : m_id(node_id), m_owner(owner) {}
  [[nodiscard]] int get_id() const { return m_id; }
  [[nodiscard]] const TreeNode &get_parent() const {
    return m_owner.get_parent(m_id);
  }
  [[nodiscard]] auto get_children() const { return m_owner.get_children(m_id); }
  [[nodiscard]] bool is_root() const { return m_owner.is_root(m_id); }
  [[nodiscard]] std::string to_string() const;
  void print() const;
};

#endif