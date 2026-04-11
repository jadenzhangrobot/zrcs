#pragma once

#include <string>
#include <map>
#include <stdexcept>

template <typename T>
class TreeNode {
public:
    T data;
    std::map<std::string, std::string> attribute;
    std::map<std::string, TreeNode*> children;

    explicit TreeNode(T value) : data(value) {}

    void addChild(std::string childName, TreeNode* child)
    {
        auto result = children.insert({childName, child});
        if (!result.second) {
            throw std::runtime_error("Child with name " + childName + " already exists.");
        }
    }
};

template <typename T>
class Tree {
public:
    TreeNode<T>* root;

    explicit Tree(T rootData) : root(new TreeNode<T>(rootData)) {}

    ~Tree() { destroyTree(root); root = nullptr; }

    void destroyTree(TreeNode<T>* node)
    {
        if (!node) return;
        for (auto& [name, child] : node->children)
            destroyTree(child);
        delete node;
    }
};
