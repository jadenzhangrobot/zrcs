/*
 * @Description: 非实时平台控制循环（Windows / 普通 Linux）
 *
 * simulation 模式使用 20 ms 墙钟批次，每批推进多个 cycletime 逻辑步；
 * 每个逻辑步都是一次完整控制拍和一次 MuJoCo step。这样不依赖普通
 * Windows/Linux 稳定提供 1 ms 调度，同时保持 1 ms 仿真时间分辨率。
 *
 * standard 模式仍保持墙钟周期与逻辑步长 1:1，落后时有限追赶。
 *
 * 仅用标准 C++ chrono，不调用平台原生定时器 / 优先级 API。
 */
#include "controller/rtos/Linux.h"
#include "system/log/RtLog.h"

#include <algorithm>
#include <chrono>

namespace ZrcsHardware {
namespace {

// 单次唤醒内最多追赶的逻辑拍数，避免异常卡顿后长时间空转占满 CPU。
constexpr int kMaxCatchUpSteps = 50;
constexpr int kSimulationWallBatchMs = 20;

} // namespace

void Nativelinux::rtos_task_create(void)
{
    my_thread = std::thread([this](void) {
        using clock = std::chrono::steady_clock;

        const int logicMs = (static_cast<int>(cycletime) > 0) ? static_cast<int>(cycletime) : 1;

        const auto runSteps = [this](int count) -> bool {
            for (int i = 0; i < count; ++i) {
                if (!running_.load(std::memory_order_relaxed)) {
                    return false;
                }
                if (strategy_ == nullptr) {
                    continue;
                }
                try {
                    strategy_();
                } catch (const std::exception& e) {
                    ERROR_PRINT("RT 实时循环异常: %s\n", e.what());
                    running_.store(false, std::memory_order_release);
                    return false;
                }
            }
            return true;
        };

#ifdef SIMULATION
        const auto wallPeriod = std::chrono::milliseconds(kSimulationWallBatchMs);
        const int stepsPerWake = std::max(1, kSimulationWallBatchMs / logicMs);

        INFO_PRINT(
            "Nativelinux simulation: wall=%dms, logic=%dms, stepsPerWake=%d\n",
            kSimulationWallBatchMs, logicMs, stepsPerWake);

        auto nextWake = clock::now() + wallPeriod;
        while (running_.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_until(nextWake);
            if (!runSteps(stepsPerWake)) {
                break;
            }

            nextWake += wallPeriod;
            const auto now = clock::now();
            if (nextWake < now) {
                nextWake = now + wallPeriod;
            }
        }
#else
        const auto wallPeriod = std::chrono::milliseconds(logicMs);

        INFO_PRINT(
            "Nativelinux: wall=logic=%dms (1:1 Ruckig update per wake, maxCatchUp=%d)\n",
            logicMs, kMaxCatchUpSteps);

        auto nextWake = clock::now() + wallPeriod;

        while (running_.load(std::memory_order_relaxed)) {
            // —— 准时睡到下一逻辑周期边界 ——
            if (clock::now() < nextWake) {
                std::this_thread::sleep_until(nextWake);
            }

            // —— 本拍至少跑 1 步；若已落后则有限追赶，使规划时间追上墙钟 ——
            int stepsThisWake = 1;
            {
                const auto now = clock::now();
                if (now > nextWake) {
                    const auto lag = now - nextWake;
                    const auto lagMs =
                        std::chrono::duration_cast<std::chrono::milliseconds>(lag).count();
                    // 已过 nextWake 的整拍数 + 当前这一拍
                    const int overdue =
                        static_cast<int>(lagMs / logicMs) + 1;
                    stepsThisWake = std::clamp(overdue, 1, kMaxCatchUpSteps);
                }
            }

            if (!runSteps(stepsThisWake)) {
                break;
            }

            // 按实际推进的逻辑拍数推进下一唤醒点
            nextWake += wallPeriod * stepsThisWake;

            // 严重落后：对齐到“现在 + 一拍”，避免 sleep 目标落在过去导致忙等
            const auto now = clock::now();
            if (nextWake + wallPeriod * kMaxCatchUpSteps < now) {
                nextWake = now + wallPeriod;
            }
        }
#endif
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
