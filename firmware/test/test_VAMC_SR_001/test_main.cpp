#include <unity.h>
#include <string.h>
#include "secure_channel.h"
#include "../fakes/fake_tls_socket.h"

using namespace vamc;
using vamc::testing::FakeTlsSocket;

static const char* kCa = "-----BEGIN CERTIFICATE-----\nFAKE\n-----END CERTIFICATE-----\n";

void setUp() {}
void tearDown() {}

static HttpsUrl url(const char* s) {
    HttpsUrl u;
    TEST_ASSERT_TRUE_MESSAGE(parse_https_url(s, u), s);
    return u;
}

// TC-SEC-01 (native): a TLS 1.2 / 1.3 verified session opens and carries data.
void test_TC_SEC_01_tls12_and_tls13_verified_sessions_open() {
    const TlsVersion ok[] = {TlsVersion::TLS_1_2, TlsVersion::TLS_1_3};
    for (size_t i = 0; i < 2; ++i) {
        FakeTlsSocket sock;
        sock.version = ok[i];
        SecureChannel ch(sock, kCa);
        TEST_ASSERT_EQUAL(ChannelStatus::OK, ch.open(url("https://speech.example.com/v1"), 3000));
        TEST_ASSERT_TRUE(ch.is_open());
        TEST_ASSERT_TRUE(ch.write("audio"));
        TEST_ASSERT_EQUAL_STRING("audio", sock.written.c_str());
    }
}

// TC-SEC-01: the policy is requested from the TLS stack before the handshake.
void test_TC_SEC_01_minimum_version_and_ca_are_passed_to_the_stack() {
    FakeTlsSocket sock;
    SecureChannel ch(sock, kCa);
    ch.open(url("https://speech.googleapis.com/v1/speech:recognize"), 2500);
    TEST_ASSERT_EQUAL(TlsVersion::TLS_1_2, sock.last_min_version);
    TEST_ASSERT_EQUAL_PTR(kCa, sock.last_ca);
    TEST_ASSERT_EQUAL_STRING("speech.googleapis.com", sock.last_host.c_str());
    TEST_ASSERT_EQUAL_UINT16(443, sock.last_port);
    TEST_ASSERT_EQUAL_UINT32(2500, sock.last_timeout_ms);
}

// TC-SEC-01: anything below TLS 1.2 is closed before a byte is sent.
void test_TC_SEC_01_tls_below_1_2_is_refused() {
    const TlsVersion bad[] = {TlsVersion::TLS_1_0, TlsVersion::TLS_1_1, TlsVersion::UNKNOWN};
    for (size_t i = 0; i < 3; ++i) {
        FakeTlsSocket sock;
        sock.version = bad[i];
        SecureChannel ch(sock, kCa);
        TEST_ASSERT_EQUAL(ChannelStatus::TLS_VERSION_TOO_OLD,
                          ch.open(url("https://speech.example.com/"), 1000));
        TEST_ASSERT_FALSE(ch.is_open());
        TEST_ASSERT_FALSE(sock.connected());
        TEST_ASSERT_FALSE(ch.write("audio"));
        TEST_ASSERT_EQUAL_UINT(0, sock.written.size());
    }
}

// An unverified certificate (e.g. interception proxy) is refused.
void test_unverified_server_certificate_is_refused() {
    FakeTlsSocket sock;
    sock.verified = false;
    SecureChannel ch(sock, kCa);
    TEST_ASSERT_EQUAL(ChannelStatus::PEER_NOT_VERIFIED,
                      ch.open(url("https://speech.example.com/"), 1000));
    TEST_ASSERT_FALSE(sock.connected());
    TEST_ASSERT_FALSE(ch.write("audio"));
    TEST_ASSERT_EQUAL_UINT(0, sock.written.size());
}

// Without a trust anchor there is nothing to verify against: no connection.
void test_missing_ca_refuses_without_connecting() {
    FakeTlsSocket sock;
    SecureChannel no_ca(sock, nullptr);
    TEST_ASSERT_EQUAL(ChannelStatus::NO_TRUST_ANCHOR, no_ca.open(url("https://a.example/"), 1000));
    SecureChannel empty_ca(sock, "");
    TEST_ASSERT_EQUAL(ChannelStatus::NO_TRUST_ANCHOR, empty_ca.open(url("https://a.example/"), 1000));
    TEST_ASSERT_EQUAL_UINT(0, sock.connect_calls);
}

