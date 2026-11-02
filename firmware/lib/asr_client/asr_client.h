#pragma once

#include "secure_channel.h"
#include <stdint.h>
#include <stddef.h>

namespace vamc {

// VAMC-F-004: stream captured audio to a cloud ASR service over HTTPS for
// transcription (SAD 4.3, Interface 1).
//
// AsrClient sends one captured utterance (16 kHz, 16-bit mono PCM) to the
// provider over SecureChannel (VAMC-SR-001), streaming the request body in
// small chunks so the base64 copy of the audio never has to fit in RAM, and
// returns the transcript and confidence. It does not decide what the
// transcript means: callers pass an OK result to CommandGate (VAMC-F-006,
// evaluate_transcript) and any other status to CommandGate::report_asr_failure.

namespace asr_limits {
    constexpr size_t kMaxTranscriptChars = 128;    // matches the F-005 matcher limit
    constexpr size_t kMaxResponseBytes = 4096;      // status line + headers + body
    constexpr size_t kMaxHeaderLineChars = 160;     // provider extra header
    constexpr uint32_t kLatencyBudgetMs = 3000;     // AC-VAMC-F-004 round trip
    constexpr uint32_t kDefaultTimeoutMs = 6000;    // hard abort for one request
    constexpr size_t kBodyChunkInputBytes = 384;    // PCM bytes per base64 chunk
} // namespace asr_limits

// Monotonic millisecond clock, provided by the platform (millis() on Arduino).
class IClock {
public:
    virtual ~IClock() = default;
    virtual uint32_t now_ms() const = 0;
};

// Provider-specific request and response format. The transport, TLS policy,
// streaming and error mapping are shared; a new provider only implements this.
class IAsrProvider {
public:
    virtual ~IAsrProvider() = default;
    // Full https:// URL. Credentials should go in extra_header(), not here.
    virtual const char* url() const = 0;
    virtual const char* content_type() const = 0;
    // One extra request header line without CRLF (e.g. an API key header),
    // or nullptr.
    virtual const char* extra_header() const = 0;
    // Body = body_prefix() + audio + body_suffix(). When audio_as_base64()
    // is true the audio is base64-encoded (JSON providers); otherwise the raw
    // PCM bytes are sent (binary-upload providers).
    virtual bool audio_as_base64() const = 0;
    virtual const char* body_prefix() const = 0;
    virtual const char* body_suffix() const = 0;

    enum class ParseResult : uint8_t { TRANSCRIPT, NO_SPEECH, TOO_LONG, MALFORMED };
    // Parses a 2xx response body. On TRANSCRIPT, writes a NUL-terminated
    // transcript (at most capacity-1 chars) and sets `confidence` to a value
    // in [0, 1] or to a negative value if the provider gave none.
    virtual ParseResult parse_response(const char* body, size_t len, char* transcript,
                                       size_t capacity, float& confidence) const = 0;
};

enum class AsrStatus : uint8_t {
    OK,                   // transcript and confidence are valid
    NO_SPEECH,            // the service heard nothing it could transcribe
    TRANSCRIPT_TOO_LONG,  // longer than kMaxTranscriptChars; not a command
    BAD_INPUT,            // no audio, odd byte count or longer than kMaxCaptureMs
    NOT_CONFIGURED,       // provider URL or header invalid
    CHANNEL_REFUSED,      // SR-001 refused the connection; see channel_status
    NETWORK_ERROR,        // write/read failed or connection dropped early
    TIMEOUT,              // no complete response within the timeout
    AUTH_FAILED,          // HTTP 401 / 403
    RATE_LIMITED,         // HTTP 429
    SERVICE_UNAVAILABLE,  // HTTP 5xx
    HTTP_ERROR,           // any other non-2xx status
    BAD_RESPONSE          // unparseable or oversized response
};

struct AsrResult {
    AsrStatus status;
    char transcript[asr_limits::kMaxTranscriptChars + 1];
    float confidence;            // [0, 1], or negative when not provided
    uint16_t http_status;        // 0 when no HTTP response was read
    ChannelStatus channel_status;
    uint32_t latency_ms;         // request start to response parsed

    bool ok() const { return status == AsrStatus::OK; }
    bool within_latency_budget() const { return latency_ms < asr_limits::kLatencyBudgetMs; }
};

class AsrClient {
public:
    AsrClient(SecureChannel& channel, const IAsrProvider& provider, const IClock& clock,
              uint32_t timeout_ms = asr_limits::kDefaultTimeoutMs)
        : channel_(channel), provider_(provider), clock_(clock), timeout_ms_(timeout_ms) {}

    // Transcribes one utterance of 16 kHz, 16-bit little-endian mono PCM.
    // Blocking; always closes the connection before returning.
    AsrResult transcribe(const uint8_t* pcm, size_t pcm_bytes);

private:
    AsrClient(const AsrClient&);
    AsrClient& operator=(const AsrClient&);

    AsrStatus send_request(const HttpsUrl& url, const uint8_t* pcm, size_t pcm_bytes,
                           uint32_t start_ms);
    AsrStatus read_response(AsrResult& result, uint32_t start_ms);
    bool expired(uint32_t start_ms) const;

    SecureChannel& channel_;
    const IAsrProvider& provider_;
    const IClock& clock_;
    uint32_t timeout_ms_;
    char response_[asr_limits::kMaxResponseBytes + 1];
};

namespace asr_detail {
    size_t base64_length(size_t input_bytes);
    // Writes base64_length(len) chars to `out` (no terminator). Returns count.
    size_t base64_encode(const uint8_t* in, size_t len, char* out);
    // Decodes an HTTP/1.1 chunked body in place. Returns false if malformed
    // or incomplete; on success `len` is the decoded length.
    bool dechunk_in_place(char* body, size_t& len);
} // namespace asr_detail

} // namespace vamc
