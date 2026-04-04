#include "versions.h"

// additional check, because not every use the openproducer
#if defined(EP_ModuleVersion) && defined(MODULE_EnergyPriceModule_ETS)

    #define ModuleVersion_ModuleName "EnergyPriceModule"
    #define ModuleVersion_knxprod_h EP_ModuleVersion
    #define ModuleVersion_version_h MODULE_EnergyPriceModule_ETS

    #define VALUE_TO_STRING(x) #x
    #define VALUE(x) VALUE_TO_STRING(x)

    #if ModuleVersion_knxprod_h != ModuleVersion_version_h
        #pragma message "\n\n\nYou need to >>> INCREASE YOUR <<< ETS ApplicationVersion and manually synchronize op:verify of " ModuleVersion_ModuleName " to " VALUE(ModuleVersion_version_h) "\n\n(see https://github.com/OpenKNX/OpenKNX/wiki/Versionierung-von-Modulen-(OFM)#fehler-vom-compiler )\n\n\n"
        #pragma GCC error "\n\nETS Application Version problem (see next message)\n\n"
    #endif

#endif
