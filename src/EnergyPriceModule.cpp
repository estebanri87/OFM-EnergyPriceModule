#include "EnergyPriceModule.h"
#include "AwattarChannel.h"

OpenKNX::Channel* EnergyPriceModule::createChannel(uint8_t index)
{
    // ParamEP_CHProviderType: 0=aWATTar
    switch (ParamEP_CHProviderType)
    {
        case 0:
        default:
            return new AwattarChannel(index);
    }
}

EnergyPriceModule openknxEnergyPriceModule;
