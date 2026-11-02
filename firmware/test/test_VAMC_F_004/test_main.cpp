#include <unity.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "asr_client.h"
#include "google_speech_provider.h"
#include "secure_channel.h"
#include "config.h"
#include "../fakes/fake_tls_socket.h"

using namespace vamc;
using vamc::testing::FakeTlsSocket;

static const char* kCa = "-----BEGIN CERTIFICATE-----\nFAKE\n-----END CERTIFICATE-----\n";
static const char* kKey = "AIzaTEST_key-123";

class FakeClock : public IClock {
public:
    mutable uint32_t t = 1000;
    uint32_t step = 0;  // added on every read of the clock
    uint32_t now_ms() const override {
        uint32_t r = t;
        t += step;
        return r;
    }
};

static std::vector<uint8_t> pcm(size_t bytes) {
    std::vector<uint8_t> v(bytes);
    for (size_t i = 0; i < bytes; ++i) v[i] = static_cast<uint8_t>(i * 7 + 3);
    return v;
}

static std::string http_200(const std::string& body) {
    char head[128];
    snprintf(head, sizeof(head),
             "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\n"
             "Content-Length: %lu\r\n\r\n", static_cast<unsigned long>(body.size()));
    return head + body;
}

static std::string http_status(int code, const char* reason) {
    char buf[160];
    snprintf(buf, sizeof(buf),
             "HTTP/1.1 %d %s\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}", code, reason);
    return buf;
}

static const char* kTurnItUp =
    "{\n  \"results\": [\n    {\n      \"alternatives\": [\n        {\n"
    "          \"transcript\": \"turn it up\",\n          \"confidence\": 0.93\n"
    "        }\n      ],\n      \"resultEndTime\": \"1.200s\",\n"
    "      \"languageCode\": \"en-us\"\n    }\n  ],\n"
    "  \"totalBilledTime\": \"2s\",\n  \"requestId\": \"123\"\n}\n";

// Everything needed for one transcribe() call against a fake server.
struct Rig {
    FakeTlsSocket sock;
    FakeClock clock;
    GoogleSpeechV1Provider google{kKey, "en-US"};
    SecureChannel channel{sock, kCa};

    AsrResult run(const std::vector<uint8_t>& audio, const IAsrProvider* provider = nullptr,
                  uint32_t timeout_ms = asr_limits::kDefaultTimeoutMs) {
        AsrClient client(channel, provider ? *provider : google, clock, timeout_ms);
        return client.transcribe(audio.data(), audio.size());
    }
    std::string headers() const {
        size_t end = sock.written.find("\r\n\r\n");
        return end == std::string::npos ? std::string() : sock.written.substr(0, end + 2);
    }
    std::string body() const {
        size_t end = sock.written.find("\r\n\r\n");
        return end == std::string::npos ? std::string() : sock.written.substr(end + 4);
    }
};

void setUp() {}
void tearDown() {}

// TC-ASR-01 (native): audio is sent over HTTPS and the transcript comes back.
void test_TC_ASR_01_supported_command_is_transcribed_over_https() {
    Rig rig;
    rig.sock.response = http_200(kTurnItUp);
    rig.clock.step = 10;
    AsrResult r = rig.run(pcm(32000));  // 1 s of audio

    TEST_ASSERT_EQUAL(AsrStatus::OK, r.status);
    TEST_ASSERT_TRUE(r.ok());
    TEST_ASSERT_EQUAL_STRING("turn it up", r.transcript);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.93f, r.confidence);
    TEST_ASSERT_EQUAL_UINT16(200, r.http_status);
    TEST_ASSERT_EQUAL(ChannelStatus::OK, r.channel_status);
    TEST_ASSERT_TRUE(r.latency_ms > 0);
    TEST_ASSERT_TRUE(r.within_latency_budget());

    // HTTPS to the SAD endpoint, TLS 1.2 minimum (SR-001).
    TEST_ASSERT_EQUAL_STRING("speech.googleapis.com", rig.sock.last_host.c_str());
    TEST_ASSERT_EQUAL_UINT16(443, rig.sock.last_port);
    TEST_ASSERT_EQUAL(TlsVersion::TLS_1_2, rig.sock.last_min_version);
    TEST_ASSERT_FALSE(rig.sock.connected());  // always closed afterwards
}

