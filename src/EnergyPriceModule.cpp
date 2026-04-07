#include "EnergyPriceModule.h"
#include "AwattarChannel.h"
#include "EnergyChartsChannel.h"

OpenKNX::Channel* EnergyPriceModule::createChannel(uint8_t _channelIndex /* used in param macros, do not rename */)
{
    // ParamEP_CHProviderType: 0=Deaktiviert, 1=aWATTar, 2=Energy Charts
    switch (ParamEP_CHProviderType)
    {
        case 1:
            return new AwattarChannel(_channelIndex);
        case 2:
            return new EnergyChartsChannel(_channelIndex);
        default:
            return nullptr;
    }
}

EnergyPriceModule openknxEnergyPriceModule;
