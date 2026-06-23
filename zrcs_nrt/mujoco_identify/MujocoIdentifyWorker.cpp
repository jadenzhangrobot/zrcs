#include "mujoco_identify/MujocoIdentifyWorker.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>

#include <spdlog/spdlog.h>

namespace zrcs_nrt {
namespace {

struct AxisEstimator {
    bool initialized = false;
    uint32_t axisId = 0;
    uint32_t slaveId = 0;
    std::array<double, 4> theta{0.02, 0.1, 0.0, 0.0};
    double p[4][4] = {};
    uint64_t sampleCount = 0;
    uint64_t acceptedCount = 0;
    uint64_t rejectedCount = 0;
    double squaredErrorSum = 0.0;
    double rmsError = 0.0;
    double lastApplyTime = -1.0;

    void init(uint32_t axis, uint32_t slave)
    {
        initialized = true;
        axisId = axis;
        slaveId = slave;
        theta = {0.02, 0.1, 0.0, 0.0};
        for (auto& row : p) {
            for (double& value : row) {
                value = 0.0;
            }
        }
        for (int i = 0; i < 4; ++i) {
            p[i][i] = 1000.0;
        }
    }

    bool update(double ddq, double dq, double tau, bool saturated,
                bool enabled, double lambda, double eps)
    {
        ++sampleCount;
        if (!enabled || saturated || !std::isfinite(ddq) || !std::isfinite(dq) ||
            !std::isfinite(tau) || std::abs(dq) < 1e-4 ||
            (std::abs(ddq) < 1e-3 && std::abs(dq) < 1e-3)) {
            ++rejectedCount;
            return false;
        }

        const std::array<double, 4> phi = {
            ddq,
            dq,
            std::tanh(dq / eps),
            1.0
        };

        double pPhi[4] = {};
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                pPhi[r] += p[r][c] * phi[c];
            }
        }

        double denom = lambda;
        for (int i = 0; i < 4; ++i) {
            denom += phi[i] * pPhi[i];
        }
        if (denom <= 1e-12 || !std::isfinite(denom)) {
            ++rejectedCount;
            return false;
        }

        std::array<double, 4> gain{};
        for (int i = 0; i < 4; ++i) {
            gain[i] = pPhi[i] / denom;
        }

        double predicted = 0.0;
        for (int i = 0; i < 4; ++i) {
            predicted += theta[i] * phi[i];
        }
        const double error = tau - predicted;
        if (!std::isfinite(error)) {
            ++rejectedCount;
            return false;
        }

        for (int i = 0; i < 4; ++i) {
            theta[i] += gain[i] * error;
        }

        double phiP[4] = {};
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                phiP[c] += phi[r] * p[r][c];
            }
        }
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                p[r][c] = (p[r][c] - gain[r] * phiP[c]) / lambda;
            }
        }

        theta[0] = std::clamp(theta[0], 1e-5, 1000.0);
        theta[1] = std::clamp(theta[1], 0.0, 10000.0);
        theta[2] = std::clamp(theta[2], -10000.0, 10000.0);
        theta[3] = std::clamp(theta[3], -10000.0, 10000.0);

        ++acceptedCount;
        squaredErrorSum += error * error;
        rmsError = std::sqrt(squaredErrorSum / static_cast<double>(acceptedCount));
        return true;
    }

    zrcs::MujocoParamUpdate makeUpdate(uint64_t seq,
                                       const MujocoIdentifyOptions& options) const
    {
        zrcs::MujocoParamUpdate update{};
        update.seq = seq;
        update.axisId = axisId;
        update.slaveId = slaveId;
        update.inertia = theta[0];
        update.damping = theta[1];
        update.friction = std::abs(theta[2]);
        update.bias = theta[3];

        constexpr double pi = 3.14159265358979323846;
        const double omega = 2.0 * pi * options.bandwidthHz;
        update.kp = std::clamp(theta[0] * omega * omega, 1.0, 50000.0);
        update.kv = std::clamp(2.0 * options.dampingRatio * theta[0] * omega,
                               0.0, 10000.0);
        update.bandwidthHz = options.bandwidthHz;
        update.dampingRatio = options.dampingRatio;
        update.applyArmature = 0;
        update.applyDamping = 1;
        update.applyFriction = 1;
        update.applyGains = 1;
        update.valid = acceptedCount > 200 ? 1 : 0;
        return update;
    }
};

