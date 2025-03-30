#ifndef ZRCS_PARAMETER_H
#define ZRCS_PARAMETER_H

#include "tinyxml2.h"
#include <filesystem>
#include <string>
#if (DREALTIME)
#include <ecrt.h
#include "ethercatParameter.h"
#endif
#include "axisParameter.h"
namespace ZrcsSystem {

class Parameter {
public:
   #if (DREALTIME)
   SlaveConfig* slaveConfig;
   #endif
   AxisConfig* axisConfig;
  Parameter() {
    std::string currentExePath = std::filesystem::current_path().string();
    std::string target = "build";
    std::string projectPath ;
// 查找目标字符串 "zrcs" 在 fullPath 中的位置
    size_t found = currentExePath.find(target);

    if (found != std::string::npos)
    {
    // 截取 "zrcs" 前面的子串
    projectPath  = currentExePath.substr(0, found);
    }
    else {
        throw std::runtime_error("没有找到工程名 zrcs");
    }
    loadConfiguration(projectPath);
  }
  void loadConfiguration(const std::string& projectPath) {
    #if (DREALTIME)
    slaveConfig = new SlaveConfig(projectPath);
    #endif
    axisConfig = new  AxisConfig(projectPath);
  }
  ~Parameter()
  {
    #if (DREALTIME)
    delete slaveConfig;
    #endif
    delete axisConfig;
  }
};
}

#endif // ZRCS_PARAMETER_H