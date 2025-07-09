#ifndef NODE_COMMUNICATION_EXAMPLE_H
#define NODE_COMMUNICATION_EXAMPLE_H

#include "nodeCommunication.h"
#include <iostream>
#include <string>

namespace zrcsSystem {

// 示例数据结构
struct SensorData {
    int sensorId;
    double value;
    std::string timestamp;
};

// 生产者节点
class ProducerNode {
public:
    OutputPort<SensorData> data_out;

    void generateData() {
        SensorData data{1, 99.5, "2023-10-27T10:00:00Z"};
        std::cout << "Producer: Sending data..." << std::endl;
        if (data_out.write(data) != ConnStatus::SUCCESS) {
            std::cerr << "Producer: Failed to write data to port." << std::endl;
        }
    }
};

// 消费者节点
class ConsumerNode {
public:
    InputPort<SensorData> data_in;

    void processData() {
        SensorData data;
        if (data_in.read(data) == ConnStatus::SUCCESS) {
            std::cout << "Consumer: Received data -> ID: " << data.sensorId
                      << ", Value: " << data.value
                      << ", Timestamp: " << data.timestamp << std::endl;
        } else {
            std::cout << "Consumer: No data to read." << std::endl;
        }
    }
};

// 示例用法
void communicationExample() {
    std::cout << "--- Node Communication Example ---" << std::endl;

    ProducerNode producer;
    ConsumerNode consumer;

    // 连接端口
    connect(producer.data_out, consumer.data_in, 10); // 容量为 10

    // 模拟数据流
    consumer.processData(); // 尝试读取，此时应为空
    producer.generateData(); // 生产者发送数据
    consumer.processData(); // 消费者读取数据
    consumer.processData(); // 再次尝试读取，应为空

    std::cout << "------------------------------------" << std::endl;
}

} // namespace zrcsSystem

#endif // NODE_COMMUNICATION_EXAMPLE_H