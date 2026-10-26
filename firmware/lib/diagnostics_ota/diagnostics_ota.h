#pragma once
#include "command_ids.h"
#include <stdint.h>
namespace vamc {
// Records have no text/audio/credential fields. Error code catalogue remains
// owned by the diagnostics story; no new error code values are assigned here.
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
// Hosting, signature verification and storage policy are pending decisions.
class OtaUpdater;
} // namespace vamc
