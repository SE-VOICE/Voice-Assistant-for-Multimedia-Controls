#include "wifi_provisioning.h"
#include <string.h>

namespace vamc {
namespace {
bool fits(const char* text, size_t capacity) {
    if (!text) return false;
    for (size_t i = 0; i < capacity; ++i) if (!text[i]) return true;
    return false;
}
}
void WifiProvisioning::clear_pending() {
    volatile char* a = pending_ssid_;
    volatile char* b = pending_password_;
    for (size_t i = 0; i < sizeof(pending_ssid_); ++i) a[i] = 0;
    for (size_t i = 0; i < sizeof(pending_password_); ++i) b[i] = 0;
    connecting_ = false;
}
void WifiProvisioning::cancel() {
    if (active_) { connection_.cancel(); portal_.stop(); }
    active_ = false;
    clear_pending();
}
WifiProvisioningResult WifiProvisioning::start(unsigned long now_ms) {
    cancel();
    started_at_ms_ = now_ms;
    active_ = portal_.start();
    return active_ ? WifiProvisioningResult::SUCCESS : WifiProvisioningResult::PORTAL_START_FAILED;
}
WifiProvisioningResult WifiProvisioning::poll(unsigned long now_ms) {
    if (!active_) return WifiProvisioningResult::INACTIVE;
    if (static_cast<unsigned long>(now_ms - started_at_ms_) >= kConnectionTimeoutMs) {
        cancel();
        return WifiProvisioningResult::CONNECTION_TIMEOUT;
    }
    if (connecting_ && connection_.connected()) {
        const bool saved = credential_store_.save_protected(pending_ssid_, pending_password_);
        portal_.stop();
        active_ = false;
        clear_pending();
        return saved ? WifiProvisioningResult::SUCCESS : WifiProvisioningResult::CREDENTIAL_SAVE_FAILED;
    }
    if (!connecting_) {
        WifiCredentials credentials{nullptr, nullptr};
        if (!portal_.poll_submission(credentials)) return WifiProvisioningResult::SUCCESS;
        if (!fits(credentials.ssid, sizeof(pending_ssid_)) || !credentials.ssid[0] ||
            !fits(credentials.password, sizeof(pending_password_)))
            return WifiProvisioningResult::INVALID_CREDENTIALS;
        strcpy(pending_ssid_, credentials.ssid);
        strcpy(pending_password_, credentials.password);
        if (!connection_.begin(pending_ssid_, pending_password_)) {
            clear_pending();
            return WifiProvisioningResult::CONNECTION_START_FAILED;
        }
        connecting_ = true;
    }
    return WifiProvisioningResult::SUCCESS;
}
} // namespace vamc
