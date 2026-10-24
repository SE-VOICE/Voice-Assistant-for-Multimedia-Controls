#pragma once
#include <stdint.h>
namespace vamc {
// Symbolic IDs only; these numeric values are not HID usages or a wire protocol.
enum class CommandId : uint8_t {
    VOLUME_UP, VOLUME_DOWN, PAUSE, RESUME, PLAY, NEXT, PREVIOUS
};
struct CommandRequest {
    CommandId command;
    uint32_t timestamp; // milliseconds from the platform monotonic clock
};
} // namespace vamc
