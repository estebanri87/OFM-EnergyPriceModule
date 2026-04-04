#include "AwattarChannel.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

// aWATTar API – no API key required for basic market data.
// Germany: https://api.awattar.de/v1/marketdata
// Austria: https://api.awattar.at/v1/marketdata
// Response: {"data": [{"start_timestamp": <ms>, "end_timestamp": <ms>, "marketprice": <EUR/MWh>, "unit": "Eur/MWh"}, ...]}
// Conversion: ct/kWh = marketprice / 10.0

static const char* AWATTAR_URL_DE = "https://api.awattar.de/v1/marketdata";
static const char* AWATTAR_URL_AT = "https://api.awattar.at/v1/marketdata";

int16_t AwattarChannel::fillPrices(EnergyPriceHourlyData* prices, uint8_t maxCount)
{
    // ParamEP_CHCountry: 0=Germany, 1=Austria
    const char* url = (ParamEP_CHCountry == 1) ? AWATTAR_URL_AT : AWATTAR_URL_DE;

    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        logErrorP("aWATTar HTTP %d", httpCode);
        http.end();
        return -1;
    }

    // Use stream-based JSON parsing to reduce memory footprint
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err)
    {
        logErrorP("aWATTar JSON parse error: %s", err.c_str());
        return -1;
    }

    JsonArray data = doc["data"].as<JsonArray>();
    uint8_t count = 0;
    for (JsonObject slot : data)
    {
        if (count >= maxCount)
            break;
        // start_timestamp is in milliseconds → convert to seconds
        prices[count].startTimestamp  = (time_t)(slot["start_timestamp"].as<long long>() / 1000LL);
        prices[count].price_ct_per_kWh = slot["marketprice"].as<float>() / 10.0f;
        count++;
    }

    logDebugP("aWATTar: parsed %d price slots", count);
    return (int16_t)count;
}
