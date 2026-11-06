#pragma once
#include "wifi_provisioning.h"
#include <stdint.h>

namespace vamc {
class EncryptedCredentialBlob {
public:
    virtual ~EncryptedCredentialBlob() = default;
    // -1 = storage error, 0 = absent, otherwise exact byte count.
    virtual int read(uint8_t* destination, size_t capacity) = 0;
    virtual bool write(const uint8_t* data, size_t length) = 0;
    virtual bool erase() = 0;
};

// Key/passphrase never serialized. Caller must clear the passphrase input.
class CredentialVault : public WifiCredentialStore {
public:
    static constexpr size_t kRecordBytes = 146;
    explicit CredentialVault(EncryptedCredentialBlob& blob) : blob_(blob) {}
    ~CredentialVault() { lock(); }
    CredentialVault(const CredentialVault&) = delete;
    CredentialVault& operator=(const CredentialVault&) = delete;
    bool unlock(const char* passphrase);
    void lock();
    bool unlocked() const { return unlocked_; }
    bool save_protected(const char* ssid, const char* password) override;
    bool load(char (&ssid)[33], char (&password)[65]);
    bool erase();
private:
    EncryptedCredentialBlob& blob_;
    uint8_t key_[32] = {};
    uint8_t salt_[16] = {};
    bool unlocked_ = false;
};
} // namespace vamc
