#pragma once

namespace vamc {

// The GPIO adapter recognizes a long press. Its duration is a board/setup
// decision and is deliberately outside this portable module.
class FactoryResetStorage {
public:
    virtual ~FactoryResetStorage() = default;
    virtual bool clear_wifi_credentials() = 0;
    virtual bool clear_ble_pairing_data() = 0;
    virtual bool restart_provisioning() = 0;
};

enum class FactoryResetResult {
    SUCCESS,
    CLEAR_FAILED,
    PROVISIONING_RESTART_FAILED
};

class FactoryReset {
public:
    explicit FactoryReset(FactoryResetStorage& storage) : storage_(storage) {}
    FactoryResetResult on_confirmed_long_press();

private:
    FactoryResetStorage& storage_;
};

} // namespace vamc
