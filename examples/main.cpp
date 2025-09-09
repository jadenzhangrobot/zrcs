#include <iostream>
#include "model/modeinterface.h"
#include "model/CartesianKinematics.h"
#include "model/FiveAxisKinematics.h"
#include "controller/ControllerInterface.h"

// Mock Controller for demonstration
class MockController : public ZrcsHardware::Controller {
public:
    MockController() {
        for (int i = 0; i < 6; ++i) {
            axiss[i] = new MockAxis();
        }
    }
    ~MockController() {
        for (int i = 0; i < 6; ++i) {
            delete axiss[i];
        }
    }

    class MockAxis : public ZrcsHardware::Servo {
    public:
        int32_t p = 0, v = 0, a = 0;
        MC_SERVO_CODE setPos(int32_t pos) override { p = pos; return SERVONOERROR; }
        int32_t pos(void) override { return p; }
        int32_t vel(void) override { return v; }
        int32_t acc(void) override { return a; }
    };
};

int main() {
    // 1. 创建控制器实例
    MockController controller;

    // 2. 创建三轴笛卡尔机器人运动模式
    ModeInterface<3> cartesianMode("CartesianRobot");
    cartesianMode.setController(&controller);
    cartesianMode.setKinematicsModel(std::make_shared<CartesianKinematics>());

    // 3. 添加轴
    cartesianMode.addAxis(0);
    cartesianMode.addAxis(1);
    cartesianMode.addAxis(2);

    // 4. 设置轴的运动限制
    cartesianMode.setAxisLimits(0, 10.0, 5.0, 2.0);
    cartesianMode.setAxisLimits(1, 10.0, 5.0, 2.0);
    cartesianMode.setAxisLimits(2, 10.0, 5.0, 2.0);

    // 5. 初始化并设置目标位置
    cartesianMode.initialize();
    std::vector<double> target_pos_cart = {100.0, 200.0, 50.0};
    cartesianMode.setTargetPositions(target_pos_cart, true); // 使用笛卡尔坐标

    // 6. 模拟运动
    std::cout << "Running Cartesian Robot..." << std::endl;
    cartesianMode.start();
    for (int i = 0; i < 100; ++i) { // 模拟100个周期
        cartesianMode.update();
    }
    std::cout << "Cartesian Robot movement finished." << std::endl;

    // 7. 创建五轴机器人运动模式
    ModeInterface<5> fiveAxisMode("FiveAxisRobot");
    fiveAxisMode.setController(&controller);
    fiveAxisMode.setKinematicsModel(std::make_shared<FiveAxisKinematics>());

    // ... (similar setup for 5-axis robot)

    return 0;
}