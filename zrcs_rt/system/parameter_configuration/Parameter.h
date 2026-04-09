#pragma once

#include "tinyxml2.h"
#include <filesystem>
#include <string>
#include <stdexcept>
#if (DREALTIME)
#include <ecrt.h>
#include "EthercatParameter.h"
#endif
#include "axisParameter.h"

namespace ZrcsSystem {

// 封装 tinyxml2 的辅助类
class XmlHelper {
public:
    XmlHelper(const std::string& filePath) {
        if (doc.LoadFile(filePath.c_str()) != tinyxml2::XML_SUCCESS) {
            throw std::runtime_error("Failed to load XML file: " + filePath);
        }
    }

    tinyxml2::XMLElement* getRootElement(const std::string& rootName) {
        tinyxml2::XMLElement* root = doc.FirstChildElement(rootName.c_str());
        if (!root) {
            throw std::runtime_error("Root element not found: " + rootName);
        }
        return root;
    }

    std::string getElementText(tinyxml2::XMLElement* element, const std::string& childName) {
        tinyxml2::XMLElement* child = element->FirstChildElement(childName.c_str());
        if (!child || !child->GetText()) {
            throw std::runtime_error("Child element not found or empty: " + childName);
        }
        return child->GetText();
    }

private:
    tinyxml2::XMLDocument doc;
};

class Parameter {
public:
    #if (DREALTIME)
    SlaveConfig* slaveConfig;
    #endif
    AxisConfig* axisConfig;

public:
    Parameter() {
        std::string currentExePath = std::filesystem::current_path().string();
        std::string target = "build";
        std::string projectPath;
        // 查找目标字符串 "zrcs" 在 fullPath 中的位置
        size_t found = currentExePath.find(target);

        if (found != std::string::npos) {
            // 截取 "zrcs" 前面的子串
            projectPath = currentExePath.substr(0, found);
        } else {
            throw std::runtime_error("没有找到工程名 zrcs");
        }
        loadConfiguration(projectPath);
    }

    void loadConfiguration(const std::string& projectPath) {
        #if (DREALTIME)
        slaveConfig = new SlaveConfig(projectPath);
        #endif
        axisConfig = new AxisConfig(projectPath);

        // 使用 XmlHelper 加载额外的配置
        try {
            XmlHelper xmlHelper(projectPath + "/config.xml");
            auto root = xmlHelper.getRootElement("Configuration");
            std::string exampleValue = xmlHelper.getElementText(root, "ExampleKey");
            // 使用 exampleValue 进行进一步处理
        } catch (const std::exception& e) {
            throw std::runtime_error("XML configuration error: " + std::string(e.what()));
        }
    }

    ~Parameter() {
        #if (DREALTIME)
        delete slaveConfig;
        #endif
        delete axisConfig;
    }
};

}

