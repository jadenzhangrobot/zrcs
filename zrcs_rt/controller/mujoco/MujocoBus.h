#pragma once

#include "controller/mujoco/MujocoSimulation.h"

#include <memory>

namespace ZrcsHardware {

/**
 * @brief MuJoCo 仿真总线 — 只负责推进仿真（step/receive），等价于 EtherCAT 的帧收发。
 *
 * 仿真侧 GUI 识别通道（采样发布 / 参数回灌）由独立节点 MujocoIdentPub 负责，
 * 不在传输层触及共享内存，保证「传输只管帧交换」的边界清晰。
 */
class MujocoBus {
public:
    explicit MujocoBus(std::shared_ptr<MujocoSimulation> simulation);

    void send();
    void receive();

    /// 供节点（MujocoIdentPub 等）访问底层仿真对象。
    std::shared_ptr<MujocoSimulation> simulation() const { return simulation_; }

private:
    std::shared_ptr<MujocoSimulation> simulation_;
};

} // namespace ZrcsHardware
