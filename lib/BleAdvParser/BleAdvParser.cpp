#include "BleAdvParser.h"

#include <stdio.h>
#include <string.h>

static const uint16_t UUID_ENV_SENSING = 0x181A; // pvvx custom a ATC1441
static const uint16_t UUID_BTHOME = 0xFCD2;

static const uint8_t AD_SERVICE_DATA_16 = 0x16;

static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

void bleReadingClear(BleThermoReading &r)
{
    memset(&r, 0, sizeof(r));
}

static BleParseResult parsePvvxCustom(const uint8_t *d, BleThermoReading &out)
{
    // MAC[6] (lo..hi), int16 t x0.01, uint16 h x0.01, uint16 mV, uint8 %, uint8 counter, uint8 flags
    for (int i = 0; i < 6; i++)
        out.mac[i] = d[5 - i];
    out.format = BLE_FMT_PVVX;
    out.temperature = (int16_t)le16(d + 6) / 100.0f;
    out.humidity = le16(d + 8) / 100.0f;
    out.batteryMv = le16(d + 10);
    out.batteryPct = d[12];
    out.counter = d[13];
    out.hasTemperature = out.hasHumidity = out.hasBatteryMv = out.hasBatteryPct = out.hasCounter = true;
    return BLE_PARSE_OK;
}

static BleParseResult parseAtc1441(const uint8_t *d, BleThermoReading &out)
{
    // MAC[6] (hi..lo), int16 BE t x0.1, uint8 h %, uint8 bat %, uint16 BE mV, uint8 counter
    memcpy(out.mac, d, 6);
    out.format = BLE_FMT_ATC1441;
    out.temperature = (int16_t)be16(d + 6) / 10.0f;
    out.humidity = d[8];
    out.batteryPct = d[9];
    out.batteryMv = be16(d + 10);
    out.counter = d[12];
    out.hasTemperature = out.hasHumidity = out.hasBatteryMv = out.hasBatteryPct = out.hasCounter = true;
    return BLE_PARSE_OK;
}

// Délka hodnoty objektu BTHome v2 podle https://bthome.io/format/ (stav 11/2026).
// -1 = proměnná délka (další bajt je délka), 0 = neznámý objekt (dál nečteme).
static int bthomeObjectSize(uint8_t id)
{
    switch (id)
    {
    case 0x00: case 0x01: case 0x09: case 0x0F: case 0x10: case 0x11:
    case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1A: case 0x1B:
    case 0x1C: case 0x1D: case 0x1E: case 0x1F: case 0x20: case 0x21: case 0x22:
    case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x28: case 0x29:
    case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F: case 0x3A:
    case 0x46: case 0x57: case 0x58: case 0x59: case 0x60: case 0x64: case 0x65:
        return 1;
    case 0x02: case 0x03: case 0x06: case 0x07: case 0x08: case 0x0C: case 0x0D:
    case 0x0E: case 0x12: case 0x13: case 0x14: case 0x3C: case 0x3D: case 0x3F:
    case 0x40: case 0x41: case 0x43: case 0x44: case 0x45: case 0x47: case 0x48:
    case 0x49: case 0x4A: case 0x51: case 0x52: case 0x56: case 0x5A: case 0x5D:
    case 0x5E: case 0x5F: case 0x61: case 0xF0:
        return 2;
    case 0x04: case 0x05: case 0x0A: case 0x0B: case 0x42: case 0x4B: case 0xF2:
        return 3;
    case 0x3E: case 0x4C: case 0x4D: case 0x4E: case 0x4F: case 0x50: case 0x55:
    case 0x5B: case 0x5C: case 0x62: case 0x63: case 0xF1:
        return 4;
    case 0x53: case 0x54:
        return -1;
    default:
        return 0;
    }
}

