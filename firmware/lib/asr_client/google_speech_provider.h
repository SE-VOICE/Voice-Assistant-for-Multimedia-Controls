#pragma once

#include "asr_client.h"
#include <stddef.h>

namespace vamc {

// Google Cloud Speech-to-Text v1 `speech:recognize`, as specified in SAD 4.3
// Interface 1: LINEAR16, 16 kHz, base64 audio in a JSON body; response
// results[].alternatives[0].{transcript, confidence}.
//
// The API key is sent in the X-Goog-Api-Key header, never in the URL, so it
// does not appear in URLs or request lines. Where the key is stored is
// VAMC-SR-003 and is not decided here; the caller passes it in.
class GoogleSpeechV1Provider : public IAsrProvider {
public:
    static constexpr const char* kUrl = "https://speech.googleapis.com/v1/speech:recognize";
    static constexpr size_t kMaxApiKeyChars = 64;
    static constexpr size_t kMaxLanguageChars = 15;

    // Copies `api_key` and `language_code` (BCP-47, e.g. "en-US", "en-IN").
    // Invalid values leave the provider unconfigured: url() returns "" and
    // the client reports NOT_CONFIGURED without connecting.
    explicit GoogleSpeechV1Provider(const char* api_key, const char* language_code = "en-US");

    bool configured() const { return configured_; }

    const char* url() const override { return configured_ ? kUrl : ""; }
    const char* content_type() const override { return "application/json; charset=utf-8"; }
    const char* extra_header() const override { return configured_ ? header_ : nullptr; }
    bool audio_as_base64() const override { return true; }
    const char* body_prefix() const override { return prefix_; }
    const char* body_suffix() const override { return "\"}}"; }
    ParseResult parse_response(const char* body, size_t len, char* transcript,
                               size_t capacity, float& confidence) const override;

private:
    bool configured_;
    char header_[16 + kMaxApiKeyChars + 1];
    char prefix_[160];
};

} // namespace vamc
