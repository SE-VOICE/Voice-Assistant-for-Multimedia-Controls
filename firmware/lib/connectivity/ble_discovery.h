#pragma once

#include <stdint.h>
#include "authenticated_pairing.h"

namespace vamc {

// Implemented by the board-specific BLE stack. The device name and advertising
// parameters are supplied by that adapter; this portable layer invents neither.
class BleAdvertisement {
public:
    virtual ~BleAdvertisement() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
};

enum class BleSetupResult : uint8_t {
    SUCCESS,
    ALREADY_ACTIVE,
    ADVERTISING_FAILED,
    PAIRING_FAILED
};

// Coordinates host discovery with the authenticated passkey pairing policy.
class BlePairingSetup {
public:
    BlePairingSetup(BleAdvertisement& advertisement, AuthenticatedPairing& pairing)
        : advertisement_(advertisement), pairing_(pairing), advertising_(false) {}

    BleSetupResult start_discoverable();
    BleSetupResult begin_pairing(uint32_t passkey);
    void on_host_result(bool bonded, PairingMethod method);
    bool advertising() const { return advertising_; }
    PairingState pairing_state() const { return pairing_.state(); }

private:
    BleAdvertisement& advertisement_;
    AuthenticatedPairing& pairing_;
    bool advertising_;
};

} // namespace vamc
