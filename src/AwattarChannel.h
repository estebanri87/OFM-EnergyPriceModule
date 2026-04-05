#pragma once
#include "BaseEnergyPriceChannel.h"

class AwattarChannel : public BaseEnergyPriceChannel
{
  public:
    AwattarChannel(uint8_t index) : BaseEnergyPriceChannel(index) {}
    const std::string name() override { return "AwattarChannel"; }

  protected:
    int16_t fillPrices(EnergyPriceHourlyData* prices, uint8_t maxCount) override;
};
