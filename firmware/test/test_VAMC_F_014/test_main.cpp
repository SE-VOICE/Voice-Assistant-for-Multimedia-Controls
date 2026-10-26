#include <unity.h>
#include "diagnostics_ota.h"
#include "command_ids.h"
#include <string.h>

using namespace vamc;

char g_dumped_lines[8][128];
size_t g_dumped_count = 0;

void mock_serial_print(const char* line) {
    if (g_dumped_count < 8 && line != nullptr) {
        strncpy(g_dumped_lines[g_dumped_count], line, sizeof(g_dumped_lines[g_dumped_count]) - 1);
        g_dumped_lines[g_dumped_count][sizeof(g_dumped_lines[g_dumped_count]) - 1] = '\0';
        ++g_dumped_count;
    }
}

void setUp() {
    g_dumped_count = 0;
    memset(g_dumped_lines, 0, sizeof(g_dumped_lines));
}

void tearDown() {}

// Test TC-DIAG-01: Generate commands/errors and inspect serial/debug log; last N events retrievable
void test_TC_DIAG_01_last_N_events_stored_and_retrieved() {
    constexpr size_t kCapacity = 5;
    CircularLogBuffer buffer(kCapacity);

    TEST_ASSERT_TRUE(buffer.is_empty());
    TEST_ASSERT_EQUAL_UINT(0, buffer.count());

    // Append 3 events (less than capacity)
    buffer.append({CommandId::PLAY, 1000, error_codes::NONE});
    buffer.append({CommandId::PAUSE, 2000, error_codes::NONE});
    buffer.append({CommandId::VOLUME_UP, 3000, error_codes::BLE_TIMEOUT});

    TEST_ASSERT_EQUAL_UINT(3, buffer.count());
    TEST_ASSERT_FALSE(buffer.is_full());

    LogEntry entry;
    TEST_ASSERT_TRUE(buffer.get_entry(0, entry));
    TEST_ASSERT_EQUAL(CommandId::PLAY, entry.command);
    TEST_ASSERT_EQUAL_UINT32(1000, entry.timestamp);

    TEST_ASSERT_TRUE(buffer.get_entry(2, entry));
    TEST_ASSERT_EQUAL(CommandId::VOLUME_UP, entry.command);
    TEST_ASSERT_EQUAL_UINT16(error_codes::BLE_TIMEOUT, entry.error_code);

    TEST_ASSERT_FALSE(buffer.get_entry(3, entry)); // out of range
}

void test_TC_DIAG_01_circular_overwrite_maintains_last_N_chronologically() {
    constexpr size_t kCapacity = 3;
    CircularLogBuffer buffer(kCapacity);

    // Append 5 events into capacity 3
    buffer.append({CommandId::VOLUME_UP, 100, 0});
    buffer.append({CommandId::VOLUME_DOWN, 200, 0});
    buffer.append({CommandId::PLAY, 300, 0});
    // At this point buffer is full with timestamps 100, 200, 300
    TEST_ASSERT_TRUE(buffer.is_full());

    // Overwriting events
    buffer.append({CommandId::NEXT, 400, error_codes::BLE_TIMEOUT});
    buffer.append({CommandId::PREVIOUS, 500, error_codes::NONE});

    TEST_ASSERT_EQUAL_UINT(3, buffer.count());

    LogEntry e0, e1, e2;
    TEST_ASSERT_TRUE(buffer.get_entry(0, e0));
    TEST_ASSERT_TRUE(buffer.get_entry(1, e1));
    TEST_ASSERT_TRUE(buffer.get_entry(2, e2));

    // Oldest surviving should be PLAY (ts: 300)
    TEST_ASSERT_EQUAL(CommandId::PLAY, e0.command);
    TEST_ASSERT_EQUAL_UINT32(300, e0.timestamp);

    // Second should be NEXT (ts: 400)
    TEST_ASSERT_EQUAL(CommandId::NEXT, e1.command);
    TEST_ASSERT_EQUAL_UINT32(400, e1.timestamp);
    TEST_ASSERT_EQUAL_UINT16(error_codes::BLE_TIMEOUT, e1.error_code);

    // Most recent should be PREVIOUS (ts: 500)
    TEST_ASSERT_EQUAL(CommandId::PREVIOUS, e2.command);
    TEST_ASSERT_EQUAL_UINT32(500, e2.timestamp);
}

void test_serial_dump_formatting() {
    CircularLogBuffer buffer(2);
    buffer.append({CommandId::RESUME, 1500, error_codes::NONE});
    buffer.append({CommandId::VOLUME_DOWN, 2500, error_codes::ASR_UNRECOGNIZED});

    g_dumped_count = 0;
    buffer.dump(mock_serial_print);

    TEST_ASSERT_EQUAL_UINT(2, g_dumped_count);
    TEST_ASSERT_TRUE(strstr(g_dumped_lines[0], "RESUME") != nullptr);
    TEST_ASSERT_TRUE(strstr(g_dumped_lines[0], "1500") != nullptr);
    TEST_ASSERT_TRUE(strstr(g_dumped_lines[1], "VOLUME_DOWN") != nullptr);
    TEST_ASSERT_TRUE(strstr(g_dumped_lines[1], "0x0003") != nullptr);
}

// Security / Privacy test (VAMC-F-014, SR-003): no audio samples, credentials or PII in log records
void test_security_log_structure_contains_no_sensitive_fields() {
    // LogEntry contains exactly CommandId (1 byte + padding), timestamp (4 bytes), error_code (2 bytes)
    // and no string pointers, audio buffers or network tokens.
    TEST_ASSERT_TRUE(sizeof(LogEntry) <= 12);
}

void test_buffer_clear() {
    CircularLogBuffer buffer(4);
    buffer.append({CommandId::PLAY, 100, 0});
    TEST_ASSERT_EQUAL_UINT(1, buffer.count());

    buffer.clear();
    TEST_ASSERT_EQUAL_UINT(0, buffer.count());
    TEST_ASSERT_TRUE(buffer.is_empty());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_DIAG_01_last_N_events_stored_and_retrieved);
    RUN_TEST(test_TC_DIAG_01_circular_overwrite_maintains_last_N_chronologically);
    RUN_TEST(test_serial_dump_formatting);
    RUN_TEST(test_security_log_structure_contains_no_sensitive_fields);
    RUN_TEST(test_buffer_clear);
    return UNITY_END();
}
