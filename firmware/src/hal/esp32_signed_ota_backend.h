#pragma once

#include "diagnostics_ota.h"

namespace vamc {

// Adapter for Arduino-ESP32's signed Update API. Signature verification is
// unavailable unless UPDATE_SIGN and the project's trusted public key are
// supplied at build time. The private signing key must never be part of this
// repository or device firmware.
class Esp32SignedOtaBackend : public IOtaUpdateBackend {
public:
    bool enable_signature_verification() override;
    bool begin(uint32_t image_size_bytes) override;
    size_t write(const uint8_t* data, size_t length) override;
    OtaFinalizeResult finish_signed_update() override;
    void abort() override;
    void reboot() override;

private:
    bool signature_installed_ = false;
};

} // namespace vamc
