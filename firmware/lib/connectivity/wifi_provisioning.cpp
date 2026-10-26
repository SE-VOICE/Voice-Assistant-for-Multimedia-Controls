#include "wifi_provisioning.h"

namespace vamc {

WifiProvisioningResult WifiProvisioning::start(unsigned long now_ms) {
    connecting_ = false;
    pending_ssid_ = nullptr;
    pending_password_ = nullptr;
    started_at_ms_ = now_ms;
    if (!portal_.start()) {
        return WifiProvisioningResult::PORTAL_START_FAILED;
    }
    return WifiProvisioningResult::SUCCESS;
}

WifiProvisioningResult WifiProvisioning::poll(unsigned long now_ms) {
    if (connection_.connected()) {
        if (connecting_ && !credential_store_.save_protected(pending_ssid_, pending_password_)) {
            portal_.stop();
            pending_ssid_ = nullptr;
            pending_password_ = nullptr;
            connecting_ = false;
            return WifiProvisioningResult::CREDENTIAL_SAVE_FAILED;
        }
        portal_.stop();
        pending_ssid_ = nullptr;
        pending_password_ = nullptr;
        connecting_ = false;
        return WifiProvisioningResult::SUCCESS;
    }

    if (static_cast<unsigned long>(now_ms - started_at_ms_) >= kConnectionTimeoutMs) {
        portal_.stop();
        return WifiProvisioningResult::CONNECTION_TIMEOUT;
    }

    if (!connecting_) {
        WifiCredentials credentials{nullptr, nullptr};
        if (!portal_.poll_submission(credentials)) {
            return WifiProvisioningResult::SUCCESS;
        }
        if (credentials.ssid == nullptr || credentials.ssid[0] == '\0' ||
            credentials.password == nullptr) {
            return WifiProvisioningResult::INVALID_CREDENTIALS;
        }
        pending_ssid_ = credentials.ssid;
        pending_password_ = credentials.password;
        if (!connection_.begin(pending_ssid_, pending_password_)) {
            pending_ssid_ = nullptr;
            pending_password_ = nullptr;
            return WifiProvisioningResult::CONNECTION_START_FAILED;
        }
        connecting_ = true;
    }

    return WifiProvisioningResult::SUCCESS;
}

} // namespace vamc
