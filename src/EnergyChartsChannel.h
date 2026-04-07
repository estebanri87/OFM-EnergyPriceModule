#pragma once
#include "BaseEnergyPriceChannel.h"

class EnergyChartsChannel : public BaseEnergyPriceChannel
{
  public:
    EnergyChartsChannel(uint8_t index) : BaseEnergyPriceChannel(index) {}
    const std::string name() override { return "EnergyCharts"; }

  protected:
    int16_t fillPrices(EnergyPriceHourlyData* prices, uint8_t maxCount) override;
};
