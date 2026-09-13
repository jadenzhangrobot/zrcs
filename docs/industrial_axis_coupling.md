# 工业轴绑定/解绑设计方案

## 问题定义

**场景**：
- 初始状态：3个逻辑轴（其中轴1是双驱：Servo2 + Servo3）
- 解除绑定后：4个独立轴

**关键区别**：
- ❌ 错误理解：双驱 = 1个轴对象包含2个伺服
- ✅ 正确理解：双驱 = 2个物理轴通过"耦合"同步为1个逻辑轴

---

## 工业标准方案对比

### 1. Beckhoff TwinCAT - 虚拟轴 + 耦合对象

```
物理层：
  Servo1 → PhysicalAxis1
  Servo2 → PhysicalAxis2  ┐
  Servo3 → PhysicalAxis3  ├─ GantryCoupling
  Servo4 → PhysicalAxis4  ┘

逻辑层：
  VirtualAxis1 (单驱，直接映射到 PhysicalAxis1)
  VirtualAxis2 (双驱门架，由 GantryCoupling 控制 Axis2+3)
  VirtualAxis3 (单驱，直接映射到 PhysicalAxis4)
```

**特点**：
- 所有物理轴始终存在，只是被耦合对象协调
- 解耦 = 删除 GantryCoupling 对象
- 耦合状态下，虚拟轴命令由 Coupling 分发到物理轴
- 解耦后，物理轴可以独立访问

**数据结构**：
```cpp
class PhysicalAxis {
    uint32_t servoId;
    double position;
    double velocity;
    bool isIndependent;  // true=独立控制, false=被耦合控制
};

class GantryCoupling {
    std::vector<PhysicalAxis*> slaves;
    PhysicalAxis* master;
    double syncErrorLimit;
    
    void distribute(double masterCmd) {
        master->setPosition(masterCmd);
        for (auto* slave : slaves) {
            slave->setPosition(masterCmd);  // 同步命令
        }
    }
};

class VirtualAxis {
    PhysicalAxis* physical;       // 单驱模式
    GantryCoupling* coupling;     // 多驱模式
    
    void setPosition(double pos) {
        if (coupling) {
            coupling->distribute(pos);
        } else {
            physical->setPosition(pos);
        }
    }
};
```

---

### 2. Siemens SIMOTION - 主从轴模式

```
初始配置 (3个逻辑轴)：
  Axis[0] = Technology Object "Axis_X"   (Servo1)
  Axis[1] = Technology Object "Axis_Y"   (Servo2, Master)
             └─ SlaveAxis: Axis_Y_Slave  (Servo3, Slave)
  Axis[2] = Technology Object "Axis_Z"   (Servo4)
  
解耦后 (4个逻辑轴)：
  调用 MC_UngearIn(Axis_Y_Slave) → 从轴变为独立轴
  Axis[0] = Axis_X
  Axis[1] = Axis_Y
  Axis[2] = Axis_Y_Slave  (现在独立)
  Axis[3] = Axis_Z
```

**PLCopen 函数**：
```st
(* 绑定轴 *)
MC_GearIn(
    Master := Axis_Y,
    Slave := Axis_Y_Slave,
    RatioNumerator := 1,
    RatioDenominator := 1
);

(* 解绑轴 *)
MC_UngearIn(
    Slave := Axis_Y_Slave
);
```

**特点**：
- 主轴 = 正常的运动轴
- 从轴 = 跟随主轴的电子齿轮轴
- 解耦后从轴恢复为独立控制

---

### 3. Rockwell Logix - 轴组（Motion Group）

```
配置：
  MotionGroup_Main
    ├─ Axis_X (单驱)
    ├─ CoordinatedAxisPair
    │   ├─ Axis_Y1 (主)
    │   └─ Axis_Y2 (从)
    └─ Axis_Z (单驱)

控制器始终看到4个物理轴，但 CoordinatedAxisPair 决定了：
  - 耦合时：Y1/Y2 作为单一逻辑轴响应命令
  - 解耦时：Y1/Y2 可独立寻址
```

