#pragma once
#include <stdint.h>
namespace vamc {
enum class SendStatus : uint8_t { SUCCESS, RETRIED, FAILED };
enum class BleError : uint8_t { BLE_NOT_CONNECTED, BLE_TIMEOUT };
struct SendResult {
    SendStatus status;
    uint32_t latency_ms;
};
} // namespace vamc
