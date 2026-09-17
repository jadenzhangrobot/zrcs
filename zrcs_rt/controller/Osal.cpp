#include "controller/Osal.h"

namespace ZrcsHardware {

// ── Rtos（OS 抽象层）─────────────────────────────────────────────────────────

Rtos::~Rtos() = default;

void Rtos::rtos_task_stop()
{
    rtos_task_join();
}

std::uint64_t Rtos::rtos_timer_read(void)
{
    return 0;
}

} // namespace ZrcsHardware