// The request matches SAD 4.3 Interface 1 and is well-formed HTTP.
void test_request_format_matches_sad_interface() {
    Rig rig;
    rig.sock.response = http_200(kTurnItUp);
    std::vector<uint8_t> audio = pcm(1000);
    rig.run(audio);

    std::string h = rig.headers();
    TEST_ASSERT_EQUAL_INT(0, h.find("POST /v1/speech:recognize HTTP/1.1\r\n"));
    TEST_ASSERT_TRUE(h.find("\r\nHost: speech.googleapis.com\r\n") != std::string::npos);
    TEST_ASSERT_TRUE(h.find("\r\nContent-Type: application/json; charset=utf-8\r\n") != std::string::npos);
    TEST_ASSERT_TRUE(h.find(std::string("\r\nX-Goog-Api-Key: ") + kKey + "\r\n") != std::string::npos);
    TEST_ASSERT_TRUE(h.find("\r\nConnection: close\r\n") != std::string::npos);
    // The key is never in the request line.
    TEST_ASSERT_TRUE(h.substr(0, h.find("\r\n")).find(kKey) == std::string::npos);

    std::string b = rig.body();
    char cl[48];
    snprintf(cl, sizeof(cl), "\r\nContent-Length: %lu\r\n", static_cast<unsigned long>(b.size()));
    TEST_ASSERT_TRUE_MESSAGE(h.find(cl) != std::string::npos, "Content-Length matches body");

    std::vector<char> expected_b64(asr_detail::base64_length(audio.size()));
    asr_detail::base64_encode(audio.data(), audio.size(), expected_b64.data());
    std::string expected =
        "{\"config\":{\"encoding\":\"LINEAR16\",\"sampleRateHertz\":16000,"
        "\"languageCode\":\"en-US\",\"maxAlternatives\":1},\"audio\":{\"content\":\"" +
        std::string(expected_b64.begin(), expected_b64.end()) + "\"}}";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), b.c_str());
}

// Streaming: a full 5 s capture is sent in small writes, never one big copy.
void test_max_capture_is_streamed_in_small_chunks() {
    Rig rig;
    rig.sock.response = http_200(kTurnItUp);
    size_t max_bytes = config::kMaxCaptureMs * (config::kSampleRateHz / 1000) * 2;
    std::vector<uint8_t> audio = pcm(max_bytes);
    AsrResult r = rig.run(audio);
    TEST_ASSERT_EQUAL(AsrStatus::OK, r.status);
    TEST_ASSERT_TRUE(rig.sock.largest_write <= 512);
    TEST_ASSERT_TRUE(rig.sock.write_calls > 400);
    TEST_ASSERT_TRUE(rig.body().find(std::string("\"}}")) == rig.body().size() - 3);
}

void test_language_code_is_configurable() {
    FakeTlsSocket sock;
    FakeClock clock;
    SecureChannel channel(sock, kCa);
    GoogleSpeechV1Provider india(kKey, "en-IN");
    sock.response = http_200(kTurnItUp);
    AsrClient client(channel, india, clock);
    std::vector<uint8_t> audio = pcm(64);
    TEST_ASSERT_EQUAL(AsrStatus::OK, client.transcribe(audio.data(), audio.size()).status);
    TEST_ASSERT_TRUE(sock.written.find("\"languageCode\":\"en-IN\"") != std::string::npos);
}

