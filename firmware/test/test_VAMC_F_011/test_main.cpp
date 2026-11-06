#include <unity.h>
#include "wifi_provisioning.h"
#include <string.h>

using namespace vamc;

void setUp() {}
void tearDown() {}

class FakePortal : public WifiProvisioningPortal {
public:
    bool start_ok = true;
    bool submitted = false;
    bool started = false;
    bool stopped = false;
    const char* ssid = "test-network";
    const char* password = "test-password";

    bool start() override { started = true; return start_ok; }
    bool poll_submission(WifiCredentials& out) override {
        if (!submitted) return false;
        out = {ssid, password};
        submitted = false;
        return true;
    }
    void stop() override { stopped = true; }
};

class FakeWifi : public WifiConnection {
public:
    bool begin_ok = true;
    bool is_connected = false;
    unsigned begin_calls = 0;
    const char* received_ssid = nullptr;

    bool begin(const char* ssid, const char*) override {
        ++begin_calls;
        received_ssid = ssid;
        return begin_ok;
    }
    bool connected() const override { return is_connected; }
};

class FakeCredentialStore : public WifiCredentialStore {
public:
    bool save_ok = true;
    unsigned save_calls = 0;
    char saved_ssid[33] = {};
    char saved_password[65] = {};

    bool save_protected(const char* ssid, const char* password) override {
        ++save_calls;
        strcpy(saved_ssid, ssid);
        strcpy(saved_password, password);
        return save_ok;
    }
};

void test_TC_CONN_01_starts_guided_portal() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    WifiProvisioning setup(portal, wifi, store);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::SUCCESS),
                          static_cast<int>(setup.start(100)));
    TEST_ASSERT_TRUE(portal.started);
    TEST_ASSERT_FALSE(portal.stopped);
    TEST_ASSERT_EQUAL_UINT(0, wifi.begin_calls);
}

void test_TC_CONN_01_submits_credentials_and_saves_only_after_connection() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    WifiProvisioning setup(portal, wifi, store);
    setup.start(0);
    portal.submitted = true;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::SUCCESS),
                          static_cast<int>(setup.poll(10)));
    TEST_ASSERT_EQUAL_UINT(1, wifi.begin_calls);
    TEST_ASSERT_EQUAL_STRING("test-network", wifi.received_ssid);
    TEST_ASSERT_EQUAL_UINT(0, store.save_calls);
    wifi.is_connected = true;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::SUCCESS),
                          static_cast<int>(setup.poll(20)));
    TEST_ASSERT_EQUAL_UINT(1, store.save_calls);
    TEST_ASSERT_EQUAL_STRING("test-network", store.saved_ssid);
    TEST_ASSERT_EQUAL_STRING("test-password", store.saved_password);
    TEST_ASSERT_TRUE(portal.stopped);
}

void test_TC_CONN_01_rejects_empty_network_name() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    portal.ssid = "";
    portal.submitted = true;
    WifiProvisioning setup(portal, wifi, store);
    setup.start(0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::INVALID_CREDENTIALS),
                          static_cast<int>(setup.poll(1)));
    TEST_ASSERT_EQUAL_UINT(0, wifi.begin_calls);
}

void test_TC_CONN_01_reports_connection_start_failure() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    wifi.begin_ok = false;
    portal.submitted = true;
    WifiProvisioning setup(portal, wifi, store);
    setup.start(0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::CONNECTION_START_FAILED),
                          static_cast<int>(setup.poll(1)));
    TEST_ASSERT_EQUAL_UINT(0, store.save_calls);
}

void test_TC_CONN_01_times_out_at_five_minutes() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    WifiProvisioning setup(portal, wifi, store);
    setup.start(1000);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::SUCCESS),
                          static_cast<int>(setup.poll(300999)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::CONNECTION_TIMEOUT),
                          static_cast<int>(setup.poll(301000)));
    TEST_ASSERT_TRUE(portal.stopped);
}

void test_TC_CONN_01_reports_protected_save_failure() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    store.save_ok = false;
    portal.submitted = true;
    WifiProvisioning setup(portal, wifi, store);
    setup.start(0);
    setup.poll(1);
    wifi.is_connected = true;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::CREDENTIAL_SAVE_FAILED),
                          static_cast<int>(setup.poll(2)));
    TEST_ASSERT_TRUE(portal.stopped);
}

void test_TC_CONN_01_copies_and_clears_pending_credentials() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    char ssid[] = "owned-network";
    portal.ssid = ssid;
    WifiProvisioning setup(portal, wifi, store);
    setup.start(0); portal.submitted = true; setup.poll(1);
    ssid[0] = 'X';
    TEST_ASSERT_EQUAL_STRING("owned-network", wifi.received_ssid);
    setup.cancel();
    TEST_ASSERT_EQUAL_STRING("", wifi.received_ssid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::INACTIVE), static_cast<int>(setup.poll(2)));
    TEST_ASSERT_EQUAL_UINT(0, store.save_calls);
}
void test_TC_CONN_01_terminal_timeout_prevents_late_save() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    WifiProvisioning setup(portal, wifi, store);
    setup.start(0); portal.submitted = true; setup.poll(1);
    setup.poll(300000); wifi.is_connected = true;
    TEST_ASSERT_EQUAL_STRING("", wifi.received_ssid);
    setup.poll(300001);
    TEST_ASSERT_EQUAL_UINT(0, store.save_calls);
}
void test_TC_CONN_01_rejects_oversized_credentials() {
    FakePortal portal; FakeWifi wifi; FakeCredentialStore store;
    WifiProvisioning setup(portal, wifi, store);
    char long_ssid[34]; memset(long_ssid, 'x', 33); long_ssid[33] = 0;
    portal.ssid = long_ssid; portal.submitted = true; setup.start(0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WifiProvisioningResult::INVALID_CREDENTIALS), static_cast<int>(setup.poll(1)));
    TEST_ASSERT_EQUAL_UINT(0, wifi.begin_calls);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC_CONN_01_starts_guided_portal);
    RUN_TEST(test_TC_CONN_01_submits_credentials_and_saves_only_after_connection);
    RUN_TEST(test_TC_CONN_01_rejects_empty_network_name);
    RUN_TEST(test_TC_CONN_01_reports_connection_start_failure);
    RUN_TEST(test_TC_CONN_01_times_out_at_five_minutes);
    RUN_TEST(test_TC_CONN_01_reports_protected_save_failure);
    RUN_TEST(test_TC_CONN_01_copies_and_clears_pending_credentials);
    RUN_TEST(test_TC_CONN_01_terminal_timeout_prevents_late_save);
    RUN_TEST(test_TC_CONN_01_rejects_oversized_credentials);
    return UNITY_END();
}
