#include "google_speech_provider.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

namespace vamc {

constexpr const char* GoogleSpeechV1Provider::kUrl;
constexpr size_t GoogleSpeechV1Provider::kMaxApiKeyChars;
constexpr size_t GoogleSpeechV1Provider::kMaxLanguageChars;

namespace {

bool valid_chars(const char* s, size_t max_len, bool allow_underscore) {
    if (s == nullptr) return false;
    size_t n = 0;
    for (; s[n]; ++n) {
        char c = s[n];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '-' || (allow_underscore && c == '_');
        if (!ok || n >= max_len) return false;
    }
    return n > 0;
}

bool is_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// Returns the index just past the closing quote of the JSON string starting
// at body[start] == '"', or 0 if the string is unterminated.
size_t skip_string(const char* body, size_t len, size_t start) {
    for (size_t i = start + 1; i < len; ++i) {
        if (body[i] == '\\') {
            ++i;
        } else if (body[i] == '"') {
            return i + 1;
        }
    }
    return 0;
}

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Appends the decoded JSON string body[start+1 .. end-2] to out.
// Returns false on a bad escape or when out would overflow (sets overflow).
bool append_decoded(const char* body, size_t start, size_t end, char* out, size_t& used,
                    size_t capacity, bool& overflow) {
    for (size_t i = start + 1; i + 1 < end; ++i) {
        char c = body[i];
        if (c == '\\') {
            ++i;
            switch (body[i]) {
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case 'b': case 'f': case 'n': case 'r': case 't': c = ' '; break;
                case 'u': {
                    if (i + 4 >= end) return false;
                    int v = 0;
                    for (int k = 1; k <= 4; ++k) {
                        int h = hex_digit(body[i + k]);
                        if (h < 0) return false;
                        v = v * 16 + h;
                    }
                    i += 4;
                    c = (v >= 0x20 && v < 0x7F) ? static_cast<char>(v) : '?';
                    break;
                }
                default:
                    return false;
            }
        }
        if (used + 1 >= capacity) {
            overflow = true;
            return false;
        }
        out[used++] = c;
    }
    out[used] = '\0';
    return true;
}

// Parses a JSON number at body[i]; returns false if there is none.
bool parse_number(const char* body, size_t len, size_t i, float& value) {
    bool neg = false;
    if (i < len && body[i] == '-') {
        neg = true;
        ++i;
    }
    if (i >= len || body[i] < '0' || body[i] > '9') return false;
    double v = 0.0;
    for (; i < len && body[i] >= '0' && body[i] <= '9'; ++i) v = v * 10 + (body[i] - '0');
    if (i < len && body[i] == '.') {
        double scale = 0.1;
        for (++i; i < len && body[i] >= '0' && body[i] <= '9'; ++i, scale /= 10) {
            v += (body[i] - '0') * scale;
        }
    }
    if (i < len && (body[i] == 'e' || body[i] == 'E')) {
        ++i;
        bool eneg = false;
        if (i < len && (body[i] == '+' || body[i] == '-')) eneg = body[i++] == '-';
        int e = 0;
        for (; i < len && body[i] >= '0' && body[i] <= '9' && e < 100; ++i) e = e * 10 + (body[i] - '0');
        for (; e > 0; --e) v = eneg ? v / 10 : v * 10;
    }
    value = static_cast<float>(neg ? -v : v);
    return true;
}

bool key_equals(const char* body, size_t start, size_t end, const char* key) {
    size_t n = strlen(key);
    return end - start == n + 2 && memcmp(body + start + 1, key, n) == 0;
}

} // namespace

GoogleSpeechV1Provider::GoogleSpeechV1Provider(const char* api_key, const char* language_code)
    : configured_(false) {
    header_[0] = '\0';
    prefix_[0] = '\0';
    if (!valid_chars(api_key, kMaxApiKeyChars, true) ||
        !valid_chars(language_code, kMaxLanguageChars, false)) {
        return;
    }
    snprintf(header_, sizeof(header_), "X-Goog-Api-Key: %s", api_key);
    snprintf(prefix_, sizeof(prefix_),
             "{\"config\":{\"encoding\":\"LINEAR16\",\"sampleRateHertz\":%lu,"
             "\"languageCode\":\"%s\",\"maxAlternatives\":1},\"audio\":{\"content\":\"",
             static_cast<unsigned long>(config::kSampleRateHz), language_code);
    configured_ = true;
}

IAsrProvider::ParseResult GoogleSpeechV1Provider::parse_response(
    const char* body, size_t len, char* transcript, size_t capacity, float& confidence) const {
    confidence = -1.0f;
    if (transcript == nullptr || capacity == 0) return ParseResult::MALFORMED;
    transcript[0] = '\0';
    if (body == nullptr) return ParseResult::MALFORMED;

    size_t i = 0;
    while (i < len && is_space(body[i])) ++i;
    if (i >= len || body[i] != '{') return ParseResult::MALFORMED;

    // Walk the document. A string followed by ':' is a key; the value of a
    // "transcript" key is appended (one per result, as maxAlternatives is 1),
    // and the lowest "confidence" across results is kept.
    size_t used = 0;
    unsigned segments = 0;
    bool overflow = false;
    while (i < len) {
        if (body[i] != '"') {
            ++i;
            continue;
        }
        size_t key_start = i;
        size_t key_end = skip_string(body, len, i);
        if (key_end == 0) return ParseResult::MALFORMED;
        i = key_end;
        while (i < len && is_space(body[i])) ++i;
        if (i >= len || body[i] != ':') continue;  // a value, not a key
        ++i;
        while (i < len && is_space(body[i])) ++i;

        if (key_equals(body, key_start, key_end, "transcript")) {
            if (i >= len || body[i] != '"') return ParseResult::MALFORMED;
            size_t value_end = skip_string(body, len, i);
            if (value_end == 0) return ParseResult::MALFORMED;
            if (segments > 0 && used > 0 && body[i + 1] != ' ') {
                if (used + 1 >= capacity) return ParseResult::TOO_LONG;
                transcript[used++] = ' ';
                transcript[used] = '\0';
            }
            if (!append_decoded(body, i, value_end, transcript, used, capacity, overflow)) {
                return overflow ? ParseResult::TOO_LONG : ParseResult::MALFORMED;
            }
            ++segments;
            i = value_end;
        } else if (key_equals(body, key_start, key_end, "confidence")) {
            float c = 0.0f;
            if (!parse_number(body, len, i, c) || c < 0.0f) return ParseResult::MALFORMED;
            if (confidence < 0.0f || c < confidence) confidence = c;
        }
    }

    // Only whitespace transcribed counts as nothing heard.
    for (size_t k = 0; k < used; ++k) {
        if (!is_space(transcript[k])) return ParseResult::TRANSCRIPT;
    }
    transcript[0] = '\0';
    confidence = -1.0f;
    return ParseResult::NO_SPEECH;
}

} // namespace vamc
