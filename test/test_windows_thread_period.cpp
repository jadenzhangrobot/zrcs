/**
 * @file test_windows_thread_period.cpp
 * @brief 测量 Windows 下 sleep_until 控制循环的实际周期与抖动。
 *
 * 用途：为仿真/Nativelinux 选型外层唤醒周期提供数据依据。
 *
 * 测量内容（每种目标周期 × 可选配置）：
 *   - 默认 sleep_until（不改定时器分辨率）
 *   - timeBeginPeriod(1) 后 sleep_until
 *   - timeBeginPeriod(1) + 高优先级 + sleep_until
 *
 * 输出：控制台统计 + 可选 CSV（每拍实际间隔）。
 *
 * 编译（MSYS2 UCRT64，在仓库根目录）:
 *   g++ -O2 -std=c++17 -o build/test_windows_thread_period \
 *       test/test_windows_thread_period.cpp -lwinmm
 *
 * 运行:
 *   ./build/test_windows_thread_period
 *   ./build/test_windows_thread_period --duration 3 --csv period_log.csv
 *   ./build/test_windows_thread_period --targets 1,2,5,10
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace {

using Clock = std::chrono::steady_clock;

struct Stats {
    double minMs = 0;
    double maxMs = 0;
    double meanMs = 0;
    double stdMs = 0;
    double p50Ms = 0;
    double p95Ms = 0;
    double p99Ms = 0;
    size_t count = 0;
    size_t overshoot2x = 0;  // 实际间隔 > 2 * target
};

Stats computeStats(std::vector<double>& samplesMs, double targetMs)
{
    Stats s;
    if (samplesMs.empty()) {
        return s;
    }
    s.count = samplesMs.size();
    std::sort(samplesMs.begin(), samplesMs.end());
    s.minMs = samplesMs.front();
    s.maxMs = samplesMs.back();
    const double sum = std::accumulate(samplesMs.begin(), samplesMs.end(), 0.0);
    s.meanMs = sum / static_cast<double>(s.count);
    double var = 0.0;
    for (double v : samplesMs) {
        const double d = v - s.meanMs;
        var += d * d;
        if (v > 2.0 * targetMs) {
            ++s.overshoot2x;
        }
    }
    s.stdMs = std::sqrt(var / static_cast<double>(s.count));
    auto percentile = [&](double p) {
        const double idx = p * static_cast<double>(s.count - 1);
        const size_t lo = static_cast<size_t>(idx);
        const size_t hi = std::min(lo + 1, s.count - 1);
        const double t = idx - static_cast<double>(lo);
        return samplesMs[lo] * (1.0 - t) + samplesMs[hi] * t;
    };
    s.p50Ms = percentile(0.50);
    s.p95Ms = percentile(0.95);
    s.p99Ms = percentile(0.99);
    return s;
}

struct RunConfig {
    int targetMs = 1;
    double durationSec = 2.0;
    bool useTimeBeginPeriod = false;
    bool highPriority = false;
    const char* label = "";
};

std::vector<double> measureSleepUntil(const RunConfig& cfg)
{
    std::vector<double> intervalsMs;
    const auto target = std::chrono::milliseconds(cfg.targetMs);
    const auto duration = std::chrono::duration<double>(cfg.durationSec);
    const size_t expect =
        static_cast<size_t>(cfg.durationSec * 1000.0 / std::max(cfg.targetMs, 1)) + 8;
    intervalsMs.reserve(expect);

#ifdef _WIN32
    if (cfg.useTimeBeginPeriod) {
        timeBeginPeriod(1);
    }
    HANDLE thr = GetCurrentThread();
    int oldPrio = GetThreadPriority(thr);
    if (cfg.highPriority) {
        SetThreadPriority(thr, THREAD_PRIORITY_TIME_CRITICAL);
    }
#endif

    // 热身：丢掉前几次，避免冷启动干扰
    {
        auto t = Clock::now();
        for (int i = 0; i < 5; ++i) {
            t += target;
            std::this_thread::sleep_until(t);
        }
    }

    // 从“下一拍”开始，避免对过去时刻 sleep_until 立即返回产生 0ms 假样本
    auto next = Clock::now() + target;
    auto prev = Clock::now();
    const auto end = prev + std::chrono::duration_cast<Clock::duration>(
                                std::chrono::duration<double>(cfg.durationSec));

    while (Clock::now() < end) {
        std::this_thread::sleep_until(next);
        const auto now = Clock::now();
        const double dtMs =
            std::chrono::duration<double, std::milli>(now - prev).count();
        // 丢掉明显是“立刻返回”的样本（落后追赶），只统计真实唤醒间隔
        if (dtMs >= 0.05) {
            intervalsMs.push_back(dtMs);
            prev = now;
        }
        next += target;
        // 严重落后时对齐到未来一拍，避免连续 0 延迟空转
        if (next + target < now) {
            next = now + target;
        }
    }

#ifdef _WIN32
    if (cfg.highPriority) {
        SetThreadPriority(thr, oldPrio);
    }
    if (cfg.useTimeBeginPeriod) {
        timeEndPeriod(1);
    }
#else
    (void)cfg;
#endif

    return intervalsMs;
}

void printStats(const RunConfig& cfg, Stats s)
{
    const double target = static_cast<double>(cfg.targetMs);
    std::printf(
        "\n=== %s | target=%d ms | samples=%zu ===\n",
        cfg.label, cfg.targetMs, s.count);
    std::printf(
        "  min=%.3f  mean=%.3f  p50=%.3f  p95=%.3f  p99=%.3f  max=%.3f  std=%.3f  (ms)\n",
        s.minMs, s.meanMs, s.p50Ms, s.p95Ms, s.p99Ms, s.maxMs, s.stdMs);
    std::printf(
        "  mean/target=%.2f  overshoot>2x=%zu (%.1f%%)\n",
        s.meanMs / target,
        s.overshoot2x,
        s.count ? 100.0 * static_cast<double>(s.overshoot2x) / static_cast<double>(s.count)
                : 0.0);

    // 简单结论标签
    const double err = std::abs(s.meanMs - target) / target;
    const double jitter = s.stdMs / target;
    const char* grade = "差";
    if (err < 0.15 && jitter < 0.20 && s.p99Ms < target * 2.0) {
        grade = "可用";
    }
    if (err < 0.08 && jitter < 0.10 && s.p99Ms < target * 1.5) {
        grade = "较好";
    }
    if (err < 0.05 && jitter < 0.05 && s.p99Ms < target * 1.3) {
        grade = "稳定";
    }
    std::printf("  判定: %s  (mean误差=%.1f%%, 抖动std/target=%.1f%%, p99=%.2f ms)\n",
                grade, err * 100.0, jitter * 100.0, s.p99Ms);
}

void appendCsv(FILE* fp, const RunConfig& cfg, const std::vector<double>& samples)
{
    if (!fp) {
        return;
    }
    for (size_t i = 0; i < samples.size(); ++i) {
        std::fprintf(fp, "%s,%d,%zu,%.6f\n",
                     cfg.label, cfg.targetMs, i, samples[i]);
    }
}

void printTimerResolution()
{
#ifdef _WIN32
    TIMECAPS caps{};
    if (timeGetDevCaps(&caps, sizeof(caps)) == TIMERR_NOERROR) {
        std::printf("Windows multimedia timer: min=%u ms, max=%u ms\n",
                    caps.wPeriodMin, caps.wPeriodMax);
    }
    // 查询当前分辨率（Approximate）：用两次 timeGetTime 的最小步进
    DWORD t0 = timeGetTime();
    DWORD t1 = t0;
    while (t1 == t0) {
        t1 = timeGetTime();
    }
    std::printf("timeGetTime() observed step ~ %lu ms (before timeBeginPeriod)\n",
                static_cast<unsigned long>(t1 - t0));
#else
    std::printf("非 Windows 平台：结果仅供参考（仍测 sleep_until）。\n");
#endif
}

std::vector<int> parseTargets(const char* s)
{
    std::vector<int> out;
    if (!s || !*s) {
        return {1, 2, 5, 10, 15, 16};
    }
    const char* p = s;
    while (*p) {
        char* end = nullptr;
        const long v = std::strtol(p, &end, 10);
        if (end == p) {
            break;
        }
        if (v > 0 && v <= 1000) {
            out.push_back(static_cast<int>(v));
        }
        p = end;
        if (*p == ',' || *p == ';') {
            ++p;
        }
    }
    if (out.empty()) {
        out = {1, 2, 5, 10, 15, 16};
    }
    return out;
}

void usage(const char* argv0)
{
    std::printf(
        "Usage: %s [options]\n"
        "  --duration SEC     每种配置测量时长，默认 2.0\n"
        "  --targets LIST     目标周期 ms 列表，默认 1,2,5,10,15,16\n"
        "  --csv PATH         写出每拍间隔 CSV (label,target_ms,i,dt_ms)\n"
        "  --quick            仅测 1/5/10 ms，时长 1s\n"
        "  --help\n",
        argv0);
}

} // namespace

int main(int argc, char** argv)
{
    double durationSec = 2.0;
    std::vector<int> targets = {1, 2, 5, 10, 15, 16};
    const char* csvPath = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (std::strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            durationSec = std::atof(argv[++i]);
            continue;
        }
        if (std::strcmp(argv[i], "--targets") == 0 && i + 1 < argc) {
            targets = parseTargets(argv[++i]);
            continue;
        }
        if (std::strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            csvPath = argv[++i];
            continue;
        }
        if (std::strcmp(argv[i], "--quick") == 0) {
            targets = {1, 5, 10};
            durationSec = 1.0;
            continue;
        }
        std::fprintf(stderr, "Unknown arg: %s\n", argv[i]);
        usage(argv[0]);
        return 1;
    }

    if (durationSec < 0.2) {
        durationSec = 0.2;
    }

    std::printf("============================================================\n");
    std::printf(" Windows 线程周期测量 (std::this_thread::sleep_until)\n");
    std::printf("============================================================\n");
    printTimerResolution();
    std::printf("duration=%.2f s / target set:", durationSec);
    for (int t : targets) {
        std::printf(" %d", t);
    }
    std::printf(" ms\n");

    FILE* csv = nullptr;
    if (csvPath) {
        csv = std::fopen(csvPath, "w");
        if (!csv) {
            std::fprintf(stderr, "无法创建 CSV: %s\n", csvPath);
            return 1;
        }
        std::fprintf(csv, "label,target_ms,index,dt_ms\n");
    }

    struct Mode {
        const char* label;
        bool beginPeriod;
        bool highPrio;
    };
    const Mode modes[] = {
        {"default_sleep", false, false},
#ifdef _WIN32
        {"timeBeginPeriod1", true, false},
        {"beginPeriod1+TIME_CRITICAL", true, true},
#endif
    };

    std::printf(
        "\n说明:\n"
        "  - mean 接近 target 且 std 小 → 该周期适合做外层唤醒\n"
        "  - target=1 时 mean 常变成 ~15ms 或 ~1-2ms(开 timeBeginPeriod) 且抖动大\n"
        "  - 为 Nativelinux 选型：优先 mean≈target、p99 < 1.5*target 的最小周期\n");

    for (int targetMs : targets) {
        for (const Mode& mode : modes) {
            RunConfig cfg;
            cfg.targetMs = targetMs;
            cfg.durationSec = durationSec;
            cfg.useTimeBeginPeriod = mode.beginPeriod;
            cfg.highPriority = mode.highPrio;
            cfg.label = mode.label;

            auto samples = measureSleepUntil(cfg);
            auto samplesCopy = samples;  // computeStats 会排序
            Stats st = computeStats(samplesCopy, static_cast<double>(targetMs));
            printStats(cfg, st);
            appendCsv(csv, cfg, samples);
        }
    }

    if (csv) {
        std::fclose(csv);
        std::printf("\nCSV written: %s\n", csvPath);
    }

    std::printf(
        "\n------------------------------------------------------------\n"
        "建议解读:\n"
        "  1) 看 default_sleep 下哪个 target 的判定为「较好/稳定」\n"
        "  2) 若仅 timeBeginPeriod1 后 1ms 才「可用」，仍不建议当真 1kHz 控制\n"
        "  3) 外层唤醒可选: 判定稳定的最小 target；逻辑规划可仍用 1ms 多步\n"
        "------------------------------------------------------------\n");
    return 0;
}
