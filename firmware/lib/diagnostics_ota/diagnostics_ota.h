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

// Hosting, signature verification and storage policy are pending decisions.
class OtaUpdater;

} // namespace vamc
