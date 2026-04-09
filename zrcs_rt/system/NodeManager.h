/**
 * @copyright 版权所有(c)2024
 * @filename: nodeManager.h
 * @brief: 系统中心控制类，负责任务调度和节点管理
 * @author: 649894200@qq.com
 * @createdate: 2024-01-04
 */
#pragma once

#include "system/base/BaseNodeInterface.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include "NodeCommunication.h"
#include "NodeFactory.h"
#include "model/ModelConfig.h"
#include "model/ModelFactory.h"
#include "controller/HardwareFactory.h"
#include "config/ProjectConfig.h"
#include <array>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
namespace zrcsSystem {

/**
 * @brief 系统中心控制类，负责整个系统的任务调度和节点管理
 */
class NodeManager {
private:
  // 成员变量
  std::string projectName_;                                // 项目配置名称
  std::unique_ptr<RTProcess> rtProcess_;               // 实时进程指针
  std::unique_ptr<ZrcsHardware::Controller> controller_; // 硬件控制器指针
  std::unique_ptr<ModelConfig> modelConfig_;             // 模型配置
  ModelRegistry modelRegistry_;                          // 多模型注册表
  CmdNode* cmdNode_;                             // 当前命令节点指针
  Command cmd_;                                  // 命令对象

  ShmAccessor shm() { return ShmAccessor(rtProcess_->sharedBlock()); }

public:
  // 供外部访问 RTProcess（如 node->registered）
  RTProcess* rtProcess() const { return rtProcess_.get(); }

  NodeManager(const std::string& projectName = "")
              : projectName_(projectName),
                rtProcess_(std::make_unique<RTProcess>()),
                controller_(ZrcsHardware::HardwareFactory::createController(projectName)),
                cmdNode_(nullptr)
  {
  }

  // 禁用拷贝构造函数
  NodeManager(const NodeManager &) = delete;
  // 禁用赋值操作符
  NodeManager &operator=(const NodeManager &) = delete;

  /**
   * @brief 析构函数，清理资源
   */
  ~NodeManager() = default;

  /**
   * @brief 停止实时任务，释放资源
   */
  void stop()
  {
      controller_->rtos_->rtos_task_stop();
  }
  
  void initData()
  {
      shm().axisCount().store(controller_->axiss.size(), std::memory_order_release); 
  }
  
  /**
   * @brief 运行系统主循环
   * 创建实时任务并启动任务调度器
   */
  void run();
};
}
