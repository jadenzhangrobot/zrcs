#pragma once

#include "XmlNode.h"
#include <string>
#include <vector>

namespace tinyxml2 { class XMLDocument; class XMLElement; }

class XmlParsing {
public:
    Tree<std::string>* tree;

    explicit XmlParsing(std::string xmlName);
    virtual ~XmlParsing();

    TreeNode<std::string>* getRootNode();
    TreeNode<std::string>* getnodeUniquePtr(std::string nodeName,
                                             TreeNode<std::string>* root);
    void getNodePtr(std::vector<TreeNode<std::string>*>& vecTreeNode,
                    std::string nodeName, TreeNode<std::string>* root);
    std::string getProperties(std::string name, TreeNode<std::string>* node);

private:
    tinyxml2::XMLDocument* doc_;
    std::string xmlpath_;
    std::string xmlFileName_;

    void parseXMLElement(tinyxml2::XMLElement* element,
                         TreeNode<std::string>* treeNode);
    void saveParaToXml(tinyxml2::XMLElement* element,
                       TreeNode<std::string>* treeNode);
    void SaveXmlElement();
};
