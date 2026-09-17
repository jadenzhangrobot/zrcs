#include "controller/Io.h"

namespace ZrcsHardware {

// ── Io ─────────────────────────────────────────────────────────────────────

Io::~Io() = default;

bool Io::isEmergencyStop()
{
    return false;
}

void Io::setEmergencyStop(bool /*value*/)
{
}

void Io::aoWriteValue(int /*index*/, double /*value*/)
{
}

double Io::aoReadValue(int /*index*/)
{
    return 0;
}

} // namespace ZrcsHardware
