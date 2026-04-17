#include "XmlParsing.h"

#include "tinyxml2.h"
#include <iostream>
#include <filesystem>
#include <stdexcept>

using namespace tinyxml2;

XmlParsing::XmlParsing(std::string xmlName)
    : doc_(new tinyxml2::XMLDocument()), tree(nullptr)
{
    xmlFileName_ = std::filesystem::path(xmlName).stem().string();

    std::string currentExePath = std::filesystem::current_path().string();
    std::string target = "build";
    std::string projectPath;
    size_t found = currentExePath.find(target);

    if (found != std::string::npos) {
        projectPath = currentExePath.substr(0, found);
    } else {
        throw std::runtime_error("Project 'zrcs' not found in path");
    }

    xmlpath_ = projectPath + "config/" + xmlName;

    int status = doc_->LoadFile(xmlpath_.c_str());
    if (status == XML_SUCCESS) {
        std::cout << "[XML] Successfully loaded " << xmlName << std::endl;
    } else if (status == XML_ERROR_FILE_NOT_FOUND) {
        throw std::runtime_error("can't find " + xmlpath_);
    } else if (status == XML_ERROR_PARSING_ATTRIBUTE || status == XML_CAN_NOT_CONVERT_TEXT) {
        throw std::runtime_error(xmlpath_ + " has syntax error");
    } else if (status == XML_NO_TEXT_NODE) {
        throw std::runtime_error("The name of rootelement is empty");
    } else {
        throw std::runtime_error("Failed to read XML file");
    }

    XMLElement* root = doc_->RootElement();
    if (root == nullptr) {
        throw std::runtime_error("Error: root element not found!");
    }

    tree = new Tree<std::string>(root->Name());
    parseXMLElement(root, tree->root);
}

XmlParsing::~XmlParsing()
{
    SaveXmlElement();

    XMLError saveResult = doc_->SaveFile(xmlpath_.c_str());
    if (saveResult != XML_SUCCESS && doc_->Error()) {
        // save failed, nothing actionable in destructor
    }

    delete doc_;
    delete tree;
    doc_ = nullptr;
    tree = nullptr;
}

void XmlParsing::parseXMLElement(XMLElement* element, TreeNode<std::string>* treeNode)
{
    const XMLAttribute* attribute = element->FirstAttribute();
    while (attribute) {
        treeNode->attribute.insert(std::make_pair(attribute->Name(), attribute->Value()));
        attribute = attribute->Next();
    }
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        TreeNode<std::string>* childNode = new TreeNode<std::string>(child->Name());
        treeNode->addChild(childNode->data, childNode);
        parseXMLElement(child, childNode);
    }
}

void XmlParsing::saveParaToXml(XMLElement* element, TreeNode<std::string>* treeNode)
{
    for (const auto& pair : treeNode->attribute) {
        element->SetAttribute(pair.first.c_str(), pair.second.c_str());
    }
    for (const auto& pair : treeNode->children) {
        XMLElement* childElement = element->GetDocument()->NewElement(pair.second->data.c_str());
        element->InsertEndChild(childElement);
        saveParaToXml(childElement, pair.second);
    }
}

TreeNode<std::string>* XmlParsing::getRootNode()
{
    return tree->root;
}

TreeNode<std::string>* XmlParsing::getnodeUniquePtr(std::string nodeName,
                                                       TreeNode<std::string>* root)
{
    if (root == nullptr) return nullptr;
    if (nodeName == root->data) return root;

    for (auto& [name, child] : root->children) {
        TreeNode<std::string>* found = getnodeUniquePtr(nodeName, child);
        if (found != nullptr) return found;
    }
    return nullptr;
}

void XmlParsing::getNodePtr(std::vector<TreeNode<std::string>*>& vecTreeNode,
                              std::string nodeName, TreeNode<std::string>* root)
{
    if (root == nullptr) return;
    if (root->data == nodeName) {
        vecTreeNode.push_back(root);
    }
    for (auto& [name, child] : root->children) {
        getNodePtr(vecTreeNode, nodeName, child);
    }
}

std::string XmlParsing::getProperties(std::string name, TreeNode<std::string>* node)
{
    for (const auto& pair : node->attribute) {
        if (pair.first == name) return pair.second;
    }
    for (auto& [childName, child] : node->children) {
        std::string result = getProperties(name, child);
        if (!result.empty()) return result;
    }
    return "";
}

void XmlParsing::SaveXmlElement()
{
    doc_->Clear();
    XMLDeclaration* decl = doc_->NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
    if (decl) {
        doc_->InsertFirstChild(decl);
    } else {
        std::cerr << "Failed to create XML Declaration." << std::endl;
    }
    const std::string& rootName = tree->root->data.empty() ? xmlFileName_ : tree->root->data;
    tinyxml2::XMLElement* rootElement = doc_->NewElement(rootName.c_str());
    doc_->InsertEndChild(rootElement);
    saveParaToXml(doc_->RootElement(), tree->root);
}