**RSLogix 代码**：
```st
(* 激活协调 *)
MAPC(CoordinatedAxisPair);  // Motion Axis Pair Couple

(* 解除协调 *)
MAPD(CoordinatedAxisPair);  // Motion Axis Pair Decouple
```

---

### 4. EtherCAT 底层方案 - 分布式时钟同步

```
所有轴始终独立：
  Servo1 → Axis[0]
  Servo2 → Axis[1]
  Servo3 → Axis[2]
  Servo4 → Axis[3]

"双驱绑定" = 软件层协调：
  - 发送相同的位置命令给 Axis[1] 和 Axis[2]
  - 监控两者位置差
  - EtherCAT DC 保证同步性 (< 1μs)

"解绑" = 停止发送同步命令，独立控制
```

**特点**：
- 最底层、最灵活
- 没有"虚拟轴"概念
- 所有同步逻辑在应用层实现

---

## ZRCS 推荐方案：混合模式

结合 zrcs 的特点（支持仿真+实际硬件），推荐采用 **TwinCAT 模式的简化版**：

### 架构设计

```
┌──────────────────────────────────────────────────────┐
│               Controller (轴管理器)                   │
│  physicalAxes_: vector<unique_ptr<PhysicalAxis>>    │
│  logicalAxes_:  vector<unique_ptr<LogicalAxis>>     │
│  couplings_:    vector<unique_ptr<AxisCoupling>>    │
└──────────────────────────────────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│PhysicalAxis 0│ │PhysicalAxis 1│ │PhysicalAxis 2│
│ Servo1       │ │ Servo2       │ │ Servo3       │
│ independent  │ │ coupled      │ │ coupled      │
└──────────────┘ └──────────────┘ └──────────────┘
                        │               │
                        └───────┬───────┘
                                ▼
                        ┌──────────────┐
                        │GantryCoupling│
                        │ master: Axis1│
                        │ slave: Axis2 │
                        └──────────────┘
                                ▲
                                │
                        ┌──────────────┐
                        │LogicalAxis 1 │  ← 用户看到的"轴1"
                        │(双驱门架)     │
                        └──────────────┘
```

### 核心类定义

