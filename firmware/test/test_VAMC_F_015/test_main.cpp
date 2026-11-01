#include <unity.h>
#include "diagnostics_ota.h"
#include <string.h>

using namespace vamc;

class MockOtaSource : public IOtaFirmwareSource {
public:
    uint8_t image[1300];
    uint32_t image_size = sizeof(image);
    size_t position = 0;
    size_t max_read = 700;
    size_t fail_after = static_cast<size_t>(-1);
    bool open_succeeds = true;
    bool read_failed = false;
    bool closed = false;

    MockOtaSource() {
        for (size_t i = 0; i < sizeof(image); ++i) {
            image[i] = static_cast<uint8_t>(i & 0xff);
        }
    }

    bool open(OtaImageInfo& info) override {
        if (!open_succeeds) return false;
        position = 0;
        info.size_bytes = image_size;
        info.version = 2;
        return true;
    }

    size_t read(uint8_t* destination, size_t capacity) override {
        if (position >= fail_after) {
            read_failed = true;
            return 0;
        }
        const size_t remaining = image_size - position;
        const size_t before_failure = fail_after - position;
        size_t count = capacity < remaining ? capacity : remaining;
        if (count > max_read) count = max_read;
        if (count > before_failure) count = before_failure;
        if (count == 0) return 0;
        memcpy(destination, image + position, count);
        position += count;
        return count;
    }

    bool failed() const override { return read_failed; }
    void close() override { closed = true; }
};

class MockOtaBackend : public IOtaUpdateBackend {
public:
    bool signature_support = true;
    bool begin_succeeds = true;
    bool short_write = false;
    OtaFinalizeResult finalize_result = OtaFinalizeResult::VERIFIED_AND_ACTIVATED;
    uint32_t expected_size = 0;
    size_t written = 0;
    uint8_t signature_checks = 0;
    uint8_t begin_calls = 0;
    uint8_t finalize_calls = 0;
    uint8_t abort_calls = 0;
    uint8_t reboot_calls = 0;
    uint8_t event_count = 0;
    char events[8] = {0};

    void event(char value) {
        if (event_count < sizeof(events)) events[event_count++] = value;
    }
    bool enable_signature_verification() override {
        event('S');
        ++signature_checks;
        return signature_support;
    }
    bool begin(uint32_t size) override {
        event('B');
        ++begin_calls;
        expected_size = size;
        return begin_succeeds;
    }
    size_t write(const uint8_t*, size_t length) override {
        written += length;
        return short_write && length > 0 ? length - 1 : length;
    }
    OtaFinalizeResult finish_signed_update() override {
        event('F');
        ++finalize_calls;
        return finalize_result;
    }
    void abort() override { ++abort_calls; }
    void reboot() override { ++reboot_calls; }
};

void setUp() {}
void tearDown() {}

void test_TC_DIAG_02_streams_signed_image_then_reboots() {
    MockOtaSource source;
    MockOtaBackend backend;
    const OtaResult result = OtaUpdater().apply(source, backend);

    TEST_ASSERT_EQUAL(OtaResult::SUCCESS, result);
    TEST_ASSERT_TRUE(source.closed);
    TEST_ASSERT_EQUAL_UINT32(source.image_size, backend.expected_size);
    TEST_ASSERT_EQUAL_UINT32(source.image_size, backend.written);
    TEST_ASSERT_EQUAL_UINT8(1, backend.signature_checks);
    TEST_ASSERT_EQUAL_UINT8(1, backend.begin_calls);
    TEST_ASSERT_EQUAL_UINT8(1, backend.finalize_calls);
    TEST_ASSERT_EQUAL_UINT8(1, backend.reboot_calls);
    TEST_ASSERT_EQUAL_UINT8(0, backend.abort_calls);
    TEST_ASSERT_EQUAL('S', backend.events[0]);
    TEST_ASSERT_EQUAL('B', backend.events[1]);
    TEST_ASSERT_EQUAL('F', backend.events[2]);
}

