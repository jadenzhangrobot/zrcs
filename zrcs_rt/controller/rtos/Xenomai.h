#pragma once

#include "controller/Osal.h"
#include "controller/ethercat/EthercatMaster.h"
#include "system/log/RtLog.h"

#include <alchemy/task.h>
#include <alchemy/timer.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

namespace ZrcsHardware {

class xenomai : public Rtos
{
public:
    xenomai() = default;

    ~xenomai() override
    {
        rtos_task_stop();
    }

    void real_task(std::function<void()> strategy) override
    {
        strategy_ = std::move(strategy);
    }

    void rtos_task_create() override
    {
        if (taskCreated_)
        {
            return;
        }

        int err = rt_task_create(&taskDesc_, "zrcs_rt", 0, 99, T_JOINABLE);
        if (err < 0)
        {
            ERROR_PRINT("rt_task_create: %s\n", std::strerror(-err));
            return;
        }
        taskCreated_ = true;

        constexpr int kRtCpuId = 7;
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(kRtCpuId, &mask);
        const int affinityError = rt_task_set_affinity(&taskDesc_, &mask);
        if (affinityError < 0)
        {
            WARN_PRINT("rt_task_set_affinity(cpu=%d): %s\n",
                       kRtCpuId, std::strerror(-affinityError));
        }

        running_.store(true, std::memory_order_release);
        err = rt_task_start(&taskDesc_, &xenomai::taskEntry, this);
        if (err < 0)
        {
            running_.store(false, std::memory_order_release);
            ERROR_PRINT("rt_task_start: %s\n", std::strerror(-err));
            rt_task_delete(&taskDesc_);
            taskCreated_ = false;
            return;
        }
        taskStarted_ = true;
    }

    void rtos_task_stop() override
    {
        running_.store(false, std::memory_order_release);
        if (!taskCreated_)
        {
            return;
        }

        if (taskStarted_)
        {
            // Wake the task if it is blocked in rt_task_wait_period().
            rt_task_unblock(&taskDesc_);
        }
        rtos_task_join();
    }

    void rtos_task_join() override
    {
        if (!taskCreated_)
        {
            return;
        }

        if (!taskStarted_)
        {
            rt_task_delete(&taskDesc_);
            taskCreated_ = false;
            return;
        }

        const int err = rt_task_join(&taskDesc_);
        if (err < 0)
        {
            ERROR_PRINT("rt_task_join: %s\n", std::strerror(-err));
            rt_task_delete(&taskDesc_);
        }
        taskStarted_ = false;
        taskCreated_ = false;
    }

    void rtos_set_periodic(int period) override
    {
        rt_task_set_periodic(&taskDesc_, TM_NOW, period);
    }

    std::uint64_t rtos_timer_read() override
    {
        return rt_timer_read();
    }

private:
    static void taskEntry(void* arg)
    {
        auto* self = static_cast<xenomai*>(arg);
        const int err = rt_task_set_periodic(nullptr, TM_NOW, 1000000);
        if (err < 0)
        {
            ERROR_PRINT("rt_task_set_periodic: %s\n", std::strerror(-err));
            self->running_.store(false, std::memory_order_release);
            return;
        }

        while (self->running_.load(std::memory_order_acquire))
        {
            rt_task_wait_period(nullptr);
            if (!self->running_.load(std::memory_order_acquire))
            {
                break;
            }

            if (self->strategy_ != nullptr)
            {
                // RT 循环内不允许异常处理（realtime 规范 1.1）。
                // 控制链代码必须保证不抛异常；若违背将快速失败（std::terminate）。
                self->strategy_();
            }
        }
    }

    RT_TASK taskDesc_{};
    std::function<void()> strategy_{nullptr};
    std::atomic<bool> running_{false};
    bool taskCreated_{false};
    bool taskStarted_{false};
};

} // namespace ZrcsHardware
