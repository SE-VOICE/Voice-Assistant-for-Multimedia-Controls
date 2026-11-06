#include "ble_discovery.h"

namespace vamc {

BleSetupResult BlePairingSetup::start_discoverable() {
    if (advertising_) {
        return BleSetupResult::ALREADY_ACTIVE;
    }
    if (!advertisement_.start()) {
        return BleSetupResult::ADVERTISING_FAILED;
    }
    advertising_ = true;
    return BleSetupResult::SUCCESS;
}

BleSetupResult BlePairingSetup::begin_pairing(uint32_t passkey) {
    if (!advertising_ || !pairing_.begin(passkey)) {
        return BleSetupResult::PAIRING_FAILED;
    }
    return BleSetupResult::SUCCESS;
}

void BlePairingSetup::on_host_result(bool bonded, PairingMethod method) {
    pairing_.on_host_result(bonded, method);
    if (pairing_.state() == PairingState::PAIRED ||
        pairing_.state() == PairingState::REJECTED) {
        advertisement_.stop();
        advertising_ = false;
    }
}

} // namespace vamc
