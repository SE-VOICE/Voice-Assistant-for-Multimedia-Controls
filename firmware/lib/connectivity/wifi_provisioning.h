#pragma once

#include <stddef.h>

namespace vamc {

// F-011's portable setup coordinator. Network, portal and credential storage
// stay behind adapters so host tests need neither ESP32 hardware nor secrets.
struct WifiCredentials {
    const char* ssid;
    const char* password;
};

class WifiProvisioningPortal {
public:
    virtual ~WifiProvisioningPortal() = default;
    virtual bool start() = 0;
    // Non-blocking poll: true means credentials were submitted this call.
    virtual bool poll_submission(WifiCredentials& credentials) = 0;
    virtual void stop() = 0;
};

class WifiConnection {
public:
    virtual ~WifiConnection() = default;
    virtual bool begin(const char* ssid, const char* password) = 0;
    virtual bool connected() const = 0;
    virtual void cancel() {}
};

// Persistence is intentionally an interface only. SR-003's storage protection
// and key-handling design is unapproved; this story does not store credentials.
class WifiCredentialStore {
public:
    virtual ~WifiCredentialStore() = default;
    virtual bool save_protected(const char* ssid, const char* password) = 0;
};

enum class WifiProvisioningResult {
    SUCCESS,
    PORTAL_START_FAILED,
    INVALID_CREDENTIALS,
    CONNECTION_START_FAILED,
    CONNECTION_TIMEOUT,
    CREDENTIAL_SAVE_FAILED,
    INACTIVE
};

class WifiProvisioning {
public:
    static constexpr unsigned long kConnectionTimeoutMs = 300000UL;

    WifiProvisioning(WifiProvisioningPortal& portal,
                     WifiConnection& connection,
                     WifiCredentialStore& credential_store)
        : portal_(portal), connection_(connection), credential_store_(credential_store),
          started_at_ms_(0), connecting_(false), active_(false), pending_ssid_{0}, pending_password_{0} {}

    ~WifiProvisioning() { clear_pending(); }

    WifiProvisioningResult start(unsigned long now_ms);
    WifiProvisioningResult poll(unsigned long now_ms);
    void cancel();

private:
    WifiProvisioningPortal& portal_;
    WifiConnection& connection_;
    WifiCredentialStore& credential_store_;
    unsigned long started_at_ms_;
    bool connecting_;
    bool active_;
    char pending_ssid_[33];
    char pending_password_[65];
    void clear_pending();
};

} // namespace vamc
