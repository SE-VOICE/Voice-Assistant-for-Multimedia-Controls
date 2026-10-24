#pragma once
#include "command_ids.h"
#include "status_codes.h"
namespace vamc {
// SAD 4.3 call contract only. No implementation or acknowledgement is implied.
// Retry interpretation and transport completion must be resolved before F-009.
SendResult ble_send_hid_key(CommandId command_id, uint8_t retry_count);
} // namespace vamc
