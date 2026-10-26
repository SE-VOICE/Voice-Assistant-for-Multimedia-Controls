#include "factory_reset.h"

namespace vamc {

FactoryResetResult FactoryReset::on_confirmed_long_press() {
    // Try both erasures even if the first fails. Provisioning may restart only
    // after both stores report success; a partial wipe is never called success.
    const bool wifi_cleared = storage_.clear_wifi_credentials();
    const bool ble_cleared = storage_.clear_ble_pairing_data();
    if (!wifi_cleared || !ble_cleared) {
        return FactoryResetResult::CLEAR_FAILED;
    }
    if (!storage_.restart_provisioning()) {
        return FactoryResetResult::PROVISIONING_RESTART_FAILED;
    }
    return FactoryResetResult::SUCCESS;
}

} // namespace vamc
