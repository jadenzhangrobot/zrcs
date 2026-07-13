#pragma once

#include "shared_memory/ShmLayout.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <thread>

namespace zrcs_nrt {

struct MujocoIdentifyOptions {
    bool enabled = true;
    bool apply = false;
    bool useInverseTorque = false;
    double lambda = 0.998;
    double eps = 0.02;
    double applyPeriodSec = 0.5;
    double bandwidthHz = 8.0;
    double dampingRatio = 0.9;
    std::filesystem::path outputDir = "identify";
};

class MujocoIdentifyWorker {
public:
    MujocoIdentifyWorker(zrcs::SharedBlock* block, MujocoIdentifyOptions options);
    ~MujocoIdentifyWorker();

    MujocoIdentifyWorker(const MujocoIdentifyWorker&) = delete;
    MujocoIdentifyWorker& operator=(const MujocoIdentifyWorker&) = delete;

    void start();
    void stop();
    bool running() const noexcept { return running_.load(std::memory_order_acquire); }

private:
    void run();
    void publishStopped();

    zrcs::SharedBlock* block_;
    MujocoIdentifyOptions options_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

MujocoIdentifyOptions parseMujocoIdentifyOptions(int argc, char** argv);

} // namespace zrcs_nrt
