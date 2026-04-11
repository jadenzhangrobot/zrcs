#pragma once

#include "SharedData.h"
#include "ShmConstants.h"
#include <memory>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace ipc = boost::interprocess;

class NRTProcess {
public:
    explicit NRTProcess(const char* shm_name = zrcs::SHM_NAME);
    ~NRTProcess() = default;

    bool initialize();
    SharedBlock* sharedBlock() const;

    NRTProcess(const NRTProcess&) = delete;
    NRTProcess& operator=(const NRTProcess&) = delete;

private:
    const char* shm_name_;
    std::unique_ptr<ipc::managed_shared_memory> shm_;
    bool initialized_;
    SharedBlock* shared_block_;
};
