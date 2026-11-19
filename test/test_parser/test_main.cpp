// Testy parseru BLE vysílání na skutečných paketech z dokumentace:
//  - pvvx README (python-interface): custom formát, očekávané hodnoty z výstupu atc_mi_interface
//  - pvvx atc_mi_format_test.py: atc1441, custom_enc, atc1441_enc, bt_home_v2 (2 pakety), bt_home_v2_enc
//  - bthome.io/format: celé vysílání (flags + jméno + BTHome)
// Spuštění: pio test -e native
#include <string.h>
#include <unity.h>

#include "BleAdvParser.h"

static const uint8_t MAC_AABBCC[6] = {0xA4, 0xC1, 0x38, 0xAA, 0xBB, 0xCC};
static const uint8_t MAC_OTHER[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

void setUp() {}
void tearDown() {}

// pvvx README: general_format.parse("12 16 1a 18 5c c8 ee 38 c1 a4 b0 08 c3 14 ca 0a 50 14 05")
// → MAC A4:C1:38:EE:C8:5C, 22.24 °C, 53.15 %, 2.762 V, 80 %, counter 20
void test_pvvx_custom_readme()
{
    const uint8_t pkt[] = {0x12, 0x16, 0x1a, 0x18, 0x5c, 0xc8, 0xee, 0x38, 0xc1, 0xa4,
                           0xb0, 0x08, 0xc3, 0x14, 0xca, 0x0a, 0x50, 0x14, 0x05};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_OTHER, r));
    TEST_ASSERT_EQUAL(BLE_FMT_PVVX, r.format);
    char mac[18];
    bleFormatMac(r.mac, mac);
    TEST_ASSERT_EQUAL_STRING("A4:C1:38:EE:C8:5C", mac);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 22.24f, r.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 53.15f, r.humidity);
    TEST_ASSERT_EQUAL_UINT16(2762, r.batteryMv);
    TEST_ASSERT_EQUAL_UINT8(80, r.batteryPct);
    TEST_ASSERT_EQUAL_UINT8(20, r.counter);
}

// pvvx README, druhý příklad: frame1 → 19.11 °C, 50.0 %, 80 %, 2.762 V
void test_pvvx_custom_frame1()
{
    const uint8_t pkt[] = {0x12, 0x16, 0x1a, 0x18, 0xcc, 0xbb, 0xaa, 0x38, 0xc1, 0xa4,
                           0x77, 0x07, 0x88, 0x13, 0xca, 0x0a, 0x50, 0x01, 0x07};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_AABBCC, r));
    TEST_ASSERT_EQUAL_MEMORY(MAC_AABBCC, r.mac, 6);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 19.11f, r.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, r.humidity);
    TEST_ASSERT_EQUAL_UINT8(80, r.batteryPct);
    TEST_ASSERT_EQUAL_UINT16(2762, r.batteryMv);
}

// atc_mi_format_test.py "custom": 0x076c = 19.00 °C, 0x13fa = 51.14 %, 0x0ad6 = 2774 mV, 82 %, counter 9
void test_pvvx_custom_format_test()
{
    const uint8_t pkt[] = {0x12, 0x16, 0x1a, 0x18, 0xcc, 0xbb, 0xaa, 0x38, 0xc1, 0xa4,
                           0x6c, 0x07, 0xfa, 0x13, 0xd6, 0x0a, 0x52, 0x09, 0x0f};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_AABBCC, r));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 19.00f, r.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 51.14f, r.humidity);
    TEST_ASSERT_EQUAL_UINT16(2774, r.batteryMv);
    TEST_ASSERT_EQUAL_UINT8(82, r.batteryPct);
    TEST_ASSERT_EQUAL_UINT8(9, r.counter);
}

