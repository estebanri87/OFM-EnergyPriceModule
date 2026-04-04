#include "BaseEnergyPriceChannel.h"
#ifdef WLAN_WifiSSID
    #include "WiFi.h"
#else
    #include "NetworkModule.h"
#endif

BaseEnergyPriceChannel::BaseEnergyPriceChannel(uint8_t index)
{
    _channelIndex = index;
}

void BaseEnergyPriceChannel::setup()
{
    // <Enumeration Text="Keine"       Value="0" Id="%ENID%" />
    // <Enumeration Text="30 Minuten"  Value="1" Id="%ENID%" />
    // <Enumeration Text="Jede Stunde" Value="2" Id="%ENID%" />
    // <Enumeration Text="Täglich"     Value="3" Id="%ENID%" />
    switch (ParamEP_CHRefreshInterval)
    {
        case 1:
            _updateIntervalInMs = 30 * 60 * 1000;
            break;
        case 2:
            _updateIntervalInMs = 60 * 60 * 1000;
            break;
        case 3:
            _updateIntervalInMs = 24 * 60 * 60 * 1000;
            break;
        default:
            _updateIntervalInMs = 0;
            break;
    }
    logDebugP("Update interval: %ldms", _updateIntervalInMs);
}

void BaseEnergyPriceChannel::loop()
{
#ifdef WLAN_WifiSSID
    if (WiFi.isConnected())
#else
    if (openknxNetwork.established())
#endif
    {
        auto now = millis();
        if (now == 0)
            now++;  // 0 is used as "uninitialized" marker

        if (_updateIntervalInMs > 0 &&
            (_lastApiCall == 0 || (now - _lastApiCall > _updateIntervalInMs)))
        {
            _lastApiCall = now;
            fetchData();
        }
    }
}

void BaseEnergyPriceChannel::processInputKo(GroupObject& ko)
{
    switch (ko.asap())
    {
        case EP_KoRefreshData:
            if (ko.value(DPT_Trigger))
                fetchData();
            break;
    }
}

bool BaseEnergyPriceChannel::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd == "update")
    {
        fetchData();
        return true;
    }
    return false;
}

void BaseEnergyPriceChannel::fetchData()
{
    logInfoP("Fetching energy price data (channel %d)", _channelIndex);
    int16_t count = fillPrices(_hourlyPrices, EP_MAX_HOURLY_PRICES);
    if (count < 0)
    {
        logErrorP("Failed to fetch energy prices (channel %d)", _channelIndex);
        _available = false;
        return;
    }
    _numPrices = (uint8_t)count;
    _available = true;
    logInfoP("Received %d hourly prices", _numPrices);
    calculateDerivedValues();
    publishKos();
}

void BaseEnergyPriceChannel::calculateDerivedValues()
{
    if (_numPrices == 0)
        return;

    // Determine today's date boundaries (midnight local time)
    time_t now = time(nullptr);
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    tmNow.tm_hour = 0;
    tmNow.tm_min  = 0;
    tmNow.tm_sec  = 0;
    time_t todayStart = mktime(&tmNow);
    time_t tomorrowStart = todayStart + 86400;

    float sum = 0.0f;
    float minPrice = 1e9f;
    float maxPrice = -1e9f;
    uint8_t countToday = 0;

    // Cheapest N-hour window: configurable (default 1)
    uint8_t windowSize = ParamEP_CHCheapestWindowHours;
    if (windowSize < 1) windowSize = 1;
    float cheapestWindowSum = 1e9f;
    time_t cheapestWindowStart = 0;

    for (uint8_t i = 0; i < _numPrices; i++)
    {
        time_t ts = _hourlyPrices[i].startTimestamp;

        // Check if tomorrow prices are available
        if (ts >= tomorrowStart)
            _tomorrowAvailable = true;

        // Only analyse today's slots for derived values
        if (ts < todayStart || ts >= tomorrowStart)
            continue;

        float p = _hourlyPrices[i].price_ct_per_kWh;
        sum += p;
        countToday++;
        if (p < minPrice) minPrice = p;
        if (p > maxPrice) maxPrice = p;

        // Current price = slot whose window contains 'now'
        if (ts <= now && now < ts + 3600)
            _currentPrice_ct = p;
    }

    if (countToday > 0)
    {
        _avgPriceToday_ct = sum / countToday;
        _minPriceToday_ct = minPrice;
        _maxPriceToday_ct = maxPrice;
    }

    // Determine price level based on ETS thresholds
    float cheapThresh    = (float)ParamEP_CHPriceLevelCheap    / 10.0f;  // stored as ct*10
    float expensiveThresh = (float)ParamEP_CHPriceLevelExpensive / 10.0f;
    if (_currentPrice_ct <= cheapThresh)
        _priceLevel = EP_PRICE_LEVEL_CHEAP;
    else if (_currentPrice_ct >= expensiveThresh)
        _priceLevel = EP_PRICE_LEVEL_EXPENSIVE;
    else
        _priceLevel = EP_PRICE_LEVEL_NORMAL;

    // Find cheapest N-hour window for today
    for (uint8_t i = 0; i < _numPrices; i++)
    {
        time_t ts = _hourlyPrices[i].startTimestamp;
        if (ts < todayStart || ts >= tomorrowStart)
            continue;

        // Sum of 'windowSize' consecutive hours starting at i
        if (i + windowSize > _numPrices)
            break;
        float windowSum = 0.0f;
        bool allToday = true;
        for (uint8_t j = 0; j < windowSize; j++)
        {
            time_t slotTs = _hourlyPrices[i + j].startTimestamp;
            if (slotTs >= tomorrowStart) { allToday = false; break; }
            windowSum += _hourlyPrices[i + j].price_ct_per_kWh;
        }
        if (allToday && windowSum < cheapestWindowSum)
        {
            cheapestWindowSum = windowSum;
            cheapestWindowStart = ts;
        }
    }
    _cheapestWindowStart = cheapestWindowStart;
}

void BaseEnergyPriceChannel::publishKos()
{
    if (!_available)
        return;

    // Current price (ct/kWh) - DPT 9.x (2-byte float)
    KoEP_CHCurrentPrice.value(_currentPrice_ct, DPT_Value_Temp);

    // Average / min / max price today
    KoEP_CHAvgPriceToday.value(_avgPriceToday_ct, DPT_Value_Temp);
    KoEP_CHMinPriceToday.value(_minPriceToday_ct, DPT_Value_Temp);
    KoEP_CHMaxPriceToday.value(_maxPriceToday_ct, DPT_Value_Temp);

    // Price level (0=cheap, 1=normal, 2=expensive) - DPT 5.x
    KoEP_CHPriceLevel.value(_priceLevel, DPT_SceneNumber);

    // Tomorrow prices available - DPT 1.001
    KoEP_CHTomorrowAvailable.value(_tomorrowAvailable, DPT_Switch);

    // Cheapest window start time - DPT 10.001 (time of day)
    if (_cheapestWindowStart != 0)
    {
        struct tm tmStart;
        localtime_r(&_cheapestWindowStart, &tmStart);
        // DPT 10.001: packed as (day<<5 | hour), minute, second
        uint32_t t = ((uint32_t)tmStart.tm_hour << 16) | ((uint32_t)tmStart.tm_min << 8) | tmStart.tm_sec;
        KoEP_CHCheapestWindowStart.value(t, DPT_TimeOfDay);
    }
}
