#pragma once

#include "command_ids.h"
#include "status_codes.h"
#include <stdint.h>

namespace vamc {

// Upper bound latency budget per VAMC-F-008 requirement (<= 2 seconds)
constexpr uint32_t kMaxBleLatencyBudgetMs = 2000;

// Maximum retry limit per VAMC-F-009 specification
constexpr uint8_t kMaxAllowedRetries = 3;

// Base retry backoff time in milliseconds
constexpr uint32_t kDefaultBackoffMs = 50;

// Hardware/Protocol abstraction interface for BLE HID transmission
class IBleHidTransport {
public:
    virtual ~IBleHidTransport() = default;
    virtual bool is_connected() const = 0;
    virtual bool send_consumer_key(uint16_t usage_code) = 0;
    virtual uint32_t get_time_ms() const = 0;
    virtual void sleep_ms(uint32_t ms) = 0;
};

// Register an active transport (e.g., ESP32 NimBLE HID or mock for testing)
void ble_set_transport(IBleHidTransport* transport);

// Retrieve currently active transport
IBleHidTransport* ble_get_transport();

// Retrieve the most recent error code
BleError ble_get_last_error();

// Transmits recognized command as a BLE HID media key event per SAD 4.3.
// Returns SendResult containing execution status and latency in milliseconds.
SendResult ble_send_hid_key(CommandId command_id, uint8_t retry_count = 0);

} // namespace vamc
