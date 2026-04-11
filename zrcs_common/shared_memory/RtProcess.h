#pragma once

#include "SharedData.h"
#include <memory>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace ipc = boost::interprocess;

class RTProcess {
public:
    explicit RTProcess(const char* shm_name = zrcs::SHM_NAME);
    ~RTProcess();

    bool initialize();
    SharedBlock* sharedBlock() const;

    RTProcess(const RTProcess&) = delete;
    RTProcess& operator=(const RTProcess&) = delete;

private:
    const char* shm_name_;
    std::unique_ptr<ipc::managed_shared_memory> shm_;
    SharedBlock* shared_block_;
};