static BleParseResult parseBthome(const uint8_t *d, size_t len, const uint8_t advMac[6], BleThermoReading &out)
{
    if (len < 1)
        return BLE_PARSE_MALFORMED;
    uint8_t info = d[0];
    if ((info >> 5) != 2)
        return BLE_PARSE_NOT_THERMO; // jen BTHome v2
    if (info & 0x01)
        return BLE_PARSE_ENCRYPTED;

    memcpy(out.mac, advMac, 6);
    out.format = BLE_FMT_BTHOME;
    bool any = false;
    size_t i = 1;
    while (i < len)
    {
        uint8_t id = d[i++];
        int size = bthomeObjectSize(id);
        if (size == 0)
            break; // neznámý objekt: délku neznáme, zbytek nejde přečíst
        if (size < 0)
        {
            if (i >= len)
                break;
            size = d[i++];
        }
        if (i + (size_t)size > len)
            break;
        const uint8_t *v = d + i;
        switch (id)
        {
        case 0x00:
            out.counter = v[0];
            out.hasCounter = true;
            break;
        case 0x01:
            out.batteryPct = v[0];
            out.hasBatteryPct = any = true;
            break;
        case 0x02:
            out.temperature = (int16_t)le16(v) / 100.0f;
            out.hasTemperature = any = true;
            break;
        case 0x45:
            out.temperature = (int16_t)le16(v) / 10.0f;
            out.hasTemperature = any = true;
            break;
        case 0x03:
            out.humidity = le16(v) / 100.0f;
            out.hasHumidity = any = true;
            break;
        case 0x2E:
            out.humidity = v[0];
            out.hasHumidity = any = true;
            break;
        case 0x0C:
            out.batteryMv = le16(v); // 0,001 V = mV
            out.hasBatteryMv = any = true;
            break;
        default:
            break;
        }
        i += size;
    }
    return any ? BLE_PARSE_OK : BLE_PARSE_NOT_THERMO;
}

BleParseResult bleParseServiceData(uint16_t uuid, const uint8_t *data, size_t len, const uint8_t advMac[6], BleThermoReading &out)
{
    bleReadingClear(out);
    if (uuid == UUID_ENV_SENSING)
    {
        if (len == 15)
            return parsePvvxCustom(data, out);
        if (len == 13)
            return parseAtc1441(data, out);
        if (len == 11 || len == 8)
            return BLE_PARSE_ENCRYPTED; // custom_enc (11) / atc1441_enc (8)
        return BLE_PARSE_MALFORMED;
    }
    if (uuid == UUID_BTHOME)
        return parseBthome(data, len, advMac, out);
    return BLE_PARSE_NOT_THERMO;
}

BleParseResult bleParseAdvertisement(const uint8_t *payload, size_t len, const uint8_t advMac[6], BleThermoReading &out)
{
    BleParseResult best = BLE_PARSE_NOT_THERMO;
    size_t i = 0;
    while (i < len)
    {
        uint8_t elen = payload[i];
        if (elen == 0)
            break;
        if (i + 1 + elen > len)
            return best == BLE_PARSE_NOT_THERMO ? BLE_PARSE_MALFORMED : best;
        uint8_t type = payload[i + 1];
        if (type == AD_SERVICE_DATA_16 && elen >= 3)
        {
            uint16_t uuid = le16(payload + i + 2);
            BleParseResult r = bleParseServiceData(uuid, payload + i + 4, elen - 3, advMac, out);
            if (r == BLE_PARSE_OK)
                return r;
            if (r != BLE_PARSE_NOT_THERMO)
                best = r;
        }
        i += 1 + elen;
    }
    if (best != BLE_PARSE_OK)
        bleReadingClear(out);
    return best;
}

void bleMergeReading(BleThermoReading &into, const BleThermoReading &from)
{
    memcpy(into.mac, from.mac, 6);
    into.format = from.format;
    if (from.hasTemperature)
    {
        into.temperature = from.temperature;
        into.hasTemperature = true;
    }
    if (from.hasHumidity)
    {
        into.humidity = from.humidity;
        into.hasHumidity = true;
    }
    if (from.hasBatteryPct)
    {
        into.batteryPct = from.batteryPct;
        into.hasBatteryPct = true;
    }
    if (from.hasBatteryMv)
    {
        into.batteryMv = from.batteryMv;
        into.hasBatteryMv = true;
    }
    if (from.hasCounter)
    {
        into.counter = from.counter;
        into.hasCounter = true;
    }
}

