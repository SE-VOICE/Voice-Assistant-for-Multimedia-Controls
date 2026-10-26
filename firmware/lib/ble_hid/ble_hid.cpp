#include "ble_hid.h"
#include "command_map.h"

namespace vamc {

namespace {

// Default stub transport used when no custom or hardware driver is registered
class DefaultStubTransport : public IBleHidTransport {
public:
    bool is_connected() const override { return false; }
    bool send_consumer_key(uint16_t) override { return false; }
    uint32_t get_time_ms() const override { return 0; }
    void sleep_ms(uint32_t) override {}
};

DefaultStubTransport g_default_transport;
IBleHidTransport* g_active_transport = &g_default_transport;
BleError g_last_error = BleError::BLE_NOT_CONNECTED;

} // namespace

void ble_set_transport(IBleHidTransport* transport) {
    if (transport != nullptr) {
        g_active_transport = transport;
    } else {
        g_active_transport = &g_default_transport;
    }
}

IBleHidTransport* ble_get_transport() {
    return g_active_transport;
}

BleError ble_get_last_error() {
    return g_last_error;
}

SendResult ble_send_hid_key(CommandId command_id, uint8_t retry_count) {
    if (!CommandMapper::is_supported(command_id)) {
        g_last_error = BleError::BLE_TIMEOUT;
        return { SendStatus::FAILED, 0 };
    }

    IBleHidTransport* transport = ble_get_transport();
    if (!transport->is_connected()) {
        g_last_error = BleError::BLE_NOT_CONNECTED;
        return { SendStatus::FAILED, 0 };
    }

    const uint16_t usage_code = CommandMapper::to_hid_usage(command_id);
    const uint32_t start_ms = transport->get_time_ms();
    const uint8_t effective_retries = (retry_count > kMaxAllowedRetries) ? kMaxAllowedRetries : retry_count;

    for (uint8_t attempt = 0; attempt <= effective_retries; ++attempt) {
        if (attempt > 0) {
            uint32_t backoff_ms = kDefaultBackoffMs * attempt;
            uint32_t elapsed_before_sleep = transport->get_time_ms() - start_ms;

            if (elapsed_before_sleep + backoff_ms >= kMaxBleLatencyBudgetMs) {
                g_last_error = BleError::BLE_TIMEOUT;
                return { SendStatus::FAILED, elapsed_before_sleep };
            }
            transport->sleep_ms(backoff_ms);
        }

        bool success = transport->send_consumer_key(usage_code);
        uint32_t elapsed = transport->get_time_ms() - start_ms;

        if (success) {
            if (elapsed <= kMaxBleLatencyBudgetMs) {
                SendStatus status = (attempt == 0) ? SendStatus::SUCCESS : SendStatus::RETRIED;
                return { status, elapsed };
            } else {
                g_last_error = BleError::BLE_TIMEOUT;
                return { SendStatus::FAILED, elapsed };
            }
        }
    }

    uint32_t total_elapsed = transport->get_time_ms() - start_ms;
    g_last_error = BleError::BLE_TIMEOUT;
    return { SendStatus::FAILED, total_elapsed };
}

} // namespace vamc
