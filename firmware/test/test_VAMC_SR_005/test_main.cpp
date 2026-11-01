#include <unity.h>
#include "diagnostics_ota.h"

using namespace vamc;

class OneByteSource : public IOtaFirmwareSource {
public:
    bool open(OtaImageInfo& info) override { info = {1, 1}; return true; }
    size_t read(uint8_t* out, size_t capacity) override {
        if (position_ || capacity == 0) return 0;
        *out = 0x42;
        position_ = true;
        return 1;
    }
    bool failed() const override { return false; }
    void close() override { closed_ = true; }
    bool closed() const { return closed_; }
private:
    bool position_ = false;
    bool closed_ = false;
};

class SignatureBackend : public IOtaUpdateBackend {
public:
    bool verifier_available = true;
    OtaFinalizeResult verification = OtaFinalizeResult::VERIFIED_AND_ACTIVATED;
    unsigned begin_count = 0;
    unsigned finish_count = 0;
    unsigned abort_count = 0;
    unsigned reboot_count = 0;

    bool enable_signature_verification() override { return verifier_available; }
    bool begin(uint32_t) override { ++begin_count; return true; }
    size_t write(const uint8_t*, size_t count) override { return count; }
    OtaFinalizeResult finish_signed_update() override {
        ++finish_count;
        return verification;
    }
    void abort() override { ++abort_count; }
    void reboot() override { ++reboot_count; }
};

void setUp() {}
void tearDown() {}

void test_TC_SEC_05_fails_closed_without_trusted_signature_verifier() {
    OneByteSource source;
    SignatureBackend backend;
    backend.verifier_available = false;

    TEST_ASSERT_EQUAL(OtaResult::SIGNATURE_VERIFIER_UNAVAILABLE,
                     OtaUpdater().apply(source, backend));
    TEST_ASSERT_EQUAL_UINT(0, backend.begin_count);
    TEST_ASSERT_EQUAL_UINT(0, backend.finish_count);
    TEST_ASSERT_EQUAL_UINT(0, backend.reboot_count);
    TEST_ASSERT_TRUE(source.closed());
}

void test_TC_SEC_05_rejects_signature_failure_without_activation() {
    OneByteSource source;
    SignatureBackend backend;
    backend.verification = OtaFinalizeResult::SIGNATURE_REJECTED;

    TEST_ASSERT_EQUAL(OtaResult::SIGNATURE_REJECTED,
                     OtaUpdater().apply(source, backend));
    TEST_ASSERT_EQUAL_UINT(1, backend.finish_count);
    TEST_ASSERT_EQUAL_UINT(1, backend.abort_count);
    TEST_ASSERT_EQUAL_UINT(0, backend.reboot_count);
}

void test_TC_SEC_05_reboots_only_after_verified_image_activation() {
    OneByteSource source;
    SignatureBackend backend;

    TEST_ASSERT_EQUAL(OtaResult::SUCCESS,
                     OtaUpdater().apply(source, backend));
    TEST_ASSERT_EQUAL_UINT(1, backend.finish_count);
    TEST_ASSERT_EQUAL_UINT(1, backend.reboot_count);
    TEST_ASSERT_EQUAL_UINT(0, backend.abort_count);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_SEC_05_fails_closed_without_trusted_signature_verifier);
    RUN_TEST(test_TC_SEC_05_rejects_signature_failure_without_activation);
    RUN_TEST(test_TC_SEC_05_reboots_only_after_verified_image_activation);
    return UNITY_END();
}
