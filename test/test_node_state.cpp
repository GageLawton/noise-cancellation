#include <cstddef>
#include <cstring>

#include "anc/node_state.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr anc::NodeState kAllStates[] = {
    anc::NodeState::Boot,
    anc::NodeState::InitHal,
    anc::NodeState::CalibrateSecondaryPath,
    anc::NodeState::Run,
    anc::NodeState::RecheckSecondaryPath,
    anc::NodeState::Fault,
    anc::NodeState::SafeMute,
};
constexpr std::size_t kStateCount = sizeof(kAllStates) / sizeof(kAllStates[0]);

// The names telemetry prints must match design doc section 5.3.
void test_names_match_design_doc() {
    TEST_ASSERT_EQUAL_STRING("BOOT", anc::toString(anc::NodeState::Boot));
    TEST_ASSERT_EQUAL_STRING("INIT_HAL", anc::toString(anc::NodeState::InitHal));
    TEST_ASSERT_EQUAL_STRING("CALIBRATE_SECONDARY_PATH",
                             anc::toString(anc::NodeState::CalibrateSecondaryPath));
    TEST_ASSERT_EQUAL_STRING("RUN", anc::toString(anc::NodeState::Run));
    TEST_ASSERT_EQUAL_STRING("RECHECK_SECONDARY_PATH",
                             anc::toString(anc::NodeState::RecheckSecondaryPath));
    TEST_ASSERT_EQUAL_STRING("FAULT", anc::toString(anc::NodeState::Fault));
    TEST_ASSERT_EQUAL_STRING("SAFE_MUTE", anc::toString(anc::NodeState::SafeMute));
}

void test_names_are_all_different() {
    for (std::size_t i = 0; i < kStateCount; ++i) {
        for (std::size_t j = i + 1; j < kStateCount; ++j) {
            TEST_ASSERT_TRUE_MESSAGE(std::strcmp(anc::toString(kAllStates[i]),
                                                 anc::toString(kAllStates[j])) != 0,
                                     "two states share a name");
        }
    }
}

// A value outside the enum (for example from corrupted memory) still
// gives a printable name instead of a null pointer.
void test_unknown_value_has_a_name() {
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", anc::toString(static_cast<anc::NodeState>(99)));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_names_match_design_doc);
    RUN_TEST(test_names_are_all_different);
    RUN_TEST(test_unknown_value_has_a_name);
    return UNITY_END();
}
