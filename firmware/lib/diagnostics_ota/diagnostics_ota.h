#pragma once

#include "command_ids.h"
#include <stdint.h>
#include <stddef.h>

namespace vamc {

constexpr size_t kDefaultLogCapacity = 32;

// Standard diagnostic error codes catalogue
namespace error_codes {
    constexpr uint16_t NONE               = 0x0000;
    constexpr uint16_t WAKE_WORD_TIMEOUT  = 0x0001;
    constexpr uint16_t AUDIO_CAPTURE_FAIL = 0x0002;
    constexpr uint16_t ASR_UNRECOGNIZED   = 0x0003;
    constexpr uint16_t ASR_NETWORK_ERROR  = 0x0004;
    constexpr uint16_t BLE_DISCONNECTED   = 0x0005;
    constexpr uint16_t BLE_TIMEOUT        = 0x0006;
    constexpr uint16_t BLE_SEND_FAILED    = 0x0007;
} // namespace error_codes

// Records have NO text, audio, or credential fields per VAMC-F-014 and SR-003.
struct LogEntry {
    CommandId command;
    uint32_t timestamp;
    uint16_t error_code;
};

class DiagnosticLog {
public:
    virtual ~DiagnosticLog() = default;
    virtual void append(const LogEntry& entry) = 0;
};

// Fixed-size circular buffer for storing last N diagnostic log entries
class CircularLogBuffer : public DiagnosticLog {
public:
    explicit CircularLogBuffer(size_t capacity = kDefaultLogCapacity);
    ~CircularLogBuffer() override;

    // Appends entry, overwriting oldest if capacity is reached
    void append(const LogEntry& entry) override;

    size_t count() const { return count_; }
    size_t capacity() const { return capacity_; }
    bool is_empty() const { return count_ == 0; }
    bool is_full() const { return count_ == capacity_; }
    void clear();

    // Retrieves an entry by chronological order:
    // index 0 is oldest stored event, index (count - 1) is newest event.
    bool get_entry(size_t index, LogEntry& out_entry) const;

    // Formats a log entry into a human-readable string: "[TS: <timestamp> ms] CMD: <name>, ERR: 0x<hex>"
    static size_t format_entry(const LogEntry& entry, char* out_buf, size_t buf_size);

    // Dumps all stored events in chronological order to a print callback
    void dump(void (*print_fn)(const char* line)) const;

private:
    size_t capacity_;
    size_t count_;
    size_t head_; // points to next write index
    LogEntry* buffer_;
};

// OTA interfaces keep the portable update policy independent of the selected
// network host and ESP32 board. The platform backend must fail closed when it
// cannot install a real signature verifier.
constexpr size_t kOtaTransferBufferBytes = 512;

struct OtaImageInfo {
    uint32_t size_bytes;
    uint32_t version;
};

enum class OtaFinalizeResult : uint8_t {
    VERIFIED_AND_ACTIVATED,
    SIGNATURE_REJECTED,
    ACTIVATION_FAILED
};

enum class OtaResult : uint8_t {
    SUCCESS,
    SOURCE_UNAVAILABLE,
    INVALID_IMAGE_SIZE,
    SIGNATURE_VERIFIER_UNAVAILABLE,
    UPDATE_BEGIN_FAILED,
    SOURCE_READ_FAILED,
    UPDATE_WRITE_FAILED,
    SIGNATURE_REJECTED,
    UPDATE_ACTIVATION_FAILED
};

class IOtaFirmwareSource {
public:
    virtual ~IOtaFirmwareSource() = default;
    // Opens a remote or test source and returns the exact signed image size.
    virtual bool open(OtaImageInfo& image_info) = 0;
    // Reads up to capacity bytes; a zero result before size_bytes is exhausted
    // is a transfer failure, not a successful short image.
    virtual size_t read(uint8_t* destination, size_t capacity) = 0;
    virtual bool failed() const = 0;
    virtual void close() = 0;
};

class IOtaUpdateBackend {
public:
    virtual ~IOtaUpdateBackend() = default;
    // Must install a genuine signature verifier before begin(). Return false
    // when signing support or a trusted public key is unavailable.
    virtual bool enable_signature_verification() = 0;
    // Stages bytes in an inactive update slot; must not select it for boot.
    virtual bool begin(uint32_t image_size_bytes) = 0;
    virtual size_t write(const uint8_t* data, size_t length) = 0;
    // Verifies the signed image and activates it only after successful verify.
    virtual OtaFinalizeResult finish_signed_update() = 0;
    virtual void abort() = 0;
    virtual void reboot() = 0;
};

class OtaUpdater {
public:
    OtaResult apply(IOtaFirmwareSource& source, IOtaUpdateBackend& backend) const;
};

} // namespace vamc