// atc_mi_format_test.py "atc1441": "10 16 1a 18 a4 c1 38 aa bb cc 00 ce 33 43 0a 6e c3"
// big-endian: 0x00ce = 20.6 °C, 0x33 = 51 %, 0x43 = 67 %, 0x0a6e = 2670 mV, counter 0xc3
void test_atc1441()
{
    const uint8_t pkt[] = {0x10, 0x16, 0x1a, 0x18, 0xa4, 0xc1, 0x38, 0xaa, 0xbb, 0xcc,
                           0x00, 0xce, 0x33, 0x43, 0x0a, 0x6e, 0xc3};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_OTHER, r));
    TEST_ASSERT_EQUAL(BLE_FMT_ATC1441, r.format);
    TEST_ASSERT_EQUAL_MEMORY(MAC_AABBCC, r.mac, 6);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.6f, r.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 51.0f, r.humidity);
    TEST_ASSERT_EQUAL_UINT8(67, r.batteryPct);
    TEST_ASSERT_EQUAL_UINT16(2670, r.batteryMv);
    TEST_ASSERT_EQUAL_UINT8(0xc3, r.counter);
}

// Záporná teplota v ATC1441 (int16 big-endian): -5.3 °C = 0xffcb. Sestaveno ručně podle formátu.
void test_atc1441_negative()
{
    const uint8_t pkt[] = {0x10, 0x16, 0x1a, 0x18, 0xa4, 0xc1, 0x38, 0xaa, 0xbb, 0xcc,
                           0xff, 0xcb, 0x50, 0x10, 0x0a, 0x00, 0x01};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_OTHER, r));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -5.3f, r.temperature);
}

// Záporná teplota v custom (int16 little-endian): -12.34 °C = 0xfb2e
void test_pvvx_custom_negative()
{
    const uint8_t pkt[] = {0x12, 0x16, 0x1a, 0x18, 0xcc, 0xbb, 0xaa, 0x38, 0xc1, 0xa4,
                           0x2e, 0xfb, 0x88, 0x13, 0xca, 0x0a, 0x50, 0x01, 0x07};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_AABBCC, r));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -12.34f, r.temperature);
}

// atc_mi_format_test.py "bt_home_v2 (temp, humidity, batt%)": "0e 16 d2 fc 40 00 d6 01 50 02 80 07 03 8a 13"
// packet id 0xd6, baterie 80 %, 0x0780 = 19.20 °C, 0x138a = 50.02 %
void test_bthome_v2_temp_packet()
{
    const uint8_t pkt[] = {0x0e, 0x16, 0xd2, 0xfc, 0x40, 0x00, 0xd6, 0x01, 0x50,
                           0x02, 0x80, 0x07, 0x03, 0x8a, 0x13};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_AABBCC, r));
    TEST_ASSERT_EQUAL(BLE_FMT_BTHOME, r.format);
    TEST_ASSERT_EQUAL_MEMORY(MAC_AABBCC, r.mac, 6);
    TEST_ASSERT_TRUE(r.hasTemperature);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 19.20f, r.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.02f, r.humidity);
    TEST_ASSERT_EQUAL_UINT8(80, r.batteryPct);
    TEST_ASSERT_FALSE(r.hasBatteryMv);
    TEST_ASSERT_EQUAL_UINT8(0xd6, r.counter);
}

// atc_mi_format_test.py "bt_home_v2 (batt v, switch)": "0b 16 d2 fc 40 00 d9 0c d1 0a 10 00"
// 0x0ad1 = 2769 mV, objekt 0x10 (power) přeskočit
void test_bthome_v2_voltage_packet()
{
    const uint8_t pkt[] = {0x0b, 0x16, 0xd2, 0xfc, 0x40, 0x00, 0xd9, 0x0c, 0xd1, 0x0a, 0x10, 0x00};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_AABBCC, r));
    TEST_ASSERT_TRUE(r.hasBatteryMv);
    TEST_ASSERT_EQUAL_UINT16(2769, r.batteryMv);
    TEST_ASSERT_FALSE(r.hasTemperature);
}

