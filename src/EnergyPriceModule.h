#pragma once
#include "ChannelOwnerModule.h"
#include "ModuleVersionCheck.h"

class EnergyPriceModule : public EPChannelOwnerModule
{
  public:
    EnergyPriceModule() : EPChannelOwnerModule(EP_ChannelCount) {}
    const std::string name() override { return "EnergyPriceModule"; }
    const std::string version() override { return std::to_string(EP_ModuleVersion); }

  protected:
    OpenKNX::Channel* createChannel(uint8_t _channelIndex /* used in param macros, do not rename */) override;
};

extern EnergyPriceModule openknxEnergyPriceModule;