void test_chunked_and_fragmented_responses_are_read() {
    std::string body = kTurnItUp;
    std::string chunked = "HTTP/1.1 200 OK\r\nTransfer-Encoding: Chunked\r\n\r\n";
    char size_line[16];
    snprintf(size_line, sizeof(size_line), "%lx\r\n", static_cast<unsigned long>(40));
    chunked += size_line + body.substr(0, 40) + "\r\n";
    snprintf(size_line, sizeof(size_line), "%lX;ext=1\r\n",
             static_cast<unsigned long>(body.size() - 40));
    chunked += size_line + body.substr(40) + "\r\n0\r\n\r\n";

    const size_t chunk_sizes[] = {1, 3, 64, 4096};
    for (size_t i = 0; i < 4; ++i) {
        Rig rig;
        rig.sock.response = chunked;
        rig.sock.read_chunk = chunk_sizes[i];
        AsrResult r = rig.run(pcm(100));
        TEST_ASSERT_EQUAL(AsrStatus::OK, r.status);
        TEST_ASSERT_EQUAL_STRING("turn it up", r.transcript);
    }
    // Body delimited only by the server closing the connection.
    Rig rig;
    rig.sock.response = std::string("HTTP/1.0 200 OK\r\n\r\n") + kTurnItUp;
    TEST_ASSERT_EQUAL(AsrStatus::OK, rig.run(pcm(100)).status);
}

// SAD 4.3 error mapping: 401, 429, 5xx (and others) are distinct failures.
void test_http_errors_are_mapped() {
    const struct { int code; const char* reason; AsrStatus expected; } cases[] = {
        {401, "Unauthorized", AsrStatus::AUTH_FAILED},
        {403, "Forbidden", AsrStatus::AUTH_FAILED},
        {429, "Too Many Requests", AsrStatus::RATE_LIMITED},
        {500, "Internal Server Error", AsrStatus::SERVICE_UNAVAILABLE},
        {503, "Service Unavailable", AsrStatus::SERVICE_UNAVAILABLE},
        {400, "Bad Request", AsrStatus::HTTP_ERROR},
        {404, "Not Found", AsrStatus::HTTP_ERROR},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Rig rig;
        rig.sock.response = http_status(cases[i].code, cases[i].reason);
        AsrResult r = rig.run(pcm(100));
        TEST_ASSERT_EQUAL_MESSAGE(cases[i].expected, r.status, cases[i].reason);
        TEST_ASSERT_EQUAL_UINT16(cases[i].code, r.http_status);
        TEST_ASSERT_EQUAL_STRING("", r.transcript);
        TEST_ASSERT_FALSE(r.ok());
    }
}

void test_silence_is_no_speech() {
    const char* const bodies[] = {
        "{}",
        "{\n  \"totalBilledTime\": \"1s\",\n  \"requestId\": \"9\"\n}",
        "{\"results\":[{\"alternatives\":[{\"transcript\":\"  \"}]}]}",
    };
    for (size_t i = 0; i < 3; ++i) {
        Rig rig;
        rig.sock.response = http_200(bodies[i]);
        AsrResult r = rig.run(pcm(100));
        TEST_ASSERT_EQUAL_MESSAGE(AsrStatus::NO_SPEECH, r.status, bodies[i]);
        TEST_ASSERT_EQUAL_STRING("", r.transcript);
    }
}

void test_transcript_parsing_details() {
    GoogleSpeechV1Provider g(kKey);
    char t[asr_limits::kMaxTranscriptChars + 1];
    float c = 0;

    // Several results are joined; the lowest confidence is kept.
    const char* multi =
        "{\"results\":[{\"alternatives\":[{\"transcript\":\"next\",\"confidence\":0.9}]},"
        "{\"alternatives\":[{\"transcript\":\" song\",\"confidence\":0.7}]}]}";
    TEST_ASSERT_EQUAL(IAsrProvider::ParseResult::TRANSCRIPT,
                      g.parse_response(multi, strlen(multi), t, sizeof(t), c));
    TEST_ASSERT_EQUAL_STRING("next song", t);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.7f, c);

    // Missing confidence is reported as negative ("not provided").
    const char* noconf = "{\"results\":[{\"alternatives\":[{\"transcript\":\"pause\"}]}]}";
    TEST_ASSERT_EQUAL(IAsrProvider::ParseResult::TRANSCRIPT,
                      g.parse_response(noconf, strlen(noconf), t, sizeof(t), c));
    TEST_ASSERT_TRUE(c < 0.0f);

    // JSON escapes are decoded; keys inside strings are not mistaken for keys.
    const char* esc =
        "{\"results\":[{\"alternatives\":[{\"transcript\":\"turn \\\"it\\\" \\u0075p\","
        "\"confidence\":9.5e-1}]}],\"note\":\"\\\"transcript\\\": \\\"evil\\\"\"}";
    TEST_ASSERT_EQUAL(IAsrProvider::ParseResult::TRANSCRIPT,
                      g.parse_response(esc, strlen(esc), t, sizeof(t), c));
    TEST_ASSERT_EQUAL_STRING("turn \"it\" up", t);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.95f, c);
}

