#include "systemSnapshot.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
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
        if (line.starts_with("cpu ")) {      // "cpu " — skip per-core lines (cpu0, cpu1...)
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
    const uint64_t d_user    = current.user    - prev.user;
    const uint64_t d_nice    = current.nice    - prev.nice;
    const uint64_t d_system  = current.system  - prev.system;
    const uint64_t d_idle    = current.idle    - prev.idle;
    const uint64_t d_iowait  = current.iowait  - prev.iowait;
    const uint64_t d_irq     = current.irq     - prev.irq;
    const uint64_t d_softirq = current.softirq - prev.softirq;
    const uint64_t d_steal   = current.steal   - prev.steal;

    const uint64_t d_total = d_user + d_nice + d_system + d_idle
                           + d_iowait + d_irq + d_softirq + d_steal;

    if (d_total == 0) {
        return {};
    }

    const float total_f = static_cast<float>(d_total);

    return {
        100.0f * static_cast<float>(d_user)                      / total_f,
        100.0f * static_cast<float>(d_nice)                      / total_f,
        100.0f * static_cast<float>(d_system)                    / total_f,
        100.0f * static_cast<float>(d_idle)                      / total_f,
        100.0f * static_cast<float>(d_iowait)                    / total_f,
        100.0f * static_cast<float>(d_irq + d_softirq + d_steal) / total_f,
    };
}

// ─── getMemInfo ──────────────────────────────────────────────────────────────
//
// Reads /proc/meminfo. Values are in kB — stored as-is.
// Stops early once all three fields are found.

MemInfo getMemInfo() {
    MemInfo info{};

    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        return info;
    }

    int found = 0;
    std::string line;

    while (found < 3 && std::getline(file, line)) {
        std::istringstream ss(line);
        std::string label;
        std::size_t value{0};
        std::string unit;   // "kB" — ignored

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

// ─── readPidSample ───────────────────────────────────────────────────────────
//
// Reads /proc/[pid]/stat for CPU ticks and /proc/[pid]/status for RSS memory.
//
// /proc/[pid]/stat format:
//   pid (comm) state ppid pgrp session tty_nr tpgid flags
//   minflt cminflt majflt cmajflt utime(14) stime(15) ...
//
// The comm field (process name) is wrapped in parentheses and can itself
// contain spaces or parentheses — so we use rfind(')') to find its end
// rather than parsing field by field from the start.

PidSample readPidSample(pid_t pid) {
    PidSample sample{};

    // ── /proc/[pid]/stat — name and CPU ticks ────────────────────────────────
    {
        const std::string path = "/proc/" + std::to_string(pid) + "/stat";
        std::ifstream file(path);
        if (!file.is_open()) {
            return sample;     // process died between directory scan and open
        }

        std::string content;
        std::getline(file, content);

        const auto nameStart = content.find('(');
        const auto nameEnd   = content.rfind(')');  // rfind: name may contain '('

        if (nameStart == std::string::npos || nameEnd == std::string::npos) {
            return sample;
        }

        sample.name = content.substr(nameStart + 1, nameEnd - nameStart - 1);

        // Everything after ") " — fields are space-separated from here.
        // Field order after comm: state ppid pgrp session tty_nr tpgid flags
        //   minflt cminflt majflt cmajflt utime(14) stime(15)
        std::istringstream ss(content.substr(nameEnd + 2));

        char     state;
        int      ppid, pgrp, session, tty_nr, tpgid;
        unsigned flags;
        uint64_t minflt, cminflt, majflt, cmajflt;

        ss >> state
           >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags
           >> minflt >> cminflt >> majflt >> cmajflt
           >> sample.utime >> sample.stime;
    }

    // ── /proc/[pid]/status — RSS memory ──────────────────────────────────────
    {
        const std::string path = "/proc/" + std::to_string(pid) + "/status";
        std::ifstream file(path);

        std::string line;
        while (std::getline(file, line)) {
            if (line.starts_with("VmRSS:")) {
                std::istringstream ss(line);
                std::string label;
                ss >> label >> sample.memoryKb;  // value is already in kB
                break;
            }
        }
    }

    return sample;
}

// ─── readAllPidSamples ───────────────────────────────────────────────────────
//
// Iterates /proc, identifies PID directories (purely numeric names),
// and reads a PidSample for each. Processes that die mid-scan are silently
// skipped — readPidSample returns an empty-named sample which is then ignored.

std::unordered_map<pid_t, PidSample> readAllPidSamples() {
    std::unordered_map<pid_t, PidSample> samples;

    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        if (!entry.is_directory()) {
            continue;
        }

        const std::string name = entry.path().filename().string();

        // Purely numeric directory name → it's a PID
        if (!std::ranges::all_of(name, ::isdigit)) {
            continue;
        }

        const pid_t pid    = static_cast<pid_t>(std::stoi(name));
        PidSample   sample = readPidSample(pid);

        if (sample.name.empty()) {
            continue;   // process died before we could read it
        }

        samples.emplace(pid, std::move(sample));
    }

    return samples;
}