// bthome.io/format: celé vysílání "020106 0B094449592D73656E736F72 0A16D2FC4002C40903BF13"
// → 25.00 °C, 50.55 %
void test_bthome_io_full_advertisement()
{
    const uint8_t pkt[] = {0x02, 0x01, 0x06,
                           0x0B, 0x09, 0x44, 0x49, 0x59, 0x2D, 0x73, 0x65, 0x6E, 0x73, 0x6F, 0x72,
                           0x0A, 0x16, 0xD2, 0xFC, 0x40, 0x02, 0xC4, 0x09, 0x03, 0xBF, 0x13};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_OK, bleParseAdvertisement(pkt, sizeof(pkt), MAC_OTHER, r));
    TEST_ASSERT_EQUAL_MEMORY(MAC_OTHER, r.mac, 6);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.00f, r.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.55f, r.humidity);
}

// Šifrované varianty z atc_mi_format_test.py se jen rozpoznají.
void test_encrypted_variants()
{
    const uint8_t customEnc[] = {0x0e, 0x16, 0x1a, 0x18, 0xbd, 0x86, 0xc5, 0x3f, 0xfa, 0xb9, 0x00, 0xc1, 0x51, 0x58, 0x59};
    const uint8_t atcEnc[] = {0x0b, 0x16, 0x1a, 0x18, 0xbd, 0xe9, 0xb7, 0x5d, 0xb8, 0x56, 0xf3, 0x03};
    const uint8_t bthomeEnc[] = {0x14, 0x16, 0xd2, 0xfc, 0x41, 0x95, 0xb0, 0xef, 0xda, 0x78, 0x9d, 0xd9, 0x53,
                                 0xb0, 0x26, 0x00, 0x00, 0x93, 0x41, 0x62, 0x6f};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_ENCRYPTED, bleParseAdvertisement(customEnc, sizeof(customEnc), MAC_AABBCC, r));
    TEST_ASSERT_EQUAL(BLE_PARSE_ENCRYPTED, bleParseAdvertisement(atcEnc, sizeof(atcEnc), MAC_AABBCC, r));
    TEST_ASSERT_EQUAL(BLE_PARSE_ENCRYPTED, bleParseAdvertisement(bthomeEnc, sizeof(bthomeEnc), MAC_AABBCC, r));
}

// Cizí zařízení a rozbitá data nesmí spadnout ani vrátit OK.
void test_foreign_and_broken()
{
    // Apple iBeacon-like: flags + manufacturer data
    const uint8_t apple[] = {0x02, 0x01, 0x1a, 0x0b, 0xff, 0x4c, 0x00, 0x09, 0x06, 0x03, 0x1e, 0xc0, 0xa8, 0x01, 0x0b};
    // délka elementu přesahuje konec
    const uint8_t truncated[] = {0x12, 0x16, 0x1a, 0x18, 0x5c, 0xc8};
    // 0x181A s nesmyslnou délkou
    const uint8_t weird[] = {0x05, 0x16, 0x1a, 0x18, 0x01, 0x02};
    BleThermoReading r;
    TEST_ASSERT_EQUAL(BLE_PARSE_NOT_THERMO, bleParseAdvertisement(apple, sizeof(apple), MAC_OTHER, r));
    TEST_ASSERT_EQUAL(BLE_PARSE_MALFORMED, bleParseAdvertisement(truncated, sizeof(truncated), MAC_OTHER, r));
    TEST_ASSERT_EQUAL(BLE_PARSE_MALFORMED, bleParseAdvertisement(weird, sizeof(weird), MAC_OTHER, r));
    TEST_ASSERT_EQUAL(BLE_PARSE_NOT_THERMO, bleParseAdvertisement(apple, 0, MAC_OTHER, r));
}

void test_parse_mac()
{
    uint8_t mac[6];
    TEST_ASSERT_TRUE(bleParseMac("a4:c1:38:aa:bb:cc", mac));
    TEST_ASSERT_EQUAL_MEMORY(MAC_AABBCC, mac, 6);
    TEST_ASSERT_FALSE(bleParseMac("A4:C1:38:AA:BB", mac));
    TEST_ASSERT_FALSE(bleParseMac("A4-C1-38-AA-BB-CC", mac));
    TEST_ASSERT_FALSE(bleParseMac("A4:C1:38:AA:BB:CX", mac));
    TEST_ASSERT_FALSE(bleParseMac(NULL, mac));
}