void test_overlong_transcript_is_rejected_not_truncated() {
    std::string words;
    while (words.size() <= asr_limits::kMaxTranscriptChars) words += "next ";
    Rig rig;
    rig.sock.response = http_200("{\"results\":[{\"alternatives\":[{\"transcript\":\"" + words +
                                 "\"}]}]}");
    AsrResult r = rig.run(pcm(100));
    TEST_ASSERT_EQUAL(AsrStatus::TRANSCRIPT_TOO_LONG, r.status);
    TEST_ASSERT_EQUAL_STRING("", r.transcript);
}

void test_malformed_responses_are_rejected() {
    const std::string cases[] = {
        "garbage\r\n\r\n",
        "HTTP/1.1 2x0 OK\r\n\r\n{}",
        http_200("not json"),
        http_200("{\"results\":[{\"alternatives\":[{\"transcript\":\"unterminated}]}]}"),
        http_200("{\"results\":[{\"alternatives\":[{\"transcript\":42}]}]}"),
        http_200("{\"results\":[{\"alternatives\":[{\"transcript\":\"x\",\"confidence\":-1}]}]}"),
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nZZ\r\n{}\r\n0\r\n\r\n",
        "HTTP/1.1 200 OK\r\nContent-Length: 99999\r\n\r\n{}",
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Rig rig;
        rig.sock.response = cases[i];
        AsrResult r = rig.run(pcm(100));
        char msg[32];
        snprintf(msg, sizeof(msg), "case %u", static_cast<unsigned>(i));
        TEST_ASSERT_EQUAL_MESSAGE(AsrStatus::BAD_RESPONSE, r.status, msg);
    }
    // A response bigger than the buffer is rejected, not overrun.
    Rig rig;
    rig.sock.response = "HTTP/1.1 200 OK\r\n\r\n{\"x\":\"" +
                        std::string(asr_limits::kMaxResponseBytes, 'a') + "\"}";
    TEST_ASSERT_EQUAL(AsrStatus::BAD_RESPONSE, rig.run(pcm(100)).status);
}

void test_network_failures() {
    {   // Connection closed before any response.
        Rig rig;
        rig.sock.response = "";
        TEST_ASSERT_EQUAL(AsrStatus::NETWORK_ERROR, rig.run(pcm(100)).status);
    }
    {   // Closed mid-headers.
        Rig rig;
        rig.sock.response = "HTTP/1.1 200 OK\r\nContent-Le";
        TEST_ASSERT_EQUAL(AsrStatus::NETWORK_ERROR, rig.run(pcm(100)).status);
    }
    {   // Body shorter than Content-Length.
        Rig rig;
        rig.sock.response = "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n{\"results\":";
        TEST_ASSERT_EQUAL(AsrStatus::NETWORK_ERROR, rig.run(pcm(100)).status);
    }
    {   // Socket read error.
        Rig rig;
        rig.sock.response = "HTTP/1.1 200 OK\r\n";
        rig.sock.read_error_at_end = true;
        TEST_ASSERT_EQUAL(AsrStatus::NETWORK_ERROR, rig.run(pcm(100)).status);
    }
    {   // Write fails while streaming audio.
        Rig rig;
        rig.sock.fail_writes = true;
        AsrResult r = rig.run(pcm(100));
        TEST_ASSERT_EQUAL(AsrStatus::NETWORK_ERROR, r.status);
        TEST_ASSERT_FALSE(rig.sock.connected());
    }
}