// ─── computePidCpuPercent ────────────────────────────────────────────────────
//
// CPU% for one process = process ticks delta / total system ticks delta × 100.
// Using the system-wide total as denominator keeps per-process percentages
// consistent with the global CpuStat values.

float computePidCpuPercent(const PidSample& prev,
                           const PidSample& current,
                           uint64_t         totalSystemDeltaTicks) {
    if (totalSystemDeltaTicks == 0) {
        return 0.0f;
    }

    const uint64_t d_pid = (current.utime + current.stime)
                         - (prev.utime    + prev.stime);

    return 100.0f * static_cast<float>(d_pid)
                  / static_cast<float>(totalSystemDeltaTicks);
}

// ─── SystemSnapshot ──────────────────────────────────────────────────────────

SystemSnapshot::SystemSnapshot()
    : osName(getOsName())
    , memInfo(getMemInfo())
    , cpuStat{}                         // zeros — meaningful only after first update()
    , processes{}
    , mPrevSample(readCpuSample())
    , mCurrentSample(mPrevSample)
    , mPrevPidSamples(readAllPidSamples())
{
    // cpuStat and processes are intentionally left at zero/empty on construction:
    // both CPU samples are taken at the same instant so the delta is meaningless.
    // Call update() after at least one polling interval to get real values.
}

void SystemSnapshot::updateProcesses(uint64_t totalSystemDeltaTicks) {
    auto currentPidSamples = readAllPidSamples();

    processes.clear();

    for (const auto& [pid, current] : currentPidSamples) {
        ProcessInfo info;
        info.pid      = pid;
        info.name     = current.name;
        info.memoryKb = current.memoryKb;

        // O(1) lookup — was this PID alive in the previous poll?
        if (const auto it = mPrevPidSamples.find(pid);
            it != mPrevPidSamples.end()) {
            info.cpuPercent = computePidCpuPercent(
                it->second, current, totalSystemDeltaTicks);
        }
        // New PID with no previous sample → cpuPercent stays 0.0f

        processes.push_back(std::move(info));
    }

    // Sort descending by CPU% — what the UI will display
    std::ranges::sort(processes, std::greater{}, &ProcessInfo::cpuPercent);

    mPrevPidSamples = std::move(currentPidSamples);
}

void SystemSnapshot::update() {
    osName         = getOsName();
    memInfo        = getMemInfo();
    mCurrentSample = readCpuSample();
    cpuStat        = computeCpuStat(mPrevSample, mCurrentSample);

    const uint64_t totalDelta = mCurrentSample.total() - mPrevSample.total();
    updateProcesses(totalDelta);

    mPrevSample = mCurrentSample;
}

void SystemSnapshot::print() const {
    std::cout << "OS:     " << osName << '\n'
              << "Memory: total="  << memInfo.total
              << " free="          << memInfo.free
              << " available="     << memInfo.available << " kB\n"
              << "CPU:    user="   << cpuStat.user
              << "% system="       << cpuStat.system
              << "% idle="         << cpuStat.idle
              << "% iowait="       << cpuStat.iowait << "%\n"
              << "Top processes:\n";

    constexpr int topN = 5;
    for (int i = 0; i < topN && i < static_cast<int>(processes.size()); ++i) {
        const auto& p = processes[static_cast<std::size_t>(i)];
        std::cout << "  [" << p.pid << "] "
                  << p.name
                  << "  cpu=" << p.cpuPercent << "%"
                  << "  mem=" << p.memoryKb   << " kB\n";
    }
}
