#include <unity.h>
#include "credential_vault.h"
#include <string.h>
#include <algorithm>
using namespace vamc;
class Blob : public EncryptedCredentialBlob {
public:
    uint8_t bytes[CredentialVault::kRecordBytes] = {};
    int length = 0;
    bool write_ok = true;
    int read(uint8_t* out, size_t capacity) override {
        if (length < 0 || size_t(length) > capacity) return -1;
        memcpy(out, bytes, length); return length;
    }
    bool write(const uint8_t* in, size_t count) override {
        if (!write_ok) return false;
        memcpy(bytes, in, count); length = count; return true;
    }
    bool erase() override { memset(bytes, 0, sizeof(bytes)); length = 0; return true; }
};
void setUp() {}
void tearDown() {}
void test_TC_SEC_03_ciphertext_roundtrip_and_reboot_unlock() {
    Blob blob; CredentialVault vault(blob);
    TEST_ASSERT_TRUE(vault.unlock("test-only-strong-passphrase"));
    TEST_ASSERT_TRUE(vault.save_protected("test-network", "test-password"));
    const char* secret = "test-password";
    TEST_ASSERT_TRUE(std::search(blob.bytes, blob.bytes + blob.length, secret, secret + strlen(secret)) == blob.bytes + blob.length);
    vault.lock();
    char ssid[33], password[65];
    TEST_ASSERT_FALSE(vault.load(ssid, password));
    CredentialVault rebooted(blob);
    TEST_ASSERT_TRUE(rebooted.unlock("test-only-strong-passphrase"));
    TEST_ASSERT_TRUE(rebooted.load(ssid, password));
    TEST_ASSERT_EQUAL_STRING("test-network", ssid);
    TEST_ASSERT_EQUAL_STRING("test-password", password);
}
void test_TC_SEC_03_wrong_passphrase_fails_closed() {
    Blob blob; CredentialVault vault(blob);
    TEST_ASSERT_TRUE(vault.unlock("test-passphrase"));
    TEST_ASSERT_TRUE(vault.save_protected("network", "password"));
    TEST_ASSERT_FALSE(vault.unlock("wrong-passphrase"));
    TEST_ASSERT_FALSE(vault.unlocked());
    TEST_ASSERT_FALSE(vault.save_protected("network", "password"));
}
void test_TC_SEC_03_tampered_tag_clears_outputs() {
    Blob blob; CredentialVault vault(blob);
    TEST_ASSERT_TRUE(vault.unlock("test-passphrase"));
    TEST_ASSERT_TRUE(vault.save_protected("network", "password"));
    blob.bytes[145] ^= 1;
    char ssid[33] = "stale", password[65] = "stale";
    TEST_ASSERT_FALSE(vault.load(ssid, password));
    TEST_ASSERT_EQUAL_STRING("", ssid); TEST_ASSERT_EQUAL_STRING("", password);
}
void test_TC_SEC_03_fresh_nonce_and_reset() {
    Blob blob; CredentialVault vault(blob);
    TEST_ASSERT_TRUE(vault.unlock("test-passphrase"));
    TEST_ASSERT_TRUE(vault.save_protected("network", "password"));
    uint8_t nonce[12]; memcpy(nonce, blob.bytes + 20, 12);
    TEST_ASSERT_TRUE(vault.save_protected("network", "password"));
    TEST_ASSERT_TRUE(memcmp(nonce, blob.bytes + 20, 12) != 0);
    TEST_ASSERT_TRUE(vault.erase()); TEST_ASSERT_FALSE(vault.unlocked());
    TEST_ASSERT_EQUAL_INT(0, blob.length);
}
void test_TC_SEC_03_storage_errors_and_empty_passphrase() {
    Blob blob; CredentialVault vault(blob);
    TEST_ASSERT_FALSE(vault.unlock(""));
    blob.length = -1; TEST_ASSERT_FALSE(vault.unlock("test-passphrase"));
    blob.length = 0; TEST_ASSERT_TRUE(vault.unlock("test-passphrase"));
    blob.write_ok = false; TEST_ASSERT_FALSE(vault.save_protected("network", "password"));
    TEST_ASSERT_EQUAL_INT(0, blob.length);
}
void test_TC_SEC_03_corrupt_format_is_not_overwritten() {
    Blob blob; blob.length = 4; memcpy(blob.bytes, "junk", 4);
    CredentialVault vault(blob);
    TEST_ASSERT_FALSE(vault.unlock("test-passphrase"));
    TEST_ASSERT_FALSE(vault.save_protected("network", "password"));
    TEST_ASSERT_EQUAL_MEMORY("junk", blob.bytes, 4);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_SEC_03_ciphertext_roundtrip_and_reboot_unlock);
    RUN_TEST(test_TC_SEC_03_wrong_passphrase_fails_closed);
    RUN_TEST(test_TC_SEC_03_tampered_tag_clears_outputs);
    RUN_TEST(test_TC_SEC_03_fresh_nonce_and_reset);
    RUN_TEST(test_TC_SEC_03_storage_errors_and_empty_passphrase);
    RUN_TEST(test_TC_SEC_03_corrupt_format_is_not_overwritten);
    return UNITY_END();
}
