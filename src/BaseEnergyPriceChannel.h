#pragma once
#include "OpenKNX.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"

// Maximum number of hourly price slots stored (24h today + up to 24h tomorrow)
#define EP_MAX_HOURLY_PRICES 48

// Price level thresholds - can be overridden via ETS parameters
// 0 = cheap, 1 = normal, 2 = expensive
#define EP_PRICE_LEVEL_CHEAP    0
#define EP_PRICE_LEVEL_NORMAL   1
#define EP_PRICE_LEVEL_EXPENSIVE 2

struct EnergyPriceHourlyData
{
    time_t startTimestamp = 0;  // Unix timestamp (seconds)
    float  price_ct_per_kWh = 0.0f;
};

class BaseEnergyPriceChannel : public OpenKNX::Channel
{
protected:
    uint8_t  _channelIndex = 0;
    uint32_t _lastApiCall = 0;
    uint32_t _updateIntervalInMs = 0;
    bool     _available = false;

    // Raw hourly price data
    EnergyPriceHourlyData _hourlyPrices[EP_MAX_HOURLY_PRICES];
    uint8_t _numPrices = 0;

    // Derived values (recalculated after each fetch)
    float   _currentPrice_ct = 0.0f;
    float   _avgPriceToday_ct = 0.0f;
    float   _minPriceToday_ct = 0.0f;
    float   _maxPriceToday_ct = 0.0f;
    uint8_t _priceLevel = EP_PRICE_LEVEL_NORMAL;  // 0=cheap, 1=normal, 2=expensive
    time_t  _cheapestWindowStart = 0;             // start of cheapest N-hour window
    bool    _tomorrowAvailable = false;

    // Provider-specific HTTP fetch + JSON parse -> fills _hourlyPrices[]
    // Returns number of prices filled, or -1 on error
    virtual int16_t fillPrices(EnergyPriceHourlyData* prices, uint8_t maxPrices) = 0;

    // Recalculates all derived values from _hourlyPrices[]
    void calculateDerivedValues();

    // Pushes all derived values to KNX group objects
    void publishKos();

public:
    BaseEnergyPriceChannel(uint8_t index);

    void setup() override;
    void loop() override;
    void processInputKo(GroupObject& ko) override;

    bool processCommand(const std::string cmd, bool diagnoseKo);
    void fetchData();
};
