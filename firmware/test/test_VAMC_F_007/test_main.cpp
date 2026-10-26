#include <unity.h>
#include "command_ids.h"
#include "command_map.h"

using namespace vamc;

void setUp() {}
void tearDown() {}

// Test TC-CMD-01: All 7 commands independently verified to map to correct HID usage codes
void test_TC_CMD_01_all_7_commands_map_to_valid_hid_usages() {
    TEST_ASSERT_EQUAL_HEX16(hid_usage::VOLUME_INCREMENT, CommandMapper::to_hid_usage(CommandId::VOLUME_UP));
    TEST_ASSERT_EQUAL_HEX16(hid_usage::VOLUME_DECREMENT, CommandMapper::to_hid_usage(CommandId::VOLUME_DOWN));
    TEST_ASSERT_EQUAL_HEX16(hid_usage::PAUSE, CommandMapper::to_hid_usage(CommandId::PAUSE));
    TEST_ASSERT_EQUAL_HEX16(hid_usage::PLAY, CommandMapper::to_hid_usage(CommandId::RESUME));
    TEST_ASSERT_EQUAL_HEX16(hid_usage::PLAY, CommandMapper::to_hid_usage(CommandId::PLAY));
    TEST_ASSERT_EQUAL_HEX16(hid_usage::SCAN_NEXT_TRACK, CommandMapper::to_hid_usage(CommandId::NEXT));
    TEST_ASSERT_EQUAL_HEX16(hid_usage::SCAN_PREV_TRACK, CommandMapper::to_hid_usage(CommandId::PREVIOUS));
}

void test_all_7_commands_are_supported() {
    TEST_ASSERT_TRUE(CommandMapper::is_supported(CommandId::VOLUME_UP));
    TEST_ASSERT_TRUE(CommandMapper::is_supported(CommandId::VOLUME_DOWN));
    TEST_ASSERT_TRUE(CommandMapper::is_supported(CommandId::PAUSE));
    TEST_ASSERT_TRUE(CommandMapper::is_supported(CommandId::RESUME));
    TEST_ASSERT_TRUE(CommandMapper::is_supported(CommandId::PLAY));
    TEST_ASSERT_TRUE(CommandMapper::is_supported(CommandId::NEXT));
    TEST_ASSERT_TRUE(CommandMapper::is_supported(CommandId::PREVIOUS));

    TEST_ASSERT_EQUAL_UINT(7, CommandMapper::kSupportedCommandCount);
}

void test_invalid_command_id_rejected() {
    CommandId invalid_cmd = static_cast<CommandId>(99);
    TEST_ASSERT_FALSE(CommandMapper::is_supported(invalid_cmd));
    TEST_ASSERT_EQUAL_HEX16(hid_usage::NONE, CommandMapper::to_hid_usage(invalid_cmd));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", CommandMapper::to_string(invalid_cmd));
}

void test_command_to_string() {
    TEST_ASSERT_EQUAL_STRING("VOLUME_UP", CommandMapper::to_string(CommandId::VOLUME_UP));
    TEST_ASSERT_EQUAL_STRING("VOLUME_DOWN", CommandMapper::to_string(CommandId::VOLUME_DOWN));
    TEST_ASSERT_EQUAL_STRING("PAUSE", CommandMapper::to_string(CommandId::PAUSE));
    TEST_ASSERT_EQUAL_STRING("RESUME", CommandMapper::to_string(CommandId::RESUME));
    TEST_ASSERT_EQUAL_STRING("PLAY", CommandMapper::to_string(CommandId::PLAY));
    TEST_ASSERT_EQUAL_STRING("NEXT", CommandMapper::to_string(CommandId::NEXT));
    TEST_ASSERT_EQUAL_STRING("PREVIOUS", CommandMapper::to_string(CommandId::PREVIOUS));
}

void test_from_string_parsing() {
    CommandId id;
    TEST_ASSERT_TRUE(CommandMapper::from_string("volume up", id));
    TEST_ASSERT_EQUAL(CommandId::VOLUME_UP, id);

    TEST_ASSERT_TRUE(CommandMapper::from_string("VOL DOWN", id));
    TEST_ASSERT_EQUAL(CommandId::VOLUME_DOWN, id);

    TEST_ASSERT_TRUE(CommandMapper::from_string("pause", id));
    TEST_ASSERT_EQUAL(CommandId::PAUSE, id);

    TEST_ASSERT_TRUE(CommandMapper::from_string("Resume", id));
    TEST_ASSERT_EQUAL(CommandId::RESUME, id);

    TEST_ASSERT_TRUE(CommandMapper::from_string("play music", id));
    TEST_ASSERT_EQUAL(CommandId::PLAY, id);

    TEST_ASSERT_TRUE(CommandMapper::from_string("next track", id));
    TEST_ASSERT_EQUAL(CommandId::NEXT, id);

    TEST_ASSERT_TRUE(CommandMapper::from_string("prev", id));
    TEST_ASSERT_EQUAL(CommandId::PREVIOUS, id);

    // Negative tests
    TEST_ASSERT_FALSE(CommandMapper::from_string("unsupported action", id));
    TEST_ASSERT_FALSE(CommandMapper::from_string("", id));
    TEST_ASSERT_FALSE(CommandMapper::from_string(nullptr, id));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_CMD_01_all_7_commands_map_to_valid_hid_usages);
    RUN_TEST(test_all_7_commands_are_supported);
    RUN_TEST(test_invalid_command_id_rejected);
    RUN_TEST(test_command_to_string);
    RUN_TEST(test_from_string_parsing);
    return UNITY_END();
}