void test_timeout_and_latency_budget() {
    {   // Clock passes the timeout while waiting: TIMEOUT, no transcript.
        Rig rig;
        rig.sock.response = http_200(kTurnItUp);
        rig.sock.read_chunk = 1;
        rig.clock.step = 100;
        AsrResult r = rig.run(pcm(100), nullptr, 2000);
        TEST_ASSERT_EQUAL(AsrStatus::TIMEOUT, r.status);
        TEST_ASSERT_EQUAL_STRING("", r.transcript);
        TEST_ASSERT_FALSE(rig.sock.connected());
    }
    {   // Slower than the 3 s acceptance budget but inside the timeout.
        Rig rig;
        rig.sock.response = http_200(kTurnItUp);
        rig.sock.read_chunk = 4096;
        rig.clock.step = 1100;  // ~3.3 s total over three clock reads
        AsrResult r = rig.run(pcm(100));
        TEST_ASSERT_EQUAL(AsrStatus::OK, r.status);
        TEST_ASSERT_TRUE(r.latency_ms >= asr_limits::kLatencyBudgetMs);
        TEST_ASSERT_FALSE(r.within_latency_budget());
    }
}

// SR-001 is enforced end to end: refused channels send no audio.
void test_insecure_channel_sends_no_audio() {
    Rig rig;
    rig.sock.version = TlsVersion::TLS_1_1;
    rig.sock.response = http_200(kTurnItUp);
    AsrResult r = rig.run(pcm(100));
    TEST_ASSERT_EQUAL(AsrStatus::CHANNEL_REFUSED, r.status);
    TEST_ASSERT_EQUAL(ChannelStatus::TLS_VERSION_TOO_OLD, r.channel_status);
    TEST_ASSERT_EQUAL_UINT(0, rig.sock.written.size());

    Rig unverified;
    unverified.sock.verified = false;
    TEST_ASSERT_EQUAL(ChannelStatus::PEER_NOT_VERIFIED, unverified.run(pcm(100)).channel_status);
    TEST_ASSERT_EQUAL_UINT(0, unverified.sock.written.size());
}

void test_bad_audio_input_is_rejected_without_connecting() {
    Rig rig;
    size_t max_bytes = config::kMaxCaptureMs * (config::kSampleRateHz / 1000) * 2;
    AsrClient client(rig.channel, rig.google, rig.clock);
    std::vector<uint8_t> big = pcm(max_bytes + 2);
    uint8_t one[3] = {1, 2, 3};
    TEST_ASSERT_EQUAL(AsrStatus::BAD_INPUT, client.transcribe(nullptr, 100).status);
    TEST_ASSERT_EQUAL(AsrStatus::BAD_INPUT, client.transcribe(one, 0).status);
    TEST_ASSERT_EQUAL(AsrStatus::BAD_INPUT, client.transcribe(one, 3).status);
    TEST_ASSERT_EQUAL(AsrStatus::BAD_INPUT, client.transcribe(big.data(), big.size()).status);
    TEST_ASSERT_EQUAL_UINT(0, rig.sock.connect_calls);
}

void test_invalid_provider_configuration() {
    const char* const bad_keys[] = {nullptr, "", "key with space", "key\r\nX-Evil: 1",
        "0123456789012345678901234567890123456789012345678901234567890123456789"};
    for (size_t i = 0; i < sizeof(bad_keys) / sizeof(bad_keys[0]); ++i) {
        GoogleSpeechV1Provider g(bad_keys[i]);
        TEST_ASSERT_FALSE(g.configured());
        Rig rig;
        TEST_ASSERT_EQUAL(AsrStatus::NOT_CONFIGURED, rig.run(pcm(100), &g).status);
        TEST_ASSERT_EQUAL_UINT(0, rig.sock.connect_calls);
    }
    TEST_ASSERT_FALSE(GoogleSpeechV1Provider(kKey, "en_US\"").configured());
    TEST_ASSERT_TRUE(GoogleSpeechV1Provider(kKey, "en-IN").configured());
}

// Another provider plugs in by implementing IAsrProvider (raw PCM upload).
class RawUploadProvider : public IAsrProvider {
public:
    const char* url() const override { return "https://asr.example.com:8443/listen?rate=16000"; }
    const char* content_type() const override { return "audio/l16; rate=16000"; }
    const char* extra_header() const override { return "Authorization: Token abc"; }
    bool audio_as_base64() const override { return false; }
    const char* body_prefix() const override { return ""; }
    const char* body_suffix() const override { return ""; }
    ParseResult parse_response(const char* body, size_t, char* t, size_t cap,
                               float& c) const override {
        snprintf(t, cap, "%s", body);
        c = -1.0f;
        return ParseResult::TRANSCRIPT;
    }
};

