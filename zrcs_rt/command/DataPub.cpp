#include "command/DataPub.h"

void DataPub::init() {}

void DataPub::run()
{
    zrcs::JointPosData pos{};
    const size_t count = controller_->axiss.size();
    for (size_t i = 0; i < count; ++i) {
        pos.pos[i] = controller_->axiss[i]->actualPos();
    }
    zrcs::lfl_write(shm()->axisPositions, pos);
}

REGISTERINPUT(DataPub);
