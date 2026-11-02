#include "asr_client.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

namespace vamc {

namespace asr_detail {

size_t base64_length(size_t input_bytes) {
    return ((input_bytes + 2) / 3) * 4;
}

size_t base64_encode(const uint8_t* in, size_t len, char* out) {
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    size_t i = 0;
    for (; i + 3 <= len; i += 3) {
        uint32_t v = (static_cast<uint32_t>(in[i]) << 16) |
                     (static_cast<uint32_t>(in[i + 1]) << 8) | in[i + 2];
        out[o++] = kAlphabet[(v >> 18) & 0x3F];
        out[o++] = kAlphabet[(v >> 12) & 0x3F];
        out[o++] = kAlphabet[(v >> 6) & 0x3F];
        out[o++] = kAlphabet[v & 0x3F];
    }
    size_t rest = len - i;
    if (rest > 0) {
        uint32_t v = static_cast<uint32_t>(in[i]) << 16;
        if (rest == 2) v |= static_cast<uint32_t>(in[i + 1]) << 8;
        out[o++] = kAlphabet[(v >> 18) & 0x3F];
        out[o++] = kAlphabet[(v >> 12) & 0x3F];
        out[o++] = (rest == 2) ? kAlphabet[(v >> 6) & 0x3F] : '=';
        out[o++] = '=';
    }
    return o;
}

namespace {
int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
} // namespace

bool dechunk_in_place(char* body, size_t& len) {
    size_t r = 0;  // read position
    size_t w = 0;  // write position (always <= r)
    for (;;) {
        // Chunk size line: hex digits, optional ";extensions", CRLF.
        size_t size = 0;
        size_t digits = 0;
        while (r < len && hex_value(body[r]) >= 0) {
            size = size * 16 + static_cast<size_t>(hex_value(body[r]));
            if (size > len) return false;
            ++r;
            ++digits;
        }
        if (digits == 0) return false;
        while (r < len && body[r] != '\r') ++r;  // skip extensions
        if (r + 1 >= len || body[r + 1] != '\n') return false;
        r += 2;
        if (size == 0) break;  // last chunk (trailers are ignored)
        if (r + size + 2 > len) return false;
        memmove(body + w, body + r, size);
        w += size;
        r += size;
        if (body[r] != '\r' || body[r + 1] != '\n') return false;
        r += 2;
    }
    len = w;
    body[w] = '\0';
    return true;
}

} // namespace asr_detail

namespace {

bool has_line_break(const char* s) {
    for (; s && *s; ++s) {
        if (*s == '\r' || *s == '\n') return true;
    }
    return false;
}

char lower(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// Case-insensitive: does the header line [line, end) start with `name`?
bool header_is(const char* line, const char* end, const char* name) {
    for (; *name; ++line, ++name) {
        if (line >= end || lower(*line) != *name) return false;
    }
    return true;
}

bool contains_ignore_case(const char* begin, const char* end, const char* needle) {
    size_t n = strlen(needle);
    for (const char* p = begin; p + n <= end; ++p) {
        size_t k = 0;
        while (k < n && lower(p[k]) == needle[k]) ++k;
        if (k == n) return true;
    }
    return false;
}

struct ResponseHead {
    bool complete;        // status line and all headers received
    size_t body_start;
    uint16_t status;
    bool chunked;
    bool has_length;
    size_t content_length;
};

// Parses what has been received so far. Returns false if malformed.
bool parse_head(const char* buf, size_t len, ResponseHead& h) {
    h.complete = false;
    const char* end = buf + len;
    const char* header_end = nullptr;
    for (const char* p = buf; p + 4 <= end; ++p) {
        if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n') {
            header_end = p;
            break;
        }
    }
    if (header_end == nullptr) return true;  // not yet

    // Status line: "HTTP/1.x NNN reason"
    if (len < 12 || memcmp(buf, "HTTP/1.", 7) != 0 || buf[8] != ' ') return false;
    uint16_t status = 0;
    for (int i = 9; i < 12; ++i) {
        if (buf[i] < '0' || buf[i] > '9') return false;
        status = static_cast<uint16_t>(status * 10 + (buf[i] - '0'));
    }
    h.status = status;
    h.chunked = false;
    h.has_length = false;
    h.content_length = 0;

    const char* line = static_cast<const char*>(memchr(buf, '\n', len)) + 1;
    while (line < header_end) {
        const char* eol = static_cast<const char*>(memchr(line, '\r', header_end + 2 - line));
        if (eol == nullptr) return false;
        if (header_is(line, eol, "transfer-encoding:")) {
            if (contains_ignore_case(line, eol, "chunked")) h.chunked = true;
        } else if (header_is(line, eol, "content-length:")) {
            const char* p = line + 15;
            while (p < eol && *p == ' ') ++p;
            size_t value = 0;
            size_t digits = 0;
            for (; p < eol && *p >= '0' && *p <= '9'; ++p, ++digits) {
                value = value * 10 + static_cast<size_t>(*p - '0');
                if (value > asr_limits::kMaxResponseBytes) return false;
            }
            if (digits == 0) return false;
            h.has_length = true;
            h.content_length = value;
        }
        line = eol + 2;
    }
    h.body_start = static_cast<size_t>(header_end + 4 - buf);
    h.complete = true;
    return true;
}

bool body_complete(const char* buf, size_t len, const ResponseHead& h) {
    size_t body_len = len - h.body_start;
    if (h.chunked) {
        const char* body = buf + h.body_start;
        if (body_len == 5) return memcmp(body, "0\r\n\r\n", 5) == 0;
        return body_len > 5 && memcmp(buf + len - 7, "\r\n0\r\n\r\n", 7) == 0;
    }
    if (h.has_length) return body_len >= h.content_length;
    return false;  // read until the server closes
}

AsrStatus status_for_http(uint16_t code) {
    if (code >= 200 && code < 300) return AsrStatus::OK;
    if (code == 401 || code == 403) return AsrStatus::AUTH_FAILED;
    if (code == 429) return AsrStatus::RATE_LIMITED;
    if (code >= 500 && code < 600) return AsrStatus::SERVICE_UNAVAILABLE;
    return AsrStatus::HTTP_ERROR;
}

} // namespace

bool AsrClient::expired(uint32_t start_ms) const {
    return static_cast<uint32_t>(clock_.now_ms() - start_ms) >= timeout_ms_;
}

AsrResult AsrClient::transcribe(const uint8_t* pcm, size_t pcm_bytes) {
    AsrResult result;
    result.status = AsrStatus::BAD_INPUT;
    result.transcript[0] = '\0';
    result.confidence = -1.0f;
    result.http_status = 0;
    result.channel_status = ChannelStatus::NOT_OPEN;
    const uint32_t start = clock_.now_ms();

    const size_t max_bytes = static_cast<size_t>(config::kMaxCaptureMs) *
                             (config::kSampleRateHz / 1000) * (config::kSampleBits / 8);
    if (pcm == nullptr || pcm_bytes == 0 || pcm_bytes % 2 != 0 || pcm_bytes > max_bytes) {
        result.latency_ms = 0;
        return result;
    }

    HttpsUrl url;
    const char* extra = provider_.extra_header();
    const char* content_type = provider_.content_type();
    if (!parse_https_url(provider_.url(), url) || content_type == nullptr ||
        content_type[0] == '\0' || has_line_break(content_type) ||
        has_line_break(extra) ||
        (extra != nullptr && strlen(extra) > asr_limits::kMaxHeaderLineChars)) {
        result.status = AsrStatus::NOT_CONFIGURED;
        result.latency_ms = 0;
        return result;
    }

    result.channel_status = channel_.open(url, timeout_ms_);
    if (result.channel_status != ChannelStatus::OK) {
        result.status = AsrStatus::CHANNEL_REFUSED;
    } else {
        result.status = send_request(url, pcm, pcm_bytes, start);
        if (result.status == AsrStatus::OK) result.status = read_response(result, start);
    }
    channel_.close();
    result.latency_ms = clock_.now_ms() - start;
    if (result.status != AsrStatus::OK) {
        result.transcript[0] = '\0';
        result.confidence = -1.0f;
    }
    return result;
}

AsrStatus AsrClient::send_request(const HttpsUrl& url, const uint8_t* pcm, size_t pcm_bytes,
                                  uint32_t start_ms) {
    const bool b64 = provider_.audio_as_base64();
    const char* prefix = provider_.body_prefix() ? provider_.body_prefix() : "";
    const char* suffix = provider_.body_suffix() ? provider_.body_suffix() : "";
    const char* extra = provider_.extra_header();
    const unsigned long body_len = static_cast<unsigned long>(
        strlen(prefix) + (b64 ? asr_detail::base64_length(pcm_bytes) : pcm_bytes) +
        strlen(suffix));

    char host[tls_policy::kMaxHostChars + 8];
    if (url.port == tls_policy::kDefaultHttpsPort) {
        snprintf(host, sizeof(host), "%s", url.host);
    } else {
        snprintf(host, sizeof(host), "%s:%u", url.host, static_cast<unsigned>(url.port));
    }

    // The response buffer is free until the response arrives; reuse it.
    int n = snprintf(response_, sizeof(response_),
                     "POST %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %lu\r\n"
                     "%s%s"
                     "Accept-Encoding: identity\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     url.path, host, provider_.content_type(), body_len,
                     extra ? extra : "", extra ? "\r\n" : "");
    if (n < 0 || static_cast<size_t>(n) >= sizeof(response_)) return AsrStatus::NOT_CONFIGURED;
    if (!channel_.write(response_) || !channel_.write(prefix)) return AsrStatus::NETWORK_ERROR;

    // Stream the audio in small pieces: only one chunk is encoded at a time.
    const size_t step = asr_limits::kBodyChunkInputBytes;  // multiple of 3
    char encoded[(asr_limits::kBodyChunkInputBytes / 3) * 4];
    for (size_t off = 0; off < pcm_bytes; off += step) {
        if (expired(start_ms)) return AsrStatus::TIMEOUT;
        size_t len = pcm_bytes - off < step ? pcm_bytes - off : step;
        bool ok;
        if (b64) {
            size_t out = asr_detail::base64_encode(pcm + off, len, encoded);
            ok = channel_.write(reinterpret_cast<const uint8_t*>(encoded), out);
        } else {
            ok = channel_.write(pcm + off, len);
        }
        if (!ok) return AsrStatus::NETWORK_ERROR;
    }
    if (!channel_.write(suffix)) return AsrStatus::NETWORK_ERROR;
    return AsrStatus::OK;
}

AsrStatus AsrClient::read_response(AsrResult& result, uint32_t start_ms) {
    size_t len = 0;
    ResponseHead head;
    head.complete = false;
    bool closed = false;
    for (;;) {
        if (expired(start_ms)) return AsrStatus::TIMEOUT;
        if (len == asr_limits::kMaxResponseBytes) return AsrStatus::BAD_RESPONSE;
        int got = channel_.read(reinterpret_cast<uint8_t*>(response_ + len),
                                asr_limits::kMaxResponseBytes - len);
        if (got < 0) return expired(start_ms) ? AsrStatus::TIMEOUT : AsrStatus::NETWORK_ERROR;
        if (got == 0) {
            closed = true;
        } else {
            len += static_cast<size_t>(got);
        }
        response_[len] = '\0';

        if (!parse_head(response_, len, head)) return AsrStatus::BAD_RESPONSE;
        if (head.complete && body_complete(response_, len, head)) break;
        if (closed) {
            if (!head.complete) return AsrStatus::NETWORK_ERROR;
            if (head.chunked || head.has_length) return AsrStatus::NETWORK_ERROR;  // truncated
            break;  // body delimited by close
        }
    }

    result.http_status = head.status;
    AsrStatus status = status_for_http(head.status);
    if (status != AsrStatus::OK) return status;

    char* body = response_ + head.body_start;
    size_t body_len = len - head.body_start;
    if (head.has_length && !head.chunked && body_len > head.content_length) {
        body_len = head.content_length;
    }
    if (head.chunked && !asr_detail::dechunk_in_place(body, body_len)) {
        return AsrStatus::BAD_RESPONSE;
    }
    body[body_len] = '\0';

    float confidence = -1.0f;
    switch (provider_.parse_response(body, body_len, result.transcript,
                                     sizeof(result.transcript), confidence)) {
        case IAsrProvider::ParseResult::TRANSCRIPT:
            result.confidence = confidence;
            return AsrStatus::OK;
        case IAsrProvider::ParseResult::NO_SPEECH:
            return AsrStatus::NO_SPEECH;
        case IAsrProvider::ParseResult::TOO_LONG:
            return AsrStatus::TRANSCRIPT_TOO_LONG;
        case IAsrProvider::ParseResult::MALFORMED:
        default:
            return AsrStatus::BAD_RESPONSE;
    }
}

} // namespace vamc