static int hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

bool bleParseMac(const char *text, uint8_t mac[6])
{
    if (text == NULL || strlen(text) != 17)
        return false;
    for (int i = 0; i < 6; i++)
    {
        int hi = hexValue(text[i * 3]);
        int lo = hexValue(text[i * 3 + 1]);
        if (hi < 0 || lo < 0)
            return false;
        if (i < 5 && text[i * 3 + 2] != ':')
            return false;
        mac[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

void bleFormatMac(const uint8_t mac[6], char out[18])
{
    snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

const char *bleFormatName(BleFormat f)
{
    switch (f)
    {
    case BLE_FMT_PVVX:
        return "pvvx";
    case BLE_FMT_ATC1441:
        return "atc1441";
    case BLE_FMT_BTHOME:
        return "bthome";
    default:
        return "";
    }
}

// ---------------------------------------------------------------------------

int BleSensorStore::configure(const BleSensorConfig *config, size_t count)
{
    sensorCount = 0;
    for (size_t i = 0; i < count && sensorCount < MAX_SENSORS; i++)
    {
        BleSensorSlot &slot = sensors[sensorCount];
        if (!bleParseMac(config[i].mac, slot.mac))
            continue;
        slot.name = config[i].name;
        sensorCount++;
    }
    resetScan();
    return sensorCount;
}

void BleSensorStore::resetScan()
{
    for (int i = 0; i < sensorCount; i++)
    {
        sensors[i].seen = false;
        sensors[i].rssi = -128;
        sensors[i].packets = 0;
        bleReadingClear(sensors[i].reading);
    }
    unknownCount = 0;
    encryptedPackets = 0;
}

int BleSensorStore::findSlot(const uint8_t mac[6]) const
{
    for (int i = 0; i < sensorCount; i++)
        if (memcmp(sensors[i].mac, mac, 6) == 0)
            return i;
    return -1;
}

int BleSensorStore::seenCount() const
{
    int n = 0;
    for (int i = 0; i < sensorCount; i++)
        if (sensors[i].seen)
            n++;
    return n;
}

BleParseResult BleSensorStore::onAdvertisement(const uint8_t advMac[6], int rssi, const uint8_t *payload, size_t len)
{
    BleThermoReading r;
    BleParseResult res = bleParseAdvertisement(payload, len, advMac, r);
    if (res == BLE_PARSE_ENCRYPTED)
    {
        if (encryptedPackets < 0xFFFF)
            encryptedPackets++;
        return res;
    }
    if (res != BLE_PARSE_OK)
        return res;

    int8_t rssi8 = rssi < -128 ? -128 : (rssi > 127 ? 127 : (int8_t)rssi);
    // custom/ATC nesou MAC v datech; vysílají ji ale i jako adresu, bereme tu z dat
    int idx = findSlot(r.mac);
    if (idx >= 0)
    {
        BleSensorSlot &slot = sensors[idx];
        bleMergeReading(slot.reading, r);
        slot.seen = true;
        if (rssi8 > slot.rssi)
            slot.rssi = rssi8;
        if (slot.packets < 0xFFFF)
            slot.packets++;
        return res;
    }

    for (int i = 0; i < unknownCount; i++)
    {
        if (memcmp(unknown[i], r.mac, 6) == 0)
        {
            bleMergeReading(unknownReading[i], r);
            if (rssi8 > unknownRssi[i])
                unknownRssi[i] = rssi8;
            return res;
        }
    }
    if (unknownCount < MAX_UNKNOWN)
    {
        memcpy(unknown[unknownCount], r.mac, 6);
        unknownRssi[unknownCount] = rssi8;
        unknownReading[unknownCount] = r;
        unknownCount++;
    }
    return res;
}