```cpp
/**
 * @brief 物理轴 - 始终存在，对应一个伺服驱动器
 */
class PhysicalAxis {
private:
    uint32_t axisId_;
    std::unique_ptr<Servo> servo_;
    AxisPara config_;
    
    // 控制模式
    enum class ControlMode {
        INDEPENDENT,    // 独立控制（默认）
        COUPLED_MASTER, // 耦合主轴
        COUPLED_SLAVE   // 耦合从轴
    };
    ControlMode mode_{ControlMode::INDEPENDENT};
    
    // 运动状态
    double posCmd_{0.0};
    double velCmd_{0.0};
    double posAct_{0.0};
    double velAct_{0.0};
    
public:
    void setPositionCmd(double pos);
    double actualPosition() const;
    
    // 耦合控制
    void setCoupled(bool isMaster);
    void setIndependent();
    bool isIndependent() const { return mode_ == ControlMode::INDEPENDENT; }
};

/**
 * @brief 轴耦合对象 - 协调多个物理轴
 */
class AxisCoupling {
public:
    enum class CouplingType {
        GANTRY,         // 门架：主从相同命令
        GEAR,           // 齿轮：从轴 = 主轴 × 比例
        CAM             // 凸轮：从轴 = f(主轴)
    };
    
private:
    CouplingType type_;
    PhysicalAxis* master_;
    std::vector<PhysicalAxis*> slaves_;
    
    // 门架参数
    double syncErrorLimit_{0.05};
    
    // 齿轮参数
    double gearRatio_{1.0};
    
public:
    AxisCoupling(CouplingType type, PhysicalAxis* master)
        : type_(type), master_(master) {}
    
    void addSlave(PhysicalAxis* slave);
    void removeSlave(PhysicalAxis* slave);
    
    /**
     * @brief 分发主轴命令到从轴
     */
    void distribute(double masterPosCmd, double masterVelCmd);
    
    /**
     * @brief 检查同步误差
     */
    bool checkSyncError();
    
    /**
     * @brief 激活耦合
     */
    void engage();
    
    /**
     * @brief 解除耦合
     */
    void disengage();
};

/**
 * @brief 逻辑轴 - 用户看到的运动轴
 * 
 * 可以是：
 * 1. 单个物理轴的直接映射（单驱）
 * 2. 耦合对象的封装（双驱/多驱）
 */
class LogicalAxis {
private:
    uint32_t logicalId_;
    std::string name_;
    
    // 单驱模式
    PhysicalAxis* singlePhysical_{nullptr};
    
    // 多驱模式
    AxisCoupling* coupling_{nullptr};
    
public:
    LogicalAxis(uint32_t id, PhysicalAxis* physical)
        : logicalId_(id), singlePhysical_(physical) {}
    
    LogicalAxis(uint32_t id, AxisCoupling* coupling)
        : logicalId_(id), coupling_(coupling) {}
    
    /**
     * @brief 设置位置命令（逻辑轴接口）
     */
    void setPositionCmd(double pos) {
        if (coupling_) {
            // 多驱：通过耦合对象分发
            coupling_->distribute(pos, 0.0);
        } else {
            // 单驱：直接控制物理轴
            singlePhysical_->setPositionCmd(pos);
        }
    }
    
    double actualPosition() const {
        if (coupling_) {
            return coupling_->getMasterPosition();
        } else {
            return singlePhysical_->actualPosition();
        }
    }
    
    bool isMultiDrive() const { return coupling_ != nullptr; }
};

/**
 * @brief 控制器 - 管理物理轴、逻辑轴和耦合
 */
class Controller {
private:
    // 物理层：固定数量，由硬件配置决定
    std::vector<std::unique_ptr<PhysicalAxis>> physicalAxes_;
    
    // 逻辑层：动态数量，由耦合状态决定
    std::vector<std::unique_ptr<LogicalAxis>> logicalAxes_;
    
    // 耦合对象
    std::vector<std::unique_ptr<AxisCoupling>> couplings_;
    
public:
    /**
     * @brief 创建门架耦合（双驱绑定）
     * @param masterId 主轴物理ID
     * @param slaveIds 从轴物理ID列表
     * @return 新创建的逻辑轴ID
     */
    uint32_t createGantryCoupling(uint32_t masterId, 
                                   const std::vector<uint32_t>& slaveIds);
    
    /**
     * @brief 解除耦合（解绑）
     * @param logicalId 逻辑轴ID
     * @return 分离出的物理轴ID列表
     */
    std::vector<uint32_t> dissolveCoupling(uint32_t logicalId);
    
    /**
     * @brief 获取逻辑轴（运动命令使用）
     */
    LogicalAxis* getLogicalAxis(uint32_t id);
    
    /**
     * @brief 获取物理轴（诊断使用）
     */
    PhysicalAxis* getPhysicalAxis(uint32_t id);
    
    /**
     * @brief 获取当前逻辑轴数量
     */
    size_t getLogicalAxisCount() const { return logicalAxes_.size(); }
};
```

### 耦合分发实现

```cpp
void AxisCoupling::distribute(double masterPosCmd, double masterVelCmd)
{
    // 主轴接收原始命令
    master_->setPositionCmd(masterPosCmd);
    
    switch (type_) {
        case CouplingType::GANTRY:
            // 门架模式：所有从轴接收相同命令
            for (auto* slave : slaves_) {
                slave->setPositionCmd(masterPosCmd);
            }
            break;
            
        case CouplingType::GEAR:
            // 齿轮模式：从轴 = 主轴 × 比例
            for (auto* slave : slaves_) {
                slave->setPositionCmd(masterPosCmd * gearRatio_);
            }
            break;
            
        case CouplingType::CAM:
            // 凸轮模式：从轴 = f(主轴)
            // 需要凸轮曲线表
            break;
    }
}

bool AxisCoupling::checkSyncError()
{
    if (type_ != CouplingType::GANTRY) {
        return true;  // 非门架模式不检查同步
    }
    
    double masterPos = master_->actualPosition();
    
    for (auto* slave : slaves_) {
        double slavePos = slave->actualPosition();
        double error = std::abs(slavePos - masterPos);
        
        if (error > syncErrorLimit_) {
            ERROR_PRINT("Gantry sync error: %.4f > %.4f\n", 
                       error, syncErrorLimit_);
            return false;
        }
    }
    
    return true;
}
```

