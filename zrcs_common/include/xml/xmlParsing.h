/**
 * @file XmlParsing.h
 * @author zhangyongjing (6499894200@qq.com)
 * @brief 
 * @version 0.1
 * @date 2024-12-18
 * 
 * @copyright Copyright (c) 2024
 * 
 */
 #ifndef XMLPARSING_H
 #define XMLPARSING_H
 #include "tinyxml2.h"
 #include <string>
 #include <sys/stat.h>
 #include "XmlNode.h"
 #include <filesystem>
 using namespace tinyxml2;
 class XmlParsing
 {
     private:
         tinyxml2::XMLDocument* doc;
         std::string xmlpath ;
         std::string xmlFileName;
         public:
         Tree<std::string>* tree;
     public:
         XmlParsing(std::string xmlName):doc(new tinyxml2::XMLDocument())
         {
                     xmlFileName = std::filesystem::path(xmlName).stem().string();
            std::string currentExePath = std::filesystem::current_path().string();
            std::string target = "build";
            std::string projectPath ;
            size_t found = currentExePath.find(target);

            if (found != std::string::npos)
            {
            // 截取 "build" 前面的子串,工程文件不能放在build文件夹下
             projectPath  = currentExePath.substr(0, found);
            }
            else {
                throw std::runtime_error("Project 'zrcs' not found in path");
            }
             xmlpath = projectPath + "config/" + xmlName;
             int status = doc->LoadFile(xmlpath.c_str());
             if(status==XML_SUCCESS)
             {
                 // XML file loaded successfully
                  std::cout << "[XML] Successfully loaded " << xmlName << std::endl;
             
             }
             else if(status==XML_ERROR_FILE_NOT_FOUND)
             {
                 // File not found
                 std::string str="can't find \n";
                 throw str+xmlpath;
             }else if(status==XML_ERROR_PARSING_ATTRIBUTE||status== XML_CAN_NOT_CONVERT_TEXT)
             {
                 // Syntax error
                 std::string str=" has syntax error\n";
                 throw xmlpath+str;
             }else if(status==XML_NO_TEXT_NODE)
             {
                 // Root element name is empty
                 std::string str="The name of rootelement is empty\n";
                 throw str;
             }
             else {
                throw std::runtime_error("Failed to read XML file");
             }
             
          XMLElement* root=doc->RootElement();
            if (root == nullptr) 
            {
                throw std::runtime_error("Error: root element not found!");
            }
         tree=new Tree<std::string>(root->Name());

         parseXMLElement(root, tree->root);
     }
      /**
       * @brief 遍历xml节点，把xml节点信息存储到树节点中
       * 
       * @param element 
       * @param treeNode 
       */
     void parseXMLElement(XMLElement* element, TreeNode<std::string>* treeNode)
     {
         const XMLAttribute* attribute = element->FirstAttribute();
         while (attribute) 
         {
             treeNode->attribute.insert(std::make_pair(attribute->Name(), attribute->Value()));
             attribute = attribute->Next();
         }
         for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
         { 
             TreeNode<std::string>* childNode = new TreeNode<std::string>(child->Name());
             treeNode->addChild(childNode->data,childNode);
             parseXMLElement(child, childNode);
         }
     }
     void saveParaToXml(XMLElement* element, TreeNode<std::string>* treeNode)
     {
            // 遍历树节点的属性并保存到XML元素
            for (const auto& pair : treeNode->attribute) 
            {
                element->SetAttribute(pair.first.c_str(), pair.second.c_str());
            }
            // 遍历树节点的子节点并保存到XML元素
            for (const auto& pair : treeNode->children) 
            {
                XMLElement* childElement = element->GetDocument()->NewElement(pair.second->data.c_str());
                element->InsertEndChild(childElement);
                saveParaToXml(childElement, pair.second);
            }

     }
     /**
      * @brief 获取树的根节点
      * 
      * @return TreeNode<std::string>* 
      */
 TreeNode<std::string>* getRootNode()
 {
     return tree->root;
 }

 /**
  * @brief 根据节点的名字获取节点上一级的指针，节点的名字要是独一无二的
  * 
  * @param nodeName 
  * @param root 根节点的指针
  * @return TreeNode<std::string>* 
  */
 TreeNode<std::string>* getnodeUniquePtr(std::string nodeName,TreeNode<std::string>* root )
 {
    if (root == nullptr) 
     {
         return nullptr;
     }
 
     if (nodeName == root->data)
     {
         return root;
     }
 
     for (int i = 0; i < root->children.size(); i++) 
     {
        // TreeNode<std::string>* found = getnodeUniquePtr(nodeName, root->children[i]);
        // if (found != nullptr) {
          //   return found; // 如果在子节点中找到匹配节点，直接返回
       //  }
     }
 
     return nullptr; // 找不到匹配节点时返回空指针
 }
 
 
 void getNodePtr( std::vector<TreeNode<std::string>*> &vecTreeNode ,std::string nodeName, TreeNode<std::string>* root)
 {
     for (int i = 0; i < root->children.size(); i++)
     {
        // getNodePtr(vecTreeNode,nodeName, root->children[i]);
            
     }
     if (root!=nullptr)
      {
         if (root->data == nodeName)
         {     
                 vecTreeNode.push_back(root);             
         }
      }   
 }
     /**
      * @brief 根据属性的名字全局查找想对应的值，只能查找属性名字唯一的元素。
      * 
      * @param root 
      * @param name 
      * @return std::string 
      */
 std::string getProperties(std::string name, TreeNode<std::string>* node)
 {   
     // 在当前节点中查找属性
     for (const auto &pair : node->attribute) 
     {
         if (pair.first == name) 
         {
             return pair.second; // 找到匹配属性，立即返回属性值
         }
     }
 
     // 在子节点中递归查找属性
     for (int i = 0; i < node->children.size(); i++)
     {
       //  std::string childResult = getProperties(name, node->children[i]);
        //  if (!childResult.empty()) // 如果子节点找到了属性值，则返回该值
        //  {
        //      return childResult;
        //  }
     }
 
     return ""; // 未找到匹配属性，返回空字符串
 }
 /**
  * @brief 重现保存xml文件
  * 
  */
    void SaveXmlElement()
    {
            doc->Clear();
            XMLDeclaration* decl = doc->NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
             if (decl) { // 总是好的做法检查 New... 函数的返回值
            doc->InsertFirstChild(decl);
            } else {
                std::cerr << "Failed to create XML Declaration." << std::endl;
                // 处理错误，可能退出或记录
            }
                const std::string& rootName = tree->root->data.empty() ? xmlFileName : tree->root->data;
                tinyxml2::XMLElement* rootElement = doc->NewElement(rootName.c_str());
             doc->InsertEndChild(rootElement);
            saveParaToXml(doc->RootElement(), tree->root);
    }
    virtual ~XmlParsing()
     {         
            SaveXmlElement();          
            
            XMLError saveResult = doc->SaveFile(xmlpath.c_str());

            if (saveResult == XML_SUCCESS) 
            {
               
            } else {               
                // 或者使用 doc.ErrorStr() 获取更详细的错误（如果可用）
                if (doc->Error()) {
                   // std::cerr << "Detailed error: " << doc.ErrorStr() << std::endl;
                }
            }
             delete doc;
             delete tree;
             doc=nullptr;
             tree=nullptr;
     }
  
 };
 #endif
 