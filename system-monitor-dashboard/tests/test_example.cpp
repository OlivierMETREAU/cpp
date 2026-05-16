// Example test structure
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

TEST_CASE("DownloadTask state transitions", "[download][state]") {
    SECTION("Initial state is Queued") { ... }
    SECTION("Cannot pause a completed task") { ... }
}
