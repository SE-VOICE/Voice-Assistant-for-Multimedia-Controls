#include <unity.h>
#include "reset_button.h"
using namespace vamc;
void setUp() {}
void tearDown() {}
// 100 ms is a synthetic test parameter, not the approved product threshold.
void test_TC_CONN_03_held_at_boot_requires_release() {
    ResetButton button(100);
    TEST_ASSERT_FALSE(button.poll(true, 0));
    TEST_ASSERT_FALSE(button.poll(true, 1000));
    TEST_ASSERT_FALSE(button.poll(false, 1001));
    TEST_ASSERT_FALSE(button.poll(true, 1002));
    TEST_ASSERT_TRUE(button.poll(true, 1102));
}
void test_TC_CONN_03_short_press_and_bounce_cannot_accumulate() {
    ResetButton button(100);
    button.poll(false, 0);
    button.poll(true, 1);
    TEST_ASSERT_FALSE(button.poll(false, 99));
    TEST_ASSERT_FALSE(button.poll(true, 100));
    TEST_ASSERT_FALSE(button.poll(true, 199));
    TEST_ASSERT_TRUE(button.poll(true, 200));
}
void test_TC_CONN_03_one_reset_per_hold_rearms_after_release() {
    ResetButton button(100);
    button.poll(false, 0); button.poll(true, 1);
    TEST_ASSERT_TRUE(button.poll(true, 101));
    TEST_ASSERT_FALSE(button.poll(true, 1000));
    button.poll(false, 1001); button.poll(true, 1002);
    TEST_ASSERT_TRUE(button.poll(true, 1102));
}
void test_TC_CONN_03_clock_wrap_is_safe() {
    ResetButton button(100);
    button.poll(false, 0xffffffefu); button.poll(true, 0xfffffff0u);
    TEST_ASSERT_FALSE(button.poll(true, 0x53));
    TEST_ASSERT_TRUE(button.poll(true, 0x54));
}
void test_TC_CONN_03_zero_duration_disables_reset() {
    ResetButton button(0);
    button.poll(false, 0); button.poll(true, 1);
    TEST_ASSERT_FALSE(button.poll(true, 100000));
}
void test_TC_CONN_03_approved_five_second_boundary() {
    ResetButton button(kApprovedBootResetHoldMs);
    button.poll(false, 0); button.poll(true, 1);
    TEST_ASSERT_FALSE(button.poll(true, 5000));
    TEST_ASSERT_TRUE(button.poll(true, 5001));
    TEST_ASSERT_FALSE(button.poll(true, 10001));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_CONN_03_held_at_boot_requires_release);
    RUN_TEST(test_TC_CONN_03_short_press_and_bounce_cannot_accumulate);
    RUN_TEST(test_TC_CONN_03_one_reset_per_hold_rearms_after_release);
    RUN_TEST(test_TC_CONN_03_clock_wrap_is_safe);
    RUN_TEST(test_TC_CONN_03_zero_duration_disables_reset);
    RUN_TEST(test_TC_CONN_03_approved_five_second_boundary);
    return UNITY_END();
}
