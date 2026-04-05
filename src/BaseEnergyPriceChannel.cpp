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
            now >= 60000 &&  // wait 60s after boot for NTP sync
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
            if (time(nullptr) >= 1577836800LL)  // only fetch after NTP is synced
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
    openknx.watchdog.loop();  // pet before blocking HTTP call (16s watchdog)
    logInfoP("Fetching energy price data (channel %d)", _channelIndex);
    int16_t count = fillPrices(_hourlyPrices, EP_MAX_HOURLY_PRICES);
    openknx.watchdog.loop();  // pet after HTTP call returned
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

    // Sanity check: require NTP time to be synced (any time after 2020-01-01)
    time_t now = time(nullptr);
    if (now < 1577836800LL)  // 2020-01-01 00:00 UTC
    {
        logWarningP("System time not synced (time=%ld), skipping price calculation", (long)now);
        return;
    }
    // Reset all derived values before recalculating to avoid stale data
    _currentPrice_ct  = 0.0f;
    _avgPriceToday_ct = 0.0f;
    _minPriceToday_ct = 0.0f;
    _maxPriceToday_ct = 0.0f;
    _tomorrowAvailable = false;
    _cheapestWindowStart = 0;

    // Determine today's date boundaries (midnight local time)
    // Use mktime(mday+1) instead of +86400 to handle DST transitions correctly.
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    tmNow.tm_hour = 0;
    tmNow.tm_min  = 0;
    tmNow.tm_sec  = 0;
    time_t todayStart = mktime(&tmNow);
    tmNow.tm_mday++;
    time_t tomorrowStart = mktime(&tmNow);

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

    // Find cheapest N-hour window for today.
    // Each candidate window must consist of exactly 'windowSize' consecutive
    // slots that are 3600s apart and all fall within today.
    for (uint8_t i = 0; i < _numPrices; i++)
    {
        time_t ts = _hourlyPrices[i].startTimestamp;
        if (ts < todayStart || ts >= tomorrowStart)
            continue;
        if (i + windowSize > _numPrices)
            break;

        float  windowSum  = 0.0f;
        bool   valid      = true;
        for (uint8_t j = 0; j < windowSize; j++)
        {
            const EnergyPriceHourlyData& slot = _hourlyPrices[i + j];
            // Reject window if slot is outside today or not exactly 1h after predecessor
            if (slot.startTimestamp >= tomorrowStart)
                { valid = false; break; }
            if (j > 0 && slot.startTimestamp != _hourlyPrices[i + j - 1].startTimestamp + 3600)
                { valid = false; break; }
            windowSum += slot.price_ct_per_kWh;
        }
        if (valid && windowSum < cheapestWindowSum)
        {
            cheapestWindowSum   = windowSum;
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

    // Price level (0=cheap, 1=normal, 2=expensive) - DPT 5.010 (1-byte unsigned count)
    KoEP_CHPriceLevel.value(_priceLevel, DPT_Value_1_Ucount);

    // Tomorrow prices available - DPT 1.001
    KoEP_CHTomorrowAvailable.value(_tomorrowAvailable, DPT_Switch);

    // Cheapest window start time - DPT 10.001 (time of day)
    if (_cheapestWindowStart != 0)
    {
        struct tm tmStart;
        localtime_r(&_cheapestWindowStart, &tmStart);
        KoEP_CHCheapestWindowStart.value(tmStart, DPT_TimeOfDay);
    }
}
