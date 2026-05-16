#include "systemSnapshot.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ─── readCpuSample ───────────────────────────────────────────────────────────
//
// Reads the first "cpu" line from /proc/stat and returns raw tick counts.
// The line format is:
//   cpu user nice system idle iowait irq softirq steal guest guest_nice
// guest and guest_nice are already included in user/nice so we skip them.

CpuSample readCpuSample() {
    CpuSample sample{};

    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
        return sample;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.starts_with("cpu ")) {          // "cpu " — skip per-core lines (cpu0, cpu1...)
            std::istringstream ss(line);
            std::string label;
            ss >> label
               >> sample.user
               >> sample.nice
               >> sample.system
               >> sample.idle
               >> sample.iowait
               >> sample.irq
               >> sample.softirq
               >> sample.steal;
            return sample;
        }
    }

    return sample;
}

// ─── computeCpuStat ──────────────────────────────────────────────────────────
//
// Computes CPU usage percentages from two consecutive raw samples.
// Subtraction is done in uint64_t to avoid float precision loss on large values,
// then the small deltas are safely cast to float for the division.

CpuStat computeCpuStat(const CpuSample& prev, const CpuSample& current) {
    // Subtract in uint64_t — no precision loss
    const uint64_t d_user    = current.user    - prev.user;
    const uint64_t d_nice    = current.nice    - prev.nice;
    const uint64_t d_system  = current.system  - prev.system;
    const uint64_t d_idle    = current.idle    - prev.idle;
    const uint64_t d_iowait  = current.iowait  - prev.iowait;
    const uint64_t d_irq     = current.irq     - prev.irq;
    const uint64_t d_softirq = current.softirq - prev.softirq;
    const uint64_t d_steal   = current.steal   - prev.steal;

    // Total includes ALL fields so percentages sum to 100%
    const uint64_t d_total = d_user + d_nice + d_system + d_idle
                           + d_iowait + d_irq + d_softirq + d_steal;

    if (d_total == 0) {
        return {};
    }

    const float total_f = static_cast<float>(d_total);
    const uint64_t d_other = d_irq + d_softirq + d_steal;

    return {
        100.0f * static_cast<float>(d_user)   / total_f,
        100.0f * static_cast<float>(d_nice)   / total_f,
        100.0f * static_cast<float>(d_system) / total_f,
        100.0f * static_cast<float>(d_idle)   / total_f,
        100.0f * static_cast<float>(d_iowait) / total_f,
        100.0f * static_cast<float>(d_other)  / total_f
    };
}

// ─── getMemInfo ──────────────────────────────────────────────────────────────
//
// Reads /proc/meminfo. Values are in kB in the file — stored as-is.
// We stop early once all three fields are found to avoid reading the full file.

MemInfo getMemInfo() {
    MemInfo info{};

    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        return info;
    }

    int found = 0;  // stop early once all 3 fields are read
    std::string line;

    while (found < 3 && std::getline(file, line)) {
        std::istringstream ss(line);
        std::string label;
        std::size_t value{0};
        std::string unit;           // "kB" — ignored

        ss >> label >> value >> unit;

        if      (label == "MemTotal:")     { info.total     = value; ++found; }
        else if (label == "MemFree:")      { info.free      = value; ++found; }
        else if (label == "MemAvailable:") { info.available = value; ++found; }
    }

    return info;
}

// ─── getOsName ───────────────────────────────────────────────────────────────
//
// Reads PRETTY_NAME from /etc/os-release.
// Values in that file are double-quoted — e.g. PRETTY_NAME="Ubuntu 24.04 LTS"

std::string getOsName() {
    std::ifstream file("/etc/os-release");
    if (!file.is_open()) {
        return "Unknown OS";
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.starts_with("PRETTY_NAME=")) {
            std::string value = line.substr(12);    // skip "PRETTY_NAME="

            // Strip surrounding double quotes
            if (value.size() >= 2
                && value.front() == '"'
                && value.back()  == '"') {
                value = value.substr(1, value.size() - 2);
            }

            return value;
        }
    }

    return "Unknown OS";
}

// ─── SystemSnapshot ──────────────────────────────────────────────────────────

SystemSnapshot::SystemSnapshot()
    : osName(getOsName())
    , memInfo(getMemInfo())
    , cpuStat{}                         // zeros — meaningful only after first update()
    , mPrevSample(readCpuSample())
    , mCurrentSample(mPrevSample)
{
    // cpuStat is intentionally left at zero on construction:
    // both samples are taken at the same instant so the delta is meaningless.
    // Call update() after at least one polling interval to get real values.
}

void SystemSnapshot::update() {
    osName         = getOsName();
    memInfo        = getMemInfo();
    mCurrentSample = readCpuSample();
    cpuStat        = computeCpuStat(mPrevSample, mCurrentSample);
    mPrevSample    = mCurrentSample;
}

void SystemSnapshot::print() const {
    std::cout << "OS:     " << osName << '\n'
              << "Memory: total="     << memInfo.total
              << " free="             << memInfo.free
              << " available="        << memInfo.available << " kB\n"
              << "CPU:    user="      << cpuStat.user
              << "% nice="           << cpuStat.nice
              << "% system="         << cpuStat.system
              << "% idle="           << cpuStat.idle << "%\n";
}