void test_TC_SEC_05_refuses_to_stage_when_signature_verification_unavailable() {
    MockOtaSource source;
    MockOtaBackend backend;
    backend.signature_support = false;

    const OtaResult result = OtaUpdater().apply(source, backend);

    TEST_ASSERT_EQUAL(OtaResult::SIGNATURE_VERIFIER_UNAVAILABLE, result);
    TEST_ASSERT_TRUE(source.closed);
    TEST_ASSERT_EQUAL_UINT8(0, backend.begin_calls);
    TEST_ASSERT_EQUAL_UINT8(0, backend.finalize_calls);
    TEST_ASSERT_EQUAL_UINT8(0, backend.reboot_calls);
}

void test_TC_SEC_05_rejects_unsigned_or_tampered_image_without_reboot() {
    MockOtaSource source;
    MockOtaBackend backend;
    backend.finalize_result = OtaFinalizeResult::SIGNATURE_REJECTED;

    const OtaResult result = OtaUpdater().apply(source, backend);

    TEST_ASSERT_EQUAL(OtaResult::SIGNATURE_REJECTED, result);
    TEST_ASSERT_TRUE(source.closed);
    TEST_ASSERT_EQUAL_UINT8(1, backend.finalize_calls);
    TEST_ASSERT_EQUAL_UINT8(1, backend.abort_calls);
    TEST_ASSERT_EQUAL_UINT8(0, backend.reboot_calls);
}

void test_TC_DIAG_02_short_download_aborts_staged_update() {
    MockOtaSource source;
    MockOtaBackend backend;
    source.fail_after = 600;

    const OtaResult result = OtaUpdater().apply(source, backend);

    TEST_ASSERT_EQUAL(OtaResult::SOURCE_READ_FAILED, result);
    TEST_ASSERT_TRUE(source.closed);
    TEST_ASSERT_EQUAL_UINT8(1, backend.abort_calls);
    TEST_ASSERT_EQUAL_UINT8(0, backend.finalize_calls);
    TEST_ASSERT_EQUAL_UINT8(0, backend.reboot_calls);
}

void test_TC_DIAG_02_failed_write_aborts_before_finalization() {
    MockOtaSource source;
    MockOtaBackend backend;
    backend.short_write = true;

    const OtaResult result = OtaUpdater().apply(source, backend);

    TEST_ASSERT_EQUAL(OtaResult::UPDATE_WRITE_FAILED, result);
    TEST_ASSERT_EQUAL_UINT8(1, backend.abort_calls);
    TEST_ASSERT_EQUAL_UINT8(0, backend.finalize_calls);
    TEST_ASSERT_EQUAL_UINT8(0, backend.reboot_calls);
}

void test_TC_DIAG_02_empty_image_is_rejected_before_flash() {
    MockOtaSource source;
    MockOtaBackend backend;
    source.image_size = 0;

    const OtaResult result = OtaUpdater().apply(source, backend);

    TEST_ASSERT_EQUAL(OtaResult::INVALID_IMAGE_SIZE, result);
    TEST_ASSERT_TRUE(source.closed);
    TEST_ASSERT_EQUAL_UINT8(0, backend.signature_checks);
    TEST_ASSERT_EQUAL_UINT8(0, backend.begin_calls);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_DIAG_02_streams_signed_image_then_reboots);
    RUN_TEST(test_TC_SEC_05_refuses_to_stage_when_signature_verification_unavailable);
    RUN_TEST(test_TC_SEC_05_rejects_unsigned_or_tampered_image_without_reboot);
    RUN_TEST(test_TC_DIAG_02_short_download_aborts_staged_update);
    RUN_TEST(test_TC_DIAG_02_failed_write_aborts_before_finalization);
    RUN_TEST(test_TC_DIAG_02_empty_image_is_rejected_before_flash);
    return UNITY_END();
}
