#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

// 树节点类
template <typename T>
class TreeNode {
public:
    T data;
    std::map<std::string,std::string> attribute;
    std::map<std::string,TreeNode*> children;

    TreeNode(T value) : data(value) 
    {

    }
    // 添加子节点
     void addChild(std::string childName,TreeNode* child)
     {
        auto result = children.insert({childName, child});
        if (!result.second) 
        { 
            throw std::runtime_error("Child with name " + childName + " already exists.");
        }
        
     }
};

// 树类
template <typename T>
class Tree {
public:
    TreeNode<T>* root;
    Tree(T rootData) 
    {
        root = new TreeNode<T>(rootData);
    }

    // 销毁树
    void destroyTree(TreeNode<T>* node) 
    {
        if (node == nullptr)
        {
            return;
        }
        for (TreeNode<T>* child : node->children)
        {
            destroyTree(child);
        }
        delete node;
    }
};
