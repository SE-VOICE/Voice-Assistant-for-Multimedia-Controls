#pragma once
// Shared test double for ITlsSocket. PlatformIO only runs folders named
// test_*, so this folder is not a test suite; include it by relative path.

#include "secure_channel.h"
#include <string.h>
#include <string>

namespace vamc {
namespace testing {

class FakeTlsSocket : public ITlsSocket {
public:
    // Behaviour of the next connect().
    bool connect_result = true;
    TlsVersion version = TlsVersion::TLS_1_2;
    bool verified = true;
    bool fail_writes = false;
    // Bytes returned by read(), in chunks of at most read_chunk bytes.
    std::string response;
    size_t read_chunk = 7;
    bool read_error_at_end = false;  // return -1 instead of 0 after response

    // Recorded calls.
    unsigned connect_calls = 0;
    unsigned close_calls = 0;
    std::string last_host;
    uint16_t last_port = 0;
    TlsVersion last_min_version = TlsVersion::UNKNOWN;
    const char* last_ca = nullptr;
    uint32_t last_timeout_ms = 0;
    std::string written;
    size_t write_calls = 0;
    size_t largest_write = 0;

    bool connect(const char* host, uint16_t port, TlsVersion min_version,
                 const char* ca_pem, uint32_t timeout_ms) override {
        ++connect_calls;
        last_host = host;
        last_port = port;
        last_min_version = min_version;
        last_ca = ca_pem;
        last_timeout_ms = timeout_ms;
        connected_ = connect_result;
        read_pos_ = 0;
        return connect_result;
    }
    TlsVersion negotiated_version() const override { return version; }
    bool peer_verified() const override { return verified; }
    bool write(const uint8_t* data, size_t len) override {
        if (!connected_ || fail_writes) return false;
        ++write_calls;
        if (len > largest_write) largest_write = len;
        written.append(reinterpret_cast<const char*>(data), len);
        return true;
    }
    int read(uint8_t* buffer, size_t capacity) override {
        if (!connected_) return -1;
        if (read_pos_ >= response.size()) return read_error_at_end ? -1 : 0;
        size_t n = response.size() - read_pos_;
        if (n > read_chunk) n = read_chunk;
        if (n > capacity) n = capacity;
        memcpy(buffer, response.data() + read_pos_, n);
        read_pos_ += n;
        return static_cast<int>(n);
    }
    void close() override {
        ++close_calls;
        connected_ = false;
    }
    bool connected() const { return connected_; }

private:
    bool connected_ = false;
    size_t read_pos_ = 0;
};

} // namespace testing
} // namespace vamc
