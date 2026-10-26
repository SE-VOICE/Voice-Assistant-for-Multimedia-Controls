#pragma once

#include "command_ids.h"
#include <stdint.h>
#include <stddef.h>

namespace vamc {

namespace hid_usage {
    // USB HID Usage Tables for Consumer Devices (Consumer Page 0x0C)
    constexpr uint16_t NONE             = 0x0000;
    constexpr uint16_t VOLUME_INCREMENT = 0x00E9;
    constexpr uint16_t VOLUME_DECREMENT = 0x00EA;
    constexpr uint16_t PLAY             = 0x00B0;
    constexpr uint16_t PAUSE            = 0x00B1;
    constexpr uint16_t PLAY_PAUSE       = 0x00CD;
    constexpr uint16_t SCAN_NEXT_TRACK  = 0x00B5;
    constexpr uint16_t SCAN_PREV_TRACK  = 0x00B6;
    constexpr uint16_t MUTE             = 0x00E2;
    constexpr uint16_t STOP             = 0x00B7;
} // namespace hid_usage

class CommandMapper {
public:
    // Total count of supported multimedia commands per VAMC-F-007
    static constexpr size_t kSupportedCommandCount = 7;

    // Maps a recognized CommandId to its corresponding BLE/USB HID Consumer Control usage code.
    // Returns hid_usage::NONE (0x0000) for unknown or out-of-range commands.
    static uint16_t to_hid_usage(CommandId id);

    // Validates whether the given CommandId is one of the 7 supported commands.
    static bool is_supported(CommandId id);

    // Returns a canonical string representation of the CommandId.
    static const char* to_string(CommandId id);

    // Parses a text string to a CommandId (case-insensitive with synonym support).
    // Returns true on success, false if unmatched.
    static bool from_string(const char* str, CommandId& out_id);
};

} // namespace vamc
