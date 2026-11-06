#pragma once

#include "command_ids.h"
#include "status_codes.h"
#include <stdint.h>

namespace vamc {

// Upper bound latency budget per VAMC-F-008 requirement (<= 2 seconds)
constexpr uint32_t kMaxBleLatencyBudgetMs = 2000;

// Maximum retry limit per VAMC-F-009 specification
constexpr uint8_t kMaxAllowedRetries = 3;

// Retry timing can be tuned per build without changing the BLE module.
#ifndef VAMC_BLE_RETRY_BACKOFF_BASE_MS
#define VAMC_BLE_RETRY_BACKOFF_BASE_MS 50
#endif
#ifndef VAMC_BLE_RETRY_BACKOFF_MULTIPLIER
#define VAMC_BLE_RETRY_BACKOFF_MULTIPLIER 2
#endif
constexpr uint32_t kDefaultBackoffMs = VAMC_BLE_RETRY_BACKOFF_BASE_MS;
constexpr uint8_t kRetryBackoffMultiplier = VAMC_BLE_RETRY_BACKOFF_MULTIPLIER;
static_assert(kDefaultBackoffMs > 0, "BLE retry backoff must be non-zero");
static_assert(kRetryBackoffMultiplier > 0, "BLE retry multiplier must be non-zero");

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

// Transmits a recognized command as a BLE HID media key event per SAD 4.3.
// The default matches VAMC-F-009: one initial attempt plus up to three retries.
// Pass a smaller retry_count for callers with a tighter retry policy.
// Returns SendResult containing execution status and latency in milliseconds.
SendResult ble_send_hid_key(CommandId command_id,
                            uint8_t retry_count = kMaxAllowedRetries);

} // namespace vamc