### 动态绑定/解绑实现

```cpp
uint32_t Controller::createGantryCoupling(
    uint32_t masterId,
    const std::vector<uint32_t>& slaveIds)
{
    // 1. 验证物理轴存在且独立
    if (masterId >= physicalAxes_.size()) {
        throw std::runtime_error("Invalid master axis ID");
    }
    
    auto* master = physicalAxes_[masterId].get();
    if (!master->isIndependent()) {
        throw std::runtime_error("Master axis is already coupled");
    }
    
    std::vector<PhysicalAxis*> slaves;
    for (auto slaveId : slaveIds) {
        if (slaveId >= physicalAxes_.size()) {
            throw std::runtime_error("Invalid slave axis ID");
        }
        auto* slave = physicalAxes_[slaveId].get();
        if (!slave->isIndependent()) {
            throw std::runtime_error("Slave axis is already coupled");
        }
        slaves.push_back(slave);
    }
    
    // 2. 创建耦合对象
    auto coupling = std::make_unique<AxisCoupling>(
        AxisCoupling::CouplingType::GANTRY, master);
    
    for (auto* slave : slaves) {
        coupling->addSlave(slave);
    }
    
    // 3. 激活耦合
    coupling->engage();
    master->setCoupled(true);
    for (auto* slave : slaves) {
        slave->setCoupled(false);
    }
    
    // 4. 创建逻辑轴
    uint32_t newLogicalId = logicalAxes_.size();
    auto logicalAxis = std::make_unique<LogicalAxis>(
        newLogicalId, coupling.get());
    
    couplings_.push_back(std::move(coupling));
    logicalAxes_.push_back(std::move(logicalAxis));
    
    // 5. 从逻辑轴列表中移除原来的单驱轴
    // （这些物理轴现在被耦合控制）
    removeLogicalAxesForPhysicalIds({masterId, slaveIds});
    
    INFO_PRINT("Created gantry coupling: logical axis %u = physical %u (master) + %zu slaves\n",
               newLogicalId, masterId, slaveIds.size());
    
    return newLogicalId;
}

std::vector<uint32_t> Controller::dissolveCoupling(uint32_t logicalId)
{
    // 1. 查找逻辑轴
    if (logicalId >= logicalAxes_.size()) {
        throw std::runtime_error("Invalid logical axis ID");
    }
    
    auto* logicalAxis = logicalAxes_[logicalId].get();
    if (!logicalAxis->isMultiDrive()) {
        throw std::runtime_error("Logical axis is not multi-drive");
    }
    
    // 2. 获取耦合对象和物理轴
    auto* coupling = logicalAxis->getCoupling();
    auto physicalIds = coupling->getPhysicalAxisIds();
    
    // 3. 解除耦合
    coupling->disengage();
    
    for (auto physId : physicalIds) {
        physicalAxes_[physId]->setIndependent();
    }
    
    // 4. 删除逻辑轴和耦合对象
    logicalAxes_.erase(logicalAxes_.begin() + logicalId);
    couplings_.erase(
        std::remove_if(couplings_.begin(), couplings_.end(),
            [coupling](const auto& c) { return c.get() == coupling; }),
        couplings_.end());
    
    // 5. 为每个物理轴创建新的逻辑轴
    for (auto physId : physicalIds) {
        auto newLogicalAxis = std::make_unique<LogicalAxis>(
            logicalAxes_.size(), physicalAxes_[physId].get());
        logicalAxes_.push_back(std::move(newLogicalAxis));
    }
    
    INFO_PRINT("Dissolved coupling: logical axis %u → %zu independent axes\n",
               logicalId, physicalIds.size());
    
    return physicalIds;
}
```

---

## 使用示例

### 场景：3轴系统，轴1是双驱