// Úložiště skenu: povolené čidlo slučuje dva pakety BTHome, neznámé jde do seznamu, šifrované se počítá.
void test_store_merge_and_unknown()
{
    const BleSensorConfig cfg[] = {
        {"A4:C1:38:AA:BB:CC", "Ul A"},
        {"spatna-mac", "Nic"},
    };
    BleSensorStore store;
    TEST_ASSERT_EQUAL(1, store.configure(cfg, 2));

    const uint8_t p1[] = {0x0e, 0x16, 0xd2, 0xfc, 0x40, 0x00, 0xd6, 0x01, 0x50, 0x02, 0x80, 0x07, 0x03, 0x8a, 0x13};
    const uint8_t p2[] = {0x0b, 0x16, 0xd2, 0xfc, 0x40, 0x00, 0xd9, 0x0c, 0xd1, 0x0a, 0x10, 0x00};
    store.onAdvertisement(MAC_AABBCC, -80, p1, sizeof(p1));
    store.onAdvertisement(MAC_AABBCC, -71, p2, sizeof(p2));

    const BleSensorSlot &s = store.sensors[0];
    TEST_ASSERT_TRUE(s.seen);
    TEST_ASSERT_EQUAL_STRING("Ul A", s.name);
    TEST_ASSERT_EQUAL_INT8(-71, s.rssi);
    TEST_ASSERT_EQUAL_UINT16(2, s.packets);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 19.20f, s.reading.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.02f, s.reading.humidity);
    TEST_ASSERT_EQUAL_UINT8(80, s.reading.batteryPct);
    TEST_ASSERT_EQUAL_UINT16(2769, s.reading.batteryMv);
    TEST_ASSERT_EQUAL_UINT8(0xd9, s.reading.counter);

    // neznámé čidlo (MAC z dat custom formátu), dvakrát = jeden záznam
    const uint8_t other[] = {0x12, 0x16, 0x1a, 0x18, 0x5c, 0xc8, 0xee, 0x38, 0xc1, 0xa4,
                             0xb0, 0x08, 0xc3, 0x14, 0xca, 0x0a, 0x50, 0x14, 0x05};
    store.onAdvertisement(MAC_OTHER, -90, other, sizeof(other));
    store.onAdvertisement(MAC_OTHER, -85, other, sizeof(other));
    TEST_ASSERT_EQUAL(1, store.unknownCount);
    TEST_ASSERT_EQUAL_INT8(-85, store.unknownRssi[0]);
    char mac[18];
    bleFormatMac(store.unknown[0], mac);
    TEST_ASSERT_EQUAL_STRING("A4:C1:38:EE:C8:5C", mac);

    const uint8_t atcEnc[] = {0x0b, 0x16, 0x1a, 0x18, 0xbd, 0xe9, 0xb7, 0x5d, 0xb8, 0x56, 0xf3, 0x03};
    store.onAdvertisement(MAC_AABBCC, -70, atcEnc, sizeof(atcEnc));
    TEST_ASSERT_EQUAL_UINT16(1, store.encryptedPackets);
    TEST_ASSERT_EQUAL(1, store.seenCount());

    store.resetScan();
    TEST_ASSERT_FALSE(store.sensors[0].seen);
    TEST_ASSERT_EQUAL(0, store.unknownCount);
    TEST_ASSERT_EQUAL(0, store.seenCount());
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_pvvx_custom_readme);
    RUN_TEST(test_pvvx_custom_frame1);
    RUN_TEST(test_pvvx_custom_format_test);
    RUN_TEST(test_atc1441);
    RUN_TEST(test_atc1441_negative);
    RUN_TEST(test_pvvx_custom_negative);
    RUN_TEST(test_bthome_v2_temp_packet);
    RUN_TEST(test_bthome_v2_voltage_packet);
    RUN_TEST(test_bthome_io_full_advertisement);
    RUN_TEST(test_encrypted_variants);
    RUN_TEST(test_foreign_and_broken);
    RUN_TEST(test_parse_mac);
    RUN_TEST(test_store_merge_and_unknown);
    return UNITY_END();
}