class CsvWriter {
public:
    explicit CsvWriter(const std::filesystem::path& dir)
    {
        std::filesystem::create_directories(dir);
        file_.open(dir / "mujoco_ident_samples.csv", std::ios::out | std::ios::trunc);
        file_ << "sampleSeq,timeSec,sessionId,index,axisId,slaveId,"
              << "qCmd,dqCmd,ddqCmd,q,dq,ddq,tauApplied,tauInverse,ctrl,"
              << "saturation,enabled,J,B,Fc,bias,rmsError\n";
    }

    void write(const zrcs::MujocoIdentSampleData& sample,
               const std::array<AxisEstimator, zrcs::kAxisMax>& estimators)
    {
        if (!file_) {
            return;
        }
        file_ << std::setprecision(12);
        for (uint32_t i = 0; i < sample.sampleCount && i < zrcs::kAxisMax; ++i) {
            const auto axis = sample.axisId[i];
            const AxisEstimator* est = axis < zrcs::kAxisMax &&
                                       estimators[axis].initialized
                                           ? &estimators[axis]
                                           : nullptr;
            file_ << sample.seq << ','
                  << sample.timestampSec << ','
                  << sample.sessionId << ','
                  << i << ','
                  << axis << ','
                  << sample.slaveId[i] << ','
                  << sample.qCmd[i] << ','
                  << sample.dqCmd[i] << ','
                  << sample.ddqCmd[i] << ','
                  << sample.q[i] << ','
                  << sample.dq[i] << ','
                  << sample.ddq[i] << ','
                  << sample.tauApplied[i] << ','
                  << sample.tauInverse[i] << ','
                  << sample.ctrl[i] << ','
                  << static_cast<int>(sample.saturation[i]) << ','
                  << static_cast<int>(sample.enabled[i]) << ',';
            if (est) {
                file_ << est->theta[0] << ','
                      << est->theta[1] << ','
                      << est->theta[2] << ','
                      << est->theta[3] << ','
                      << est->rmsError;
            } else {
                file_ << "0,0,0,0,0";
            }
            file_ << '\n';
        }
    }

    void flush()
    {
        if (file_) {
            file_.flush();
        }
    }

private:
    std::ofstream file_;
};

void publishStatus(zrcs::SharedBlock* block,
                   const std::array<AxisEstimator, zrcs::kAxisMax>& estimators,
                   uint32_t sessionId,
                   bool applying)
{
    zrcs::MujocoIdentStatusData status{};
    zrcs::lfl_read(block->mujocoIdentStatus, status);
    status.seq += 1;
    status.sessionId = sessionId;
    status.active = 1;
    status.applying = applying ? 1 : 0;
    status.sampleCount = 0;
    status.acceptedCount = 0;
    status.rejectedCount = 0;
    for (size_t i = 0; i < estimators.size(); ++i) {
        const auto& est = estimators[i];
        if (!est.initialized) {
            continue;
        }
        status.activeAxisId = est.axisId;
        status.sampleCount += est.sampleCount;
        status.acceptedCount += est.acceptedCount;
        status.rejectedCount += est.rejectedCount;
        status.inertia[i] = est.theta[0];
        status.damping[i] = est.theta[1];
        status.friction[i] = est.theta[2];
        status.bias[i] = est.theta[3];
        status.rmsError[i] = est.rmsError;
    }
    zrcs::lfl_write(block->mujocoIdentStatus, status);
}

double parseDoubleArg(int& i, int argc, char** argv, double fallback)
{
    if (i + 1 >= argc) {
        return fallback;
    }
    try {
        return std::stod(argv[++i]);
    } catch (...) {
        return fallback;
    }
}

} // namespace

