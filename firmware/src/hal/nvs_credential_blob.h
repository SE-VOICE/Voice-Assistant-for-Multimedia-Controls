#pragma once
#if defined(ARDUINO_ARCH_ESP32)
#include <Preferences.h>
#include "credential_vault.h"
namespace vamc {
class NvsCredentialBlob : public EncryptedCredentialBlob {
public:
    bool begin() { return open_ = preferences_.begin("vamc_sec", false); }
    int read(uint8_t* out, size_t capacity) override {
        if (!open_) return -1;
        const size_t length = preferences_.getBytesLength("wifi");
        if (!length) return 0;
        if (length > capacity) return -1;
        return preferences_.getBytes("wifi", out, length) == length ? int(length) : -1;
    }
    bool write(const uint8_t* data, size_t length) override {
        return open_ && length == CredentialVault::kRecordBytes &&
            preferences_.putBytes("wifi", data, length) == length;
    }
    bool erase() override {
        return open_ && (!preferences_.isKey("wifi") || preferences_.remove("wifi"));
    }
private:
    Preferences preferences_;
    bool open_ = false;
};
}
#endif
