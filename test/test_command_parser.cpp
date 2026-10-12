#include <cstring>

#include "anc/command_parser.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

using anc::Verb;

void test_get() {
    const anc::Command cmd = anc::parseCommand("get dsp.normalized_step_size");
    TEST_ASSERT_TRUE(cmd.verb == Verb::Get);
    TEST_ASSERT_EQUAL_STRING("dsp.normalized_step_size", cmd.key);
    TEST_ASSERT_EQUAL_STRING("", cmd.value);
}

void test_set() {
    const anc::Command cmd = anc::parseCommand("set dsp.normalized_step_size 0.2");
    TEST_ASSERT_TRUE(cmd.verb == Verb::Set);
    TEST_ASSERT_EQUAL_STRING("dsp.normalized_step_size", cmd.key);
    TEST_ASSERT_EQUAL_STRING("0.2", cmd.value);
}

void test_words_without_arguments() {
    TEST_ASSERT_TRUE(anc::parseCommand("save").verb == Verb::Save);
    TEST_ASSERT_TRUE(anc::parseCommand("recal").verb == Verb::Recal);
    TEST_ASSERT_TRUE(anc::parseCommand("help").verb == Verb::Help);
}

void test_extra_spaces_and_line_ending() {
    const anc::Command cmd = anc::parseCommand("  set   output.ceiling\t 0.5 \r\n");
    TEST_ASSERT_TRUE(cmd.verb == Verb::Set);
    TEST_ASSERT_EQUAL_STRING("output.ceiling", cmd.key);
    TEST_ASSERT_EQUAL_STRING("0.5", cmd.value);
    TEST_ASSERT_TRUE(anc::parseCommand("save\r\n").verb == Verb::Save);
}

void test_set_without_value_is_invalid() {
    TEST_ASSERT_TRUE(anc::parseCommand("set dsp.normalized_step_size").verb ==
                     Verb::Invalid);
    TEST_ASSERT_TRUE(anc::parseCommand("get").verb == Verb::Invalid);
}

void test_unknown_or_empty_is_invalid() {
    TEST_ASSERT_TRUE(anc::parseCommand("reboot").verb == Verb::Invalid);
    TEST_ASSERT_TRUE(anc::parseCommand("verylongverb").verb == Verb::Invalid);
    TEST_ASSERT_TRUE(anc::parseCommand("").verb == Verb::Invalid);
    TEST_ASSERT_TRUE(anc::parseCommand("   \r\n").verb == Verb::Invalid);
    TEST_ASSERT_TRUE(anc::parseCommand(nullptr).verb == Verb::Invalid);
}

void test_extra_words_are_invalid() {
    TEST_ASSERT_TRUE(anc::parseCommand("save now").verb == Verb::Invalid);
    TEST_ASSERT_TRUE(anc::parseCommand("set a.b 1 2").verb == Verb::Invalid);
}

void test_too_long_key_or_value_is_invalid() {
    char line[80] = "get ";
    std::memset(line + 4, 'k', 60);  // a 60-character key
    line[64] = '\0';
    TEST_ASSERT_TRUE(anc::parseCommand(line).verb == Verb::Invalid);

    // 47 characters is the longest key that fits.
    line[4 + 47] = '\0';
    TEST_ASSERT_TRUE(anc::parseCommand(line).verb == Verb::Get);

    TEST_ASSERT_TRUE(
        anc::parseCommand("set a.b 0123456789012345678901234567890123456789").verb ==
        Verb::Invalid);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_get);
    RUN_TEST(test_set);
    RUN_TEST(test_words_without_arguments);
    RUN_TEST(test_extra_spaces_and_line_ending);
    RUN_TEST(test_set_without_value_is_invalid);
    RUN_TEST(test_unknown_or_empty_is_invalid);
    RUN_TEST(test_extra_words_are_invalid);
    RUN_TEST(test_too_long_key_or_value_is_invalid);
    return UNITY_END();
}