```cpp
// ====== 初始配置 ======
Controller controller;

// 创建4个物理轴（对应4个伺服）
controller.addPhysicalAxis(0, servo1);  // X轴
controller.addPhysicalAxis(1, servo2);  // Y1
controller.addPhysicalAxis(2, servo3);  // Y2
controller.addPhysicalAxis(3, servo4);  // Z轴

// 创建门架耦合：Y1(主) + Y2(从)
uint32_t gantryCouplingId = controller.createGantryCoupling(
    1,    // master: 物理轴1 (Y1)
    {2}   // slaves: 物理轴2 (Y2)
);

// 当前逻辑轴：
//   LogicalAxis[0] → PhysicalAxis[0] (X, 单驱)
//   LogicalAxis[1] → GantryCoupling (Y双驱: Physical1+2)
//   LogicalAxis[2] → PhysicalAxis[3] (Z, 单驱)
// 总共 3 个逻辑轴

// ====== 运动控制 ======
auto* yAxis = controller.getLogicalAxis(1);
yAxis->setPositionCmd(100.0);  // Y1和Y2同时移动到100mm

// ====== 解除绑定 ======
std::vector<uint32_t> separatedAxes = controller.dissolveCoupling(1);
// 返回: {1, 2}  (Y1和Y2的物理ID)

// 当前逻辑轴：
//   LogicalAxis[0] → PhysicalAxis[0] (X)
//   LogicalAxis[1] → PhysicalAxis[1] (Y1, 独立)
//   LogicalAxis[2] → PhysicalAxis[2] (Y2, 独立)
//   LogicalAxis[3] → PhysicalAxis[3] (Z)
// 总共 4 个逻辑轴

// ====== 独立控制 ======
auto* y1 = controller.getLogicalAxis(1);
auto* y2 = controller.getLogicalAxis(2);
y1->setPositionCmd(50.0);   // Y1独立移动
y2->setPositionCmd(100.0);  // Y2独立移动
```

---

## XML配置扩展

支持在配置文件中定义初始耦合：

```xml
<!-- axis.xml -->
<axes>
  <!-- 物理轴定义 -->
  <physicalAxis id="0" name="X" servoId="1" lead="0.005"/>
  <physicalAxis id="1" name="Y1" servoId="2" lead="0.005"/>
  <physicalAxis id="2" name="Y2" servoId="3" lead="0.005"/>
  <physicalAxis id="3" name="Z" servoId="4" lead="0.005"/>
  
  <!-- 耦合定义 -->
  <couplings>
    <gantry name="Y_Gantry">
      <master>1</master>
      <slaves>2</slaves>
      <syncErrorLimit>0.05</syncErrorLimit>
    </gantry>
  </couplings>
  
  <!-- 逻辑轴映射（可选，自动生成）-->
  <logicalAxes>
    <axis id="0" type="single" physical="0"/>
    <axis id="1" type="gantry" coupling="Y_Gantry"/>
    <axis id="2" type="single" physical="3"/>
  </logicalAxes>
</axes>
```

---

## 运动学模型适配

解耦后轴数量变化，需要更新运动学：

```cpp
class RobotModel {
public:
    void updateAxisCount(size_t newCount) {
        dof_ = newCount;
        // 重新初始化雅可比矩阵
        jacobian_.resize(6, dof_);
    }
    
    void remapAxes(const std::map<uint32_t, uint32_t>& oldToNew) {
        // 更新轴ID映射
        for (auto& [oldId, newId] : oldToNew) {
            axisIds_[oldId] = newId;
        }
    }
};

// 解耦后更新模型
auto separatedAxes = controller.dissolveCoupling(1);
robotModel.updateAxisCount(controller.getLogicalAxisCount());
```

---

## 对比总结

| 方案 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| **虚拟轴+耦合** | 清晰分层，灵活 | 需要重构 | 通用控制器 |
| **主从轴** | 简单直观 | 主轴必须运动 | 跟随应用 |
| **轴组** | 符合PLCopen | 静态配置 | 标准机器人 |
| **底层同步** | 最灵活 | 逻辑复杂 | 高性能应用 |

**ZRCS推荐**：虚拟轴+耦合（TwinCAT模式）
- 与现有代码兼容性好
- 支持仿真和实际硬件
- 逻辑清晰，易于调试
