#pragma once
#include <stdint.h>
namespace vamc {
namespace config {
// SRS F-002 upper bound, not a fixed capture duration.
constexpr uint32_t kMaxCaptureMs = 5000;
// SAD 4.3 audio format. No buffers are allocated by this configuration.
constexpr uint32_t kSampleRateHz = 16000;
constexpr uint8_t kSampleBits = 16;
} // namespace config
} // namespace vamc
