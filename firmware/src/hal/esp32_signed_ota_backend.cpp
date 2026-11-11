#include "esp32_signed_ota_backend.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <Update.h>
#include <esp_system.h>

#if defined(UPDATE_SIGN) && defined(VAMC_OTA_PUBLIC_KEY) && \
    defined(VAMC_OTA_PUBLIC_KEY_LEN)
#define VAMC_SIGNED_OTA_CONFIGURED 1
#include <Updater_Signing.h>
#endif

namespace vamc {

bool Esp32SignedOtaBackend::enable_signature_verification() {
    signature_installed_ = false;
#if defined(VAMC_SIGNED_OTA_CONFIGURED)
    // Keep the verifier alive for the whole update; Update retains its pointer.
    static UpdaterRSAVerifier verifier(VAMC_OTA_PUBLIC_KEY,
                                       VAMC_OTA_PUBLIC_KEY_LEN);
    signature_installed_ = Update.installSignature(&verifier);
#endif
    return signature_installed_;
}

bool Esp32SignedOtaBackend::begin(uint32_t image_size_bytes) {
    return signature_installed_ && image_size_bytes > 0 &&
           Update.begin(image_size_bytes, U_FLASH);
}

size_t Esp32SignedOtaBackend::write(const uint8_t* data, size_t length) {
    if (!signature_installed_ || data == nullptr) return 0;
    return Update.write(const_cast<uint8_t*>(data), length);
}

OtaFinalizeResult Esp32SignedOtaBackend::finish_signed_update() {
    if (!signature_installed_) return OtaFinalizeResult::SIGNATURE_REJECTED;
    if (Update.end(true)) return OtaFinalizeResult::VERIFIED_AND_ACTIVATED;
    if (Update.getError() == UPDATE_ERROR_SIGN)
        return OtaFinalizeResult::SIGNATURE_REJECTED;
    return OtaFinalizeResult::ACTIVATION_FAILED;
}

void Esp32SignedOtaBackend::abort() {
    if (Update.isRunning()) Update.abort();
    signature_installed_ = false;
}

void Esp32SignedOtaBackend::reboot() {
    ESP.restart();
}

} // namespace vamc
#endif // ARDUINO_ARCH_ESP32
