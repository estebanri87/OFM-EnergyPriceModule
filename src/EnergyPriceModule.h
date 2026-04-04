#pragma once
#include "ChannelOwnerModule.h"
#include "ModuleVersionCheck.h"

class EnergyPriceModule : public EPChannelOwnerModule
{
  public:
    const char* name() override { return "EnergyPriceModule"; }
    const char* version() override { return EP_ModuleVersion; }

  protected:
    OpenKNX::Channel* createChannel(uint8_t index) override;
};

extern EnergyPriceModule openknxEnergyPriceModule;
