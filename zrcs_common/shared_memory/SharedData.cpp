#include "SharedData.h"

ShmAccessor::ShmAccessor(SharedBlock* blk) : blk_(blk) {}

std::atomic<TaskScheduling>& ShmAccessor::taskScheduling() { return blk_->cmd; }

SPSCRingBuffer<Command, COMMAND_BUFFER_SIZE>& ShmAccessor::cmdQueue() { return blk_->commandQueue; }

SPSCRingBuffer<std::array<double, AxisMaxCount>, STATUS_BUFFER_SIZE>& ShmAccessor::statusQueue() { return blk_->statusQueue; }

SPSCRingBuffer<RtLogEntry, LOG_BUFFER_SIZE>& ShmAccessor::logQueue() { return blk_->logQueue; }

std::atomic<uint64_t>& ShmAccessor::heartBeat() { return blk_->heartBeat; }

std::atomic<uint8_t>& ShmAccessor::axisCount() { return blk_->axisCount; }

std::atomic<uint8_t>& ShmAccessor::multiPlied() { return blk_->Multiplied; }

singleAxisContinueMotion& ShmAccessor::continueMotion() { return blk_->sacm; }

std::atomic<uint32_t>& ShmAccessor::lastCmdSeq() { return blk_->lastCmdSeq; }

std::atomic<uint8_t>& ShmAccessor::lastCmdResult() { return blk_->lastCmdResult; }

std::atomic<double>& ShmAccessor::overrideRatio() { return blk_->overrideRatio; }

double* ShmAccessor::fkResult() { return blk_->fkResult; }

double* ShmAccessor::jointPosResult() { return blk_->jointPosResult; }

std::atomic<uint32_t>& ShmAccessor::ioReadResult() { return blk_->ioReadResult; }

double* ShmAccessor::probeResult() { return blk_->probeResult; }

std::atomic<bool>& ShmAccessor::probeTriggered() { return blk_->probeTriggered; }

double* ShmAccessor::capturedPos() { return blk_->capturedPos; }

std::atomic<bool>& ShmAccessor::captureTriggered() { return blk_->captureTriggered; }

std::atomic<bool>& ShmAccessor::confJEnabled() { return blk_->confJEnabled; }

std::atomic<bool>& ShmAccessor::confLEnabled() { return blk_->confLEnabled; }

std::atomic<uint8_t>& ShmAccessor::singAreaMode() { return blk_->singAreaMode; }
