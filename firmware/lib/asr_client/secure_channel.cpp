#include "secure_channel.h"
#include <string.h>

namespace vamc {

namespace {

bool starts_with_ignore_case(const char* s, const char* prefix) {
    for (; *prefix; ++s, ++prefix) {
        char c = *s;
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        if (c != *prefix) return false;
    }
    return true;
}

bool version_at_least(TlsVersion actual, TlsVersion minimum) {
    return actual != TlsVersion::UNKNOWN &&
           static_cast<uint8_t>(actual) >= static_cast<uint8_t>(minimum);
}

} // namespace

bool parse_https_url(const char* url, HttpsUrl& out) {
    if (url == nullptr || !starts_with_ignore_case(url, "https://")) return false;
    const char* p = url + 8;

    // Host: up to ':', '/', '?' or end. Only letters, digits, '-' and '.'
    // are accepted, so user-info ('@'), spaces and control characters fail.
    size_t host_len = 0;
    while (p[host_len] && p[host_len] != ':' && p[host_len] != '/' && p[host_len] != '?') {
        char c = p[host_len];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '-' || c == '.';
        if (!ok) return false;
        ++host_len;
    }
    if (host_len == 0 || host_len >= tls_policy::kMaxHostChars) return false;
    memcpy(out.host, p, host_len);
    out.host[host_len] = '\0';
    p += host_len;

    out.port = tls_policy::kDefaultHttpsPort;
    if (*p == ':') {
        ++p;
        uint32_t port = 0;
        size_t digits = 0;
        while (*p >= '0' && *p <= '9') {
            port = port * 10 + static_cast<uint32_t>(*p - '0');
            if (port > 65535) return false;
            ++p;
            ++digits;
        }
        if (digits == 0 || port == 0) return false;
        out.port = static_cast<uint16_t>(port);
    }

    if (*p == '\0') {
        out.path[0] = '/';
        out.path[1] = '\0';
        return true;
    }
    size_t path_len = strlen(p);
    size_t extra = (*p == '?') ? 1 : 0;  // "https://h?x" -> "/?x"
    if (*p != '/' && *p != '?') return false;
    if (path_len + extra >= tls_policy::kMaxPathChars) return false;
    for (size_t i = 0; i < path_len; ++i) {
        unsigned char c = static_cast<unsigned char>(p[i]);
        if (c <= 0x20 || c >= 0x7F) return false;  // spaces, control, non-ASCII
    }
    if (extra) out.path[0] = '/';
    memcpy(out.path + extra, p, path_len + 1);
    return true;
}

ChannelStatus SecureChannel::open(const HttpsUrl& url, uint32_t timeout_ms) {
    close();
    if (url.host[0] == '\0') return ChannelStatus::NOT_HTTPS;
    if (ca_pem_ == nullptr || ca_pem_[0] == '\0') return ChannelStatus::NO_TRUST_ANCHOR;

    if (!socket_.connect(url.host, url.port, tls_policy::kMinVersion, ca_pem_, timeout_ms)) {
        socket_.close();
        return ChannelStatus::CONNECT_FAILED;
    }
    // Defence in depth: do not trust that the adapter applied the policy.
    if (!version_at_least(socket_.negotiated_version(), tls_policy::kMinVersion)) {
        socket_.close();
        return ChannelStatus::TLS_VERSION_TOO_OLD;
    }
    if (!socket_.peer_verified()) {
        socket_.close();
        return ChannelStatus::PEER_NOT_VERIFIED;
    }
    open_ = true;
    return ChannelStatus::OK;
}

bool SecureChannel::write(const uint8_t* data, size_t len) {
    if (!open_ || (data == nullptr && len > 0)) return false;
    if (len == 0) return true;
    if (!socket_.write(data, len)) {
        close();
        return false;
    }
    return true;
}

bool SecureChannel::write(const char* text) {
    if (text == nullptr) return false;
    return write(reinterpret_cast<const uint8_t*>(text), strlen(text));
}

int SecureChannel::read(uint8_t* buffer, size_t capacity) {
    if (!open_ || buffer == nullptr || capacity == 0) return -1;
    return socket_.read(buffer, capacity);
}

void SecureChannel::close() {
    if (open_) {
        socket_.close();
        open_ = false;
    }
}

} // namespace vamc