void test_failed_handshake_reports_connect_failed() {
    FakeTlsSocket sock;
    sock.connect_result = false;
    SecureChannel ch(sock, kCa);
    TEST_ASSERT_EQUAL(ChannelStatus::CONNECT_FAILED, ch.open(url("https://a.example/"), 1000));
    TEST_ASSERT_FALSE(ch.is_open());
    TEST_ASSERT_EQUAL_UINT(1, sock.close_calls);
}

// No plaintext path: only https:// URLs parse at all.
void test_non_https_urls_are_rejected() {
    HttpsUrl u;
    const char* const bad[] = {
        "http://speech.googleapis.com/v1/speech:recognize",
        "ftp://example.com/",
        "speech.googleapis.com/v1",
        "https://",
        "https:///path",
        "https://user@evil.example/",
        "https://host:0/",
        "https://host:70000/",
        "https://host:/",
        "https://host/has space",
        "https://hostpath\r\nInjected: header",
        "https://evil\r\n.example/",
        "https://a.example/p\r\nX-Injected: 1",
        nullptr,
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        TEST_ASSERT_FALSE_MESSAGE(parse_https_url(bad[i], u), bad[i] ? bad[i] : "(null)");
    }
}

void test_https_url_parsing() {
    HttpsUrl u = url("https://speech.googleapis.com/v1/speech:recognize?key=ABC");
    TEST_ASSERT_EQUAL_STRING("speech.googleapis.com", u.host);
    TEST_ASSERT_EQUAL_UINT16(443, u.port);
    TEST_ASSERT_EQUAL_STRING("/v1/speech:recognize?key=ABC", u.path);

    u = url("HTTPS://asr.example.com:8443");
    TEST_ASSERT_EQUAL_STRING("asr.example.com", u.host);
    TEST_ASSERT_EQUAL_UINT16(8443, u.port);
    TEST_ASSERT_EQUAL_STRING("/", u.path);

    u = url("https://asr.example.com?x=1");
    TEST_ASSERT_EQUAL_STRING("/?x=1", u.path);
}

void test_overlong_host_or_path_is_rejected() {
    char long_host[128] = "https://";
    memset(long_host + 8, 'a', tls_policy::kMaxHostChars);
    long_host[8 + tls_policy::kMaxHostChars] = '\0';
    HttpsUrl u;
    TEST_ASSERT_FALSE(parse_https_url(long_host, u));

    char long_path[300] = "https://a.example/";
    memset(long_path + 18, 'p', tls_policy::kMaxPathChars);
    long_path[18 + tls_policy::kMaxPathChars] = '\0';
    TEST_ASSERT_FALSE(parse_https_url(long_path, u));
}

void test_io_refused_when_not_open_and_after_close() {
    FakeTlsSocket sock;
    SecureChannel ch(sock, kCa);
    uint8_t buf[8];
    TEST_ASSERT_FALSE(ch.write("x"));
    TEST_ASSERT_EQUAL_INT(-1, ch.read(buf, sizeof(buf)));

    TEST_ASSERT_EQUAL(ChannelStatus::OK, ch.open(url("https://a.example/"), 1000));
    ch.close();
    TEST_ASSERT_FALSE(ch.is_open());
    TEST_ASSERT_FALSE(ch.write("x"));
    TEST_ASSERT_EQUAL_UINT(0, sock.written.size());
}

void test_write_failure_closes_channel() {
    FakeTlsSocket sock;
    SecureChannel ch(sock, kCa);
    TEST_ASSERT_EQUAL(ChannelStatus::OK, ch.open(url("https://a.example/"), 1000));
    sock.fail_writes = true;
    TEST_ASSERT_FALSE(ch.write("x"));
    TEST_ASSERT_FALSE(ch.is_open());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_SEC_01_tls12_and_tls13_verified_sessions_open);
    RUN_TEST(test_TC_SEC_01_minimum_version_and_ca_are_passed_to_the_stack);
    RUN_TEST(test_TC_SEC_01_tls_below_1_2_is_refused);
    RUN_TEST(test_unverified_server_certificate_is_refused);
    RUN_TEST(test_missing_ca_refuses_without_connecting);
    RUN_TEST(test_failed_handshake_reports_connect_failed);
    RUN_TEST(test_non_https_urls_are_rejected);
    RUN_TEST(test_https_url_parsing);
    RUN_TEST(test_overlong_host_or_path_is_rejected);
    RUN_TEST(test_io_refused_when_not_open_and_after_close);
    RUN_TEST(test_write_failure_closes_channel);
    return UNITY_END();
}
