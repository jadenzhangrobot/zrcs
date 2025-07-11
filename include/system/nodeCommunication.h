#ifndef NODE_COMMUNICATION_H
#define NODE_COMMUNICATION_H

#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include <atomic>
#include <memory_resource>

namespace zrcsSystem {

/**
 * @brief 定义数据流方向
 */
enum class DataFlow {
    INPUT,
    OUTPUT
};

/**
 * @brief 通道连接状态
 */
enum class ConnStatus {
    SUCCESS,
    BUFFER_FULL,
    BUFFER_EMPTY,
    NOT_CONNECTED,
    INVALID_ARGUMENT
};

/**
 * @brief 节点间通信的环形缓冲区通道，为实时性优化。
 * @tparam T 数据类型
 */
template<typename T>
class NodeChannel {
public:
    explicit NodeChannel(size_t capacity)
        : buffer_(capacity),
          resource_(&buffer_[0], capacity * sizeof(T)),
          data_(&resource_) {
        data_.reserve(capacity);
        head_ = 0;
        tail_ = 0;
        count_ = 0;
    }

    bool write(const T& value) {
        if (count_ >= data_.capacity()) {
            return false; // 缓冲区满
        }
        if (data_.size() < data_.capacity()) {
            data_.push_back(value);
        } else {
            data_[tail_] = value;
        }
        tail_ = (tail_ + 1) % data_.capacity();
        count_++;
        return true;
    }

    bool read(T& value) {
        if (count_ == 0) {
            return false; // 缓冲区空
        }
        value = data_[head_];
        head_ = (head_ + 1) % data_.capacity();
        count_--;
        return true;
    }

    bool isFull() const {
        return count_ >= data_.capacity();
    }

    bool isEmpty() const {
        return count_ == 0;
    }

    size_t size() const {
        return count_;
    }

    size_t capacity() const {
        return data_.capacity();
    }

private:
    std::vector<std::byte> buffer_;
    std::pmr::monotonic_buffer_resource resource_;
    std::pmr::vector<T> data_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    std::atomic<size_t> count_;
};

template<typename T>
class OutputPort;

template<typename T>
class InputPort;

// 连接函数
template<typename T>
void connect(OutputPort<T>& output, InputPort<T>& input, size_t capacity = 64);

/**
 * @brief 输出端口，用于发送数据
 * @tparam T 数据类型
 */
template<typename T>
class OutputPort {
public:
    ConnStatus write(const T& data) {
        if (!channel_) {
            return ConnStatus::NOT_CONNECTED;
        }
        if (channel_->write(data)) {
            return ConnStatus::SUCCESS;
        } else {
            return ConnStatus::BUFFER_FULL;
        }
    }

private:
    friend void connect<T>(OutputPort<T>&, InputPort<T>&, size_t);
    std::shared_ptr<NodeChannel<T>> channel_;
};

/**
 * @brief 输入端口，用于接收数据
 * @tparam T 数据类型
 */
template<typename T>
class InputPort {
public:
    ConnStatus read(T& data) {
        if (!channel_) {
            return ConnStatus::NOT_CONNECTED;
        }
        if (channel_->read(data)) {
            return ConnStatus::SUCCESS;
        } else {
            return ConnStatus::BUFFER_EMPTY;
        }
    }

private:
    friend void connect<T>(OutputPort<T>&, InputPort<T>&, size_t);
    std::shared_ptr<NodeChannel<T>> channel_;
};

/**
 * @brief 连接一个输出端口和一个输入端口
 * @tparam T 数据类型
 * @param output 输出端口
 * @param input 输入端口
 * @param capacity 通道容量
 */
template<typename T>
void connect(OutputPort<T>& output, InputPort<T>& input, size_t capacity) {
    
    auto channel = std::make_shared<NodeChannel<T>>(capacity);
    output.channel_ = channel;
    input.channel_ = channel;
}

} // namespace zrcsSystem

#endif // NODE_COMMUNICATION_H