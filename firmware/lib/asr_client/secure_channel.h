#pragma once

#include <stdint.h>
#include <stddef.h>

namespace vamc {

// VAMC-SR-001: all audio sent to the cloud ASR API uses TLS 1.2 or higher.
//
// The ASR client only talks to the network through SecureChannel. There is
// no plaintext path: a URL that is not https://, a missing trust anchor, a
// handshake below TLS 1.2 or an unverified server certificate all close the
// connection before any request byte (and so any audio) is written.

enum class TlsVersion : uint8_t { UNKNOWN, TLS_1_0, TLS_1_1, TLS_1_2, TLS_1_3 };

namespace tls_policy {
    constexpr TlsVersion kMinVersion = TlsVersion::TLS_1_2;
    constexpr uint16_t kDefaultHttpsPort = 443;
    constexpr size_t kMaxHostChars = 64;   // including terminator
    constexpr size_t kMaxPathChars = 192;  // including terminator; path + query
} // namespace tls_policy

// Platform TLS socket, implemented in src/hal for the chosen board (e.g.
// WiFiClientSecure / mbedTLS on Arduino-ESP32) and by fakes in tests.
//
// Adapter contract:
//  - connect() must configure `min_version` and the `ca_pem` trust anchor on
//    the TLS stack BEFORE the handshake, and must verify the server
//    certificate chain and host name against it (never setInsecure()).
//    On mbedTLS: mbedtls_ssl_conf_min_version(conf, 3, 3) for TLS 1.2.
//  - negotiated_version() and peer_verified() report the actual handshake.
class ITlsSocket {
public:
    virtual ~ITlsSocket() = default;
    virtual bool connect(const char* host, uint16_t port, TlsVersion min_version,
                         const char* ca_pem, uint32_t timeout_ms) = 0;
    virtual TlsVersion negotiated_version() const = 0;
    virtual bool peer_verified() const = 0;
    virtual bool write(const uint8_t* data, size_t len) = 0;
    // > 0: bytes read; 0: connection closed by peer; < 0: error or timeout.
    virtual int read(uint8_t* buffer, size_t capacity) = 0;
    virtual void close() = 0;
};

struct HttpsUrl {
    char host[tls_policy::kMaxHostChars];
    uint16_t port;
    char path[tls_policy::kMaxPathChars];  // starts with '/', includes query
};

// Parses "https://host[:port][/path[?query]]". Any other scheme, an empty or
// overlong host/path, or a bad port returns false.
bool parse_https_url(const char* url, HttpsUrl& out);

enum class ChannelStatus : uint8_t {
    OK,
    NOT_HTTPS,          // URL rejected before connecting
    NO_TRUST_ANCHOR,    // no CA certificate configured
    CONNECT_FAILED,     // TCP or TLS handshake failed
    TLS_VERSION_TOO_OLD,
    PEER_NOT_VERIFIED,
    NOT_OPEN            // write/read attempted without an open channel
};

class SecureChannel {
public:
    // `ca_pem` must outlive the channel (typically a constant in flash).
    SecureChannel(ITlsSocket& socket, const char* ca_pem)
        : socket_(socket), ca_pem_(ca_pem), open_(false) {}
    ~SecureChannel() { close(); }

    ChannelStatus open(const HttpsUrl& url, uint32_t timeout_ms);
    bool is_open() const { return open_; }

    // Refused (returns false / -1) unless open() returned OK.
    bool write(const uint8_t* data, size_t len);
    bool write(const char* text);
    int read(uint8_t* buffer, size_t capacity);

    void close();

private:
    SecureChannel(const SecureChannel&);
    SecureChannel& operator=(const SecureChannel&);

    ITlsSocket& socket_;
    const char* ca_pem_;
    bool open_;
};

} // namespace vamc
