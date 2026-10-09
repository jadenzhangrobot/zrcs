#include "controller/mujoco/MujocoBus.h"

#include <stdexcept>

namespace ZrcsHardware {

MujocoBus::MujocoBus(std::shared_ptr<MujocoSimulation> simulation)
    : simulation_(std::move(simulation))
{
    if (!simulation_) {
        throw std::runtime_error("MujocoBus requires a simulation instance");
    }
}

void MujocoBus::send()
{
    // 传输职责：每周期推进仿真一拍。GUI 识别采样 / 参数回灌在节点 MujocoIdentPub 处理。
    simulation_->step();
}

void MujocoBus::receive()
{
    simulation_->receive();
}

} // namespace ZrcsHardware