void test_other_providers_plug_in() {
    Rig rig;
    RawUploadProvider raw;
    rig.sock.response = "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nnext";
    std::vector<uint8_t> audio = pcm(600);
    AsrResult r = rig.run(audio, &raw);
    TEST_ASSERT_EQUAL(AsrStatus::OK, r.status);
    TEST_ASSERT_EQUAL_STRING("next", r.transcript);
    TEST_ASSERT_EQUAL_UINT16(8443, rig.sock.last_port);
    std::string h = rig.headers();
    TEST_ASSERT_EQUAL_INT(0, h.find("POST /listen?rate=16000 HTTP/1.1\r\n"));
    TEST_ASSERT_TRUE(h.find("\r\nHost: asr.example.com:8443\r\n") != std::string::npos);
    TEST_ASSERT_TRUE(h.find("\r\nAuthorization: Token abc\r\n") != std::string::npos);
    TEST_ASSERT_TRUE(h.find("\r\nContent-Length: 600\r\n") != std::string::npos);
    TEST_ASSERT_TRUE(rig.body() == std::string(audio.begin(), audio.end()));
}

void test_base64_rfc4648_vectors() {
    const char* in[] = {"", "f", "fo", "foo", "foob", "fooba", "foobar"};
    const char* out[] = {"", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy"};
    char buf[16];
    for (size_t i = 0; i < 7; ++i) {
        size_t n = asr_detail::base64_encode(reinterpret_cast<const uint8_t*>(in[i]),
                                             strlen(in[i]), buf);
        buf[n] = '\0';
        TEST_ASSERT_EQUAL_STRING(out[i], buf);
        TEST_ASSERT_EQUAL_UINT(strlen(out[i]), asr_detail::base64_length(strlen(in[i])));
    }
    const uint8_t bin[] = {0xFB, 0xFF, 0x00};
    size_t n = asr_detail::base64_encode(bin, 3, buf);
    buf[n] = '\0';
    TEST_ASSERT_EQUAL_STRING("+/8A", buf);
}

void test_dechunk_vectors() {
    char ok[] = "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n";
    size_t len = strlen(ok);
    TEST_ASSERT_TRUE(asr_detail::dechunk_in_place(ok, len));
    TEST_ASSERT_EQUAL_UINT(9, len);
    TEST_ASSERT_EQUAL_STRING("Wikipedia", ok);

    const char* bad[] = {"4\r\nWik", "4\r\nWikiXX0\r\n\r\n", "\r\n", "fffffff\r\nab\r\n0\r\n\r\n"};
    for (size_t i = 0; i < 4; ++i) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s", bad[i]);
        size_t l = strlen(buf);
        TEST_ASSERT_FALSE(asr_detail::dechunk_in_place(buf, l));
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_ASR_01_supported_command_is_transcribed_over_https);
    RUN_TEST(test_request_format_matches_sad_interface);
    RUN_TEST(test_max_capture_is_streamed_in_small_chunks);
    RUN_TEST(test_language_code_is_configurable);
    RUN_TEST(test_chunked_and_fragmented_responses_are_read);
    RUN_TEST(test_http_errors_are_mapped);
    RUN_TEST(test_silence_is_no_speech);
    RUN_TEST(test_transcript_parsing_details);
    RUN_TEST(test_overlong_transcript_is_rejected_not_truncated);
    RUN_TEST(test_malformed_responses_are_rejected);
    RUN_TEST(test_network_failures);
    RUN_TEST(test_timeout_and_latency_budget);
    RUN_TEST(test_insecure_channel_sends_no_audio);
    RUN_TEST(test_bad_audio_input_is_rejected_without_connecting);
    RUN_TEST(test_invalid_provider_configuration);
    RUN_TEST(test_other_providers_plug_in);
    RUN_TEST(test_base64_rfc4648_vectors);
    RUN_TEST(test_dechunk_vectors);
    return UNITY_END();
}
