// SR-003 bench example only: no microphone, portal, BLE or media-control claim.
// Copy credential_vault.cpp/.h, wifi_provisioning.h and nvs_credential_blob.h
// beside this sketch before compiling. Do not enter credentials in source.
#include <Arduino.h>
#include <WiFi.h>
#include "credential_vault.h"
#include "nvs_credential_blob.h"

vamc::NvsCredentialBlob storage;
vamc::CredentialVault vault(storage);
char passphrase[129] = {};
size_t used = 0;
bool overflow = false;
bool ready = false;

void clear_input() {
    volatile char* p = passphrase;
    for (size_t i = 0; i < sizeof(passphrase); ++i) p[i] = 0;
    used = 0;
    overflow = false;
}

void setup() {
    Serial.begin(115200);
    // Radio initialization supplies ESP32's hardware RNG entropy. Prevent SDK
    // storage of another plaintext credential copy before any WiFi.begin call.
    WiFi.persistent(false);
    if (!WiFi.mode(WIFI_STA) || !storage.begin()) {
        Serial.println("Storage/radio initialization failed.");
        return;
    }
    ready = true;
    Serial.println("Enter boot passphrase, then newline (input is not echoed).");
}

void loop() {
    if (!ready) { yield(); return; }
    while (Serial.available()) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r') continue;
        if (c == '\n') {
            if (!used && !overflow) continue;
            const bool ok = !overflow && vault.unlock(passphrase);
            clear_input();
            Serial.println(ok ? "Vault unlocked; key in RAM only." : "Unlock failed; retry.");
            // Integrator can now pass vault to WifiProvisioning. This example
            // never prints stored credentials or initiates a Wi-Fi connection.
        } else if (c == '\b' || c == 127) {
            if (used && !overflow) passphrase[--used] = 0;
        } else if (static_cast<unsigned char>(c) >= 32) {
            if (used < sizeof(passphrase) - 1 && !overflow) passphrase[used++] = c;
            else overflow = true; // Discard until newline; never truncate a key.
        }
    }
    yield();
}
