#include <iostream>
#include <vector>
#include <map>

// 树节点类
template <typename T>
class TreeNode {
public:
    T data;
    std::map<std::string,std::string> attribute;
    std::vector<TreeNode*> children;

    TreeNode(T value) : data(value) 
    {

    }
    // 添加子节点
     void addChild(TreeNode* child)
     {
        children.push_back(child);
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
