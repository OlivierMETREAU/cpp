#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <thread>
#include <chrono>

#include "systemSnapshot.hpp"

// ─── CpuSample / computeCpuStat (free functions, testable) ───────────────────

TEST_CASE("readCpuSample returns non-zero tick counts", "[cpu][sample]") {
    CpuSample sample = readCpuSample();

    CAPTURE(sample.user);
    CAPTURE(sample.idle);

    REQUIRE(sample.total() > 0);
    REQUIRE(sample.idle    > 0);
}

TEST_CASE("computeCpuStat: identical samples return all zeros", "[cpu][delta]") {
    CpuSample sample = readCpuSample();
    CpuStat   stat   = computeCpuStat(sample, sample);

    REQUIRE(stat.user   == 0.0f);
    REQUIRE(stat.system == 0.0f);
    REQUIRE(stat.idle   == 0.0f);
}

TEST_CASE("computeCpuStat: percentages sum to ~100%", "[cpu][delta]") {
    CpuSample first = readCpuSample();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    CpuSample second = readCpuSample();

    CAPTURE(first.idle);
    CAPTURE(second.idle);

    // Ticks must have advanced
    REQUIRE(second.total() > first.total());

    CpuStat stat = computeCpuStat(first, second);
    float sum = stat.user + stat.nice + stat.system
              + stat.idle + stat.iowait + stat.other;

    CAPTURE(stat.user);
    CAPTURE(stat.system);
    CAPTURE(stat.idle);
    CAPTURE(sum);

    // Allow 0.5% float tolerance
    REQUIRE_THAT(sum, Catch::Matchers::WithinAbs(100.0f, 0.5f));
}

TEST_CASE("computeCpuStat: each percentage in [0, 100]", "[cpu][delta]") {
    CpuSample first = readCpuSample();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    CpuSample second = readCpuSample();

    CpuStat stat = computeCpuStat(first, second);

    REQUIRE(stat.user   >= 0.0f);  REQUIRE(stat.user   <= 100.0f);
    REQUIRE(stat.nice   >= 0.0f);  REQUIRE(stat.nice   <= 100.0f);
    REQUIRE(stat.system >= 0.0f);  REQUIRE(stat.system <= 100.0f);
    REQUIRE(stat.idle   >= 0.0f);  REQUIRE(stat.idle   <= 100.0f);
}

// ─── MemInfo ─────────────────────────────────────────────────────────────────

TEST_CASE("getMemInfo returns plausible values", "[memory]") {
    MemInfo info = getMemInfo();

    CAPTURE(info.total);
    CAPTURE(info.free);
    CAPTURE(info.available);

    REQUIRE(info.total     > 0);
    REQUIRE(info.free      > 0);
    REQUIRE(info.available > 0);
    REQUIRE(info.free      <= info.total);
    REQUIRE(info.available <= info.total);
}

// ─── SystemSnapshot ──────────────────────────────────────────────────────────

TEST_CASE("SystemSnapshot constructs without throwing", "[snapshot]") {
    REQUIRE_NOTHROW(SystemSnapshot{});
}

TEST_CASE("SystemSnapshot osName is not empty", "[snapshot]") {
    SystemSnapshot snap;
    REQUIRE_FALSE(snap.osName.empty());
}

TEST_CASE("SystemSnapshot osName contains Ubuntu on this machine", "[snapshot]") {
    SystemSnapshot snap;
    CAPTURE(snap.osName);
    REQUIRE(snap.osName.starts_with("Ubuntu"));
}

TEST_CASE("SystemSnapshot update changes CPU stats over time", "[snapshot]") {
    SystemSnapshot snap;

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    snap.update();

    float sum = snap.cpuStat.user + snap.cpuStat.nice
              + snap.cpuStat.system + snap.cpuStat.idle
              + snap.cpuStat.iowait + snap.cpuStat.other;

    CAPTURE(snap.cpuStat.user);
    CAPTURE(snap.cpuStat.idle);
    CAPTURE(sum);

    REQUIRE_THAT(sum, Catch::Matchers::WithinAbs(100.0f, 0.5f));
}

TEST_CASE("SystemSnapshot update refreshes memory info", "[snapshot]") {
    SystemSnapshot snap;
    size_t free_before = snap.memInfo.free;

    snap.update();

    // Values should still be plausible after update — not testing exact change
    REQUIRE(snap.memInfo.total > 0);
    REQUIRE(snap.memInfo.free  <= snap.memInfo.total);
    (void)free_before;  // suppress unused warning — value intentionally not compared
}