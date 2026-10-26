#include "authenticated_pairing.h"

namespace vamc {

bool AuthenticatedPairing::begin(uint32_t passkey) {
    if (state_ == PairingState::AWAITING_HOST_CONFIRMATION ||
        state_ == PairingState::PAIRED || passkey > 999999U) {
        return false;
    }
    // Do not start a pairing flow if the user cannot see the passkey.
    if (!presentation_.show_passkey(passkey)) {
        presentation_.clear_passkey();
        state_ = PairingState::REJECTED;
        return false;
    }
    if (!radio_.begin_passkey_pairing(passkey)) {
        presentation_.clear_passkey();
        state_ = PairingState::REJECTED;
        return false;
    }
    state_ = PairingState::AWAITING_HOST_CONFIRMATION;
    return true;
}

void AuthenticatedPairing::on_host_result(bool bonded, PairingMethod method) {
    if (state_ != PairingState::AWAITING_HOST_CONFIRMATION) {
        return;
    }
    presentation_.clear_passkey();
    // A bond created through Just Works does not satisfy SR-002.
    if (bonded && method == PairingMethod::PASSKEY) {
        state_ = PairingState::PAIRED;
    } else {
        if (bonded) {
            radio_.erase_untrusted_bond();
        }
        state_ = PairingState::REJECTED;
    }
}

} // namespace vamc
