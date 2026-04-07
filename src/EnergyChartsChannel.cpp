#include "EnergyChartsChannel.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Energy Charts API (Fraunhofer ISE) – no API key required.
// Endpoint: GET https://api.energy-charts.info/price?bzn=<zone>&start=<ISO8601>&end=<ISO8601>
// Response:
//   {
//     "unix_seconds": [1744...],   ← UTC timestamps, start of each hour
//     "price":        [85.2, ...], ← EUR/MWh (EPEX SPOT day-ahead)
//     "unit":         "EUR/MWh"
//   }
// Conversion to ct/kWh: price_EUR_per_MWh / 10.0
//
// Bidding zones (ParamEP_CHEnergyChartsBZN):
//   0=DE-LU  1=AT  2=CH  3=BE  4=FR  5=NL
//   6=DK1    7=DK2 8=NO1 9=NO2 10=NO3 11=NO4 12=NO5
//   13=SE1  14=SE2 15=SE3 16=SE4

static const char* EC_BZN_CODES[] = {
    "DE-LU", "AT", "CH", "BE", "FR", "NL",
    "DK1", "DK2",
    "NO1", "NO2", "NO3", "NO4", "NO5",
    "SE1", "SE2", "SE3", "SE4"
};
static const uint8_t EC_BZN_COUNT = sizeof(EC_BZN_CODES) / sizeof(EC_BZN_CODES[0]);

int16_t EnergyChartsChannel::fillPrices(EnergyPriceHourlyData* prices, uint8_t maxCount)
{
    uint8_t bznIndex = ParamEP_CHEnergyChartsBZN;
    if (bznIndex >= EC_BZN_COUNT)
        bznIndex = 0;
    const char* bzn = EC_BZN_CODES[bznIndex];

    // Fetch today + tomorrow: start = today 00:00 local, end = tomorrow 23:59 local
    time_t now = time(nullptr);
    struct tm tmStart;
    localtime_r(&now, &tmStart);
    tmStart.tm_hour = 0; tmStart.tm_min = 0; tmStart.tm_sec = 0;
    time_t startTs = mktime(&tmStart);

    struct tm tmEnd = tmStart;
    tmEnd.tm_mday += 2;
    time_t endTs = mktime(&tmEnd) - 1;

    // Format as ISO 8601 with UTC offset (energy-charts requires timezone in URL)
    // Use +00:00 (UTC) — the API returns unix_seconds in UTC regardless
    char startStr[30], endStr[30];
    struct tm tmUtcStart, tmUtcEnd;
    gmtime_r(&startTs, &tmUtcStart);
    gmtime_r(&endTs,   &tmUtcEnd);
    // Note: '+' must be percent-encoded as '%2B' in URL query parameters
    strftime(startStr, sizeof(startStr), "%Y-%m-%dT%H:%M%%2B00:00", &tmUtcStart);
    strftime(endStr,   sizeof(endStr),   "%Y-%m-%dT%H:%M%%2B00:00", &tmUtcEnd);

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.energy-charts.info/price?bzn=%s&start=%s&end=%s",
             bzn, startStr, endStr);
    logDebugP("Energy Charts URL: %s", url);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(8000);
#ifdef ARDUINO_ARCH_RP2040
    if (String(url).startsWith("https://"))
        http.setInsecure();
#endif
    openknx.watchdog.loop();
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        logErrorP("Energy Charts HTTP %d", httpCode);
        http.end();
        return -1;
    }

    String responseBody = http.getString();
    http.end();

    openknx.watchdog.loop();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, responseBody);
    if (err)
    {
        logErrorP("Energy Charts JSON parse error: %s", err.c_str());
        return -1;
    }

    JsonArray timestamps = doc["unix_seconds"].as<JsonArray>();
    JsonArray pricesArr  = doc["price"].as<JsonArray>();

    if (timestamps.isNull() || pricesArr.isNull())
    {
        logErrorP("Energy Charts: missing 'unix_seconds' or 'price' in response");
        return -1;
    }

    uint8_t count = 0;
    auto tsIt = timestamps.begin();
    auto prIt = pricesArr.begin();

    while (tsIt != timestamps.end() && prIt != pricesArr.end() && count < maxCount)
    {
        // price may be null during gaps (e.g. Switzerland on some days)
        if (!(*prIt).isNull())
        {
            prices[count].startTimestamp   = (time_t)(*tsIt).as<long long>();
            prices[count].price_ct_per_kWh = (*prIt).as<float>() / 10.0f;
            count++;
        }
        ++tsIt;
        ++prIt;
    }

    logDebugP("Energy Charts (%s): parsed %d price slots", bzn, count);
    return (int16_t)count;
}
