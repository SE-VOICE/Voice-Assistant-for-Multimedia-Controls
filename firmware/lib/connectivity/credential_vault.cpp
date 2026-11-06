#include "credential_vault.h"
#include <string.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
#include <esp_random.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/version.h>
#else
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

namespace vamc {
namespace {
constexpr int kIterations = 600000; // Format VA01; benchmark boot unlock on target.
void wipe(void* memory, size_t size) {
    volatile uint8_t* p = static_cast<volatile uint8_t*>(memory);
    while (size--) *p++ = 0;
}
bool valid(const char* s, size_t capacity, bool allow_empty) {
    if (!s || (!allow_empty && !*s)) return false;
    for (size_t i = 0; i < capacity; ++i) if (!s[i]) return true;
    return false;
}
bool random_bytes(uint8_t* out, size_t length) {
#if defined(ARDUINO_ARCH_ESP32)
    // Application must initialize the radio entropy source before creating a vault.
    esp_fill_random(out, length);
    return true;
#else
    return RAND_bytes(out, static_cast<int>(length)) == 1;
#endif
}
bool derive(const char* pass, const uint8_t* salt, uint8_t* key) {
#if defined(ARDUINO_ARCH_ESP32)
#if MBEDTLS_VERSION_MAJOR >= 3
    return mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
        reinterpret_cast<const unsigned char*>(pass), strlen(pass), salt, 16,
        kIterations, 32, key) == 0;
#else
    mbedtls_md_context_t context;
    mbedtls_md_init(&context);
    bool ok = mbedtls_md_setup(&context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) == 0;
    if (ok) ok = mbedtls_pkcs5_pbkdf2_hmac(&context,
        reinterpret_cast<const unsigned char*>(pass), strlen(pass), salt, 16,
        kIterations, 32, key) == 0;
    mbedtls_md_free(&context);
    return ok;
#endif
#else
    return PKCS5_PBKDF2_HMAC(pass, static_cast<int>(strlen(pass)), salt, 16,
                            kIterations, EVP_sha256(), 32, key) == 1;
#endif
}
// Record: VA01 | salt16 | nonce12 | ciphertext98 | tag16.
bool crypt(bool encrypt, const uint8_t* key, uint8_t* record, uint8_t* plain) {
#if defined(ARDUINO_ARCH_ESP32)
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int result = mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, key, 256);
    if (!result && encrypt) result = mbedtls_gcm_crypt_and_tag(&context,
        MBEDTLS_GCM_ENCRYPT, 98, record + 20, 12, record, 20, plain,
        record + 32, 16, record + 130);
    else if (!result) result = mbedtls_gcm_auth_decrypt(&context, 98,
        record + 20, 12, record, 20, record + 130, 16, record + 32, plain);
    mbedtls_gcm_free(&context);
    return result == 0;
#else
    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
    if (!context) return false;
    int length = 0, final_length = 0;
    bool ok = EVP_CipherInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr, encrypt) == 1;
    ok = ok && EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) == 1;
    ok = ok && EVP_CipherInit_ex(context, nullptr, nullptr, key, record + 20, encrypt) == 1;
    ok = ok && EVP_CipherUpdate(context, nullptr, &length, record, 20) == 1;
    ok = ok && EVP_CipherUpdate(context, encrypt ? record + 32 : plain, &length,
                                encrypt ? plain : record + 32, 98) == 1;
    if (!encrypt) ok = ok && EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, 16, record + 130) == 1;
    if (ok) ok = EVP_CipherFinal_ex(context, (encrypt ? record + 32 : plain) + length, &final_length) == 1;
    if (encrypt) ok = ok && EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, 16, record + 130) == 1;
    EVP_CIPHER_CTX_free(context);
    return ok;
#endif
}
}
void CredentialVault::lock() {
    wipe(key_, sizeof(key_)); wipe(salt_, sizeof(salt_)); unlocked_ = false;
}
bool CredentialVault::unlock(const char* passphrase) {
    lock();
    if (!valid(passphrase, 129, false)) return false;
    uint8_t record[kRecordBytes] = {}, plain[98] = {};
    const int length = blob_.read(record, sizeof(record));
    bool ok = length == 0 || (length == static_cast<int>(sizeof(record)) && !memcmp(record, "VA01", 4));
    if (ok) {
        if (length) memcpy(salt_, record + 4, 16);
        else ok = random_bytes(salt_, 16);
    }
    if (ok) ok = derive(passphrase, salt_, key_);
    if (ok && length) ok = crypt(false, key_, record, plain);
    wipe(record, sizeof(record)); wipe(plain, sizeof(plain));
    if (!ok) lock();
    unlocked_ = ok;
    return ok;
}
bool CredentialVault::save_protected(const char* ssid, const char* password) {
    if (!unlocked_ || !valid(ssid, 33, false) || !valid(password, 65, true)) return false;
    uint8_t record[kRecordBytes] = {}, plain[98] = {};
    memcpy(record, "VA01", 4); memcpy(record + 4, salt_, 16);
    strcpy(reinterpret_cast<char*>(plain), ssid);
    strcpy(reinterpret_cast<char*>(plain + 33), password);
    bool ok = random_bytes(record + 20, 12) && crypt(true, key_, record, plain);
    if (ok) ok = blob_.write(record, sizeof(record));
    wipe(record, sizeof(record)); wipe(plain, sizeof(plain));
    return ok;
}
bool CredentialVault::load(char (&ssid)[33], char (&password)[65]) {
    wipe(ssid, sizeof(ssid)); wipe(password, sizeof(password));
    if (!unlocked_) return false;
    uint8_t record[kRecordBytes] = {}, plain[98] = {};
    bool ok = blob_.read(record, sizeof(record)) == static_cast<int>(sizeof(record));
    ok = ok && !memcmp(record, "VA01", 4) && !memcmp(record + 4, salt_, 16) && crypt(false, key_, record, plain);
    ok = ok && valid(reinterpret_cast<char*>(plain), 33, false) && valid(reinterpret_cast<char*>(plain + 33), 65, true);
    if (ok) { memcpy(ssid, plain, 33); memcpy(password, plain + 33, 65); }
    wipe(record, sizeof(record)); wipe(plain, sizeof(plain));
    return ok;
}
bool CredentialVault::erase() { lock(); return blob_.erase(); }
} // namespace vamc