MujocoIdentifyOptions parseMujocoIdentifyOptions(int argc, char** argv)
{
    MujocoIdentifyOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--mujoco-ident-disable") {
            options.enabled = false;
        } else if (arg == "--mujoco-ident-apply") {
            options.apply = true;
        } else if (arg == "--mujoco-ident-use-inverse") {
            options.useInverseTorque = true;
        } else if (arg == "--mujoco-ident-lambda") {
            options.lambda = parseDoubleArg(i, argc, argv, options.lambda);
        } else if (arg == "--mujoco-ident-eps") {
            options.eps = parseDoubleArg(i, argc, argv, options.eps);
        } else if (arg == "--mujoco-ident-bandwidth") {
            options.bandwidthHz = parseDoubleArg(i, argc, argv, options.bandwidthHz);
        } else if (arg == "--mujoco-ident-zeta") {
            options.dampingRatio = parseDoubleArg(i, argc, argv, options.dampingRatio);
        } else if (arg == "--mujoco-ident-out" && i + 1 < argc) {
            options.outputDir = argv[++i];
        }
    }

    options.lambda = std::clamp(options.lambda, 0.90, 0.9999);
    options.eps = std::max(options.eps, 1e-4);
    options.bandwidthHz = std::clamp(options.bandwidthHz, 0.5, 50.0);
    options.dampingRatio = std::clamp(options.dampingRatio, 0.3, 2.0);
    return options;
}

MujocoIdentifyWorker::MujocoIdentifyWorker(zrcs::SharedBlock* block,
                                           MujocoIdentifyOptions options)
    : block_(block), options_(std::move(options))
{
}

MujocoIdentifyWorker::~MujocoIdentifyWorker()
{
    stop();
}

void MujocoIdentifyWorker::start()
{
    if (!block_ || !options_.enabled || running_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    thread_ = std::thread(&MujocoIdentifyWorker::run, this);
}

void MujocoIdentifyWorker::stop()
{
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

void MujocoIdentifyWorker::run()
{
    zrcs::ShmSPSCConsumer<zrcs::MujocoIdentSampleData, zrcs::kMujocoIdentQueueCap>
        sampleConsumer(block_->mujocoIdentSampleQueue);
    zrcs::ShmSPSCProducer<zrcs::MujocoParamUpdate, zrcs::kMujocoParamQueueCap>
        updateProducer(block_->mujocoParamUpdateQueue);

    CsvWriter csv(options_.outputDir);
    std::array<AxisEstimator, zrcs::kAxisMax> estimators{};
    uint64_t updateSeq = 0;
    uint64_t loopCount = 0;
    uint32_t currentSession = 0;

    spdlog::info("[MujocoIdentify] started, apply={}, torque={}, out={}",
                 options_.apply,
                 options_.useInverseTorque ? "inverse" : "applied",
                 options_.outputDir.string());

    while (running_.load(std::memory_order_acquire)) {
        zrcs::MujocoIdentSampleData sample{};
        bool got = false;
        while (sampleConsumer.pop(sample)) {
            got = true;
            currentSession = sample.sessionId;
            for (uint32_t i = 0; i < sample.sampleCount && i < zrcs::kAxisMax; ++i) {
                const uint32_t axis = sample.axisId[i];
                if (axis >= zrcs::kAxisMax) {
                    continue;
                }
                auto& est = estimators[axis];
                if (!est.initialized) {
                    est.init(axis, sample.slaveId[i]);
                }
                const double tau = options_.useInverseTorque
                                       ? sample.tauInverse[i]
                                       : sample.tauApplied[i];
                est.update(sample.ddq[i], sample.dq[i], tau,
                           sample.saturation[i] != 0,
                           sample.enabled[i] != 0,
                           options_.lambda, options_.eps);

                if (options_.apply && est.acceptedCount > 200 &&
                    (est.lastApplyTime < 0.0 ||
                     sample.timestampSec - est.lastApplyTime >= options_.applyPeriodSec)) {
                    const auto update = est.makeUpdate(++updateSeq, options_);
                    if (update.valid && updateProducer.push(update)) {
                        est.lastApplyTime = sample.timestampSec;
                    }
                }
            }
            csv.write(sample, estimators);
        }

        if ((loopCount++ % 100) == 0) {
            publishStatus(block_, estimators, currentSession, options_.apply);
            csv.flush();
        }

        if (!got) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    csv.flush();
    publishStopped();
    spdlog::info("[MujocoIdentify] stopped");
}

void MujocoIdentifyWorker::publishStopped()
{
    if (!block_) {
        return;
    }
    zrcs::MujocoIdentStatusData status{};
    zrcs::lfl_read(block_->mujocoIdentStatus, status);
    status.seq += 1;
    status.active = 0;
    zrcs::lfl_write(block_->mujocoIdentStatus, status);
}

} // namespace zrcs_nrt
