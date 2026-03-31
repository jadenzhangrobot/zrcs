/*
 * @Description: Nativelinux rtos class method implementations
 */
#include "controller/rtos/linux.h"
#include "system/rtLog.h"

namespace ZrcsHardware {

void Nativelinux::rtos_task_create(void)
{
    my_thread=std::thread([this](void)
    {
        using clock = std::chrono::high_resolution_clock;
        const std::chrono::milliseconds target_period(10); // 目标周期10ms
        auto next_wake_time = clock::now();
        while(running_.load(std::memory_order_relaxed))
        {
            next_wake_time += target_period;
            try {
                if (strategy_!=nullptr) {
                      strategy_();
                }
            } catch (const std::exception& e) {
                ERROR_PRINT("RT 实时循环异常: %s\n", e.what());
                break;
            }
              auto sleep_until = next_wake_time;
          std::this_thread::sleep_until(sleep_until);
        }
    });
}

void Nativelinux::rtos_task_stop()
{
    running_.store(false, std::memory_order_release);
    if (my_thread.joinable()) {
        my_thread.join();
    }
}

} // namespace ZrcsHardware
