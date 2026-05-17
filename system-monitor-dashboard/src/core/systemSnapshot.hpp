#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

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
    float iowait{0.0f};     // time waiting for IO — useful to display
    float other{0.0f};      // irq + softirq + steal bundled
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

// Raw tick counts read from /proc/[pid]/stat for one process.
// utime + stime are the CPU ticks consumed by this process.
struct PidSample {
    uint64_t    utime{0};       // user mode ticks
    uint64_t    stime{0};       // kernel mode ticks
    std::string name;           // process name from /proc/[pid]/stat comm field
    std::size_t memoryKb{0};    // RSS memory in kB from /proc/[pid]/status
};

// Display-ready process info — computed from two PidSamples
struct ProcessInfo {
    pid_t       pid{0};
    std::string name;
    float       cpuPercent{0.0f};
    std::size_t memoryKb{0};
};

// ─── Free functions — pure, testable, no class dependency ────────────────────

CpuSample   readCpuSample();
CpuStat     computeCpuStat(const CpuSample& prev, const CpuSample& current);
MemInfo     getMemInfo();
std::string getOsName();

// Reads /proc/[pid]/stat and /proc/[pid]/status for one process.
// Returns a default-constructed PidSample if the process no longer exists.
PidSample readPidSample(pid_t pid);

// Iterates /proc, reads all numeric directories (each is a PID).
// Returns a map of pid → raw sample, ready for delta computation.
std::unordered_map<pid_t, PidSample> readAllPidSamples();

// Computes CPU% for one process given two consecutive raw samples.
// totalSystemDeltaTicks is the denominator — total system ticks elapsed
// between the two samples (from CpuSample::total() delta).
float computePidCpuPercent(const PidSample& prev,
                           const PidSample& current,
                           uint64_t         totalSystemDeltaTicks);

// ─── SystemSnapshot ──────────────────────────────────────────────────────────

class SystemSnapshot {
public:
    std::string              osName;
    MemInfo                  memInfo;
    CpuStat                  cpuStat;
    std::vector<ProcessInfo> processes;  // sorted by cpuPercent descending

    SystemSnapshot();

    // Call periodically to refresh all values
    void update();

    void print() const;

private:
    CpuSample                            mPrevSample;
    CpuSample                            mCurrentSample;
    std::unordered_map<pid_t, PidSample> mPrevPidSamples;

    // Updates the processes vector using totalSystemDeltaTicks as CPU denominator
    void updateProcesses(uint64_t totalSystemDeltaTicks);
};
