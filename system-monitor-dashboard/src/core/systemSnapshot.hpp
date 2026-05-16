#pragma once

#include <cstdint>
#include <string>

// ─── Plain data structs ───────────────────────────────────────────────────────

struct MemInfo {
    std::size_t total{0};
    std::size_t free{0};
    std::size_t available{0};
};

struct CpuStat {
    float user{0.0f};
    float nice{0.0f};
    float system{0.0f};
    float idle{0.0f};
    float iowait{0.0f};   // ← IO wait — useful to display
    float other{0.0f};    // ← irq + softirq + steal bundled
};

// Raw tick counts read directly from /proc/stat
struct CpuSample {
    uint64_t user{0};
    uint64_t nice{0};
    uint64_t system{0};
    uint64_t idle{0};
    uint64_t iowait{0};
    uint64_t irq{0};
    uint64_t softirq{0};
    uint64_t steal{0};

    // Total of all fields — used as denominator for percentages
    uint64_t total() const {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
};

// ─── Free functions — pure, testable, no class dependency ────────────────────

CpuSample   readCpuSample();
CpuStat     computeCpuStat(const CpuSample& prev, const CpuSample& current);
MemInfo     getMemInfo();
std::string getOsName();

// ─── SystemSnapshot ──────────────────────────────────────────────────────────

class SystemSnapshot {
public:
    std::string osName;
    MemInfo     memInfo;
    CpuStat     cpuStat;

    SystemSnapshot();

    // Call periodically to refresh all values
    void update();

    void print() const;

private:
    CpuSample mPrevSample;
    CpuSample mCurrentSample;
};
