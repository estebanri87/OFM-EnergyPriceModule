# OFM-EnergyPriceModule

OpenKNX Modul zur Abfrage aktueller Strompreise von Online-Diensten (Day-Ahead-Marktpreise).

Das Modul stellt stündliche Börsenstrompreise über Gruppenobjekte im KNX-Bus bereit und ermöglicht so dynamische Laststeuerung basierend auf dem aktuellen Preisniveau (z. B. für Wärmepumpen, Elektroauto-Ladung, Warmwasserbereitung).

## Features

- Unterstützung mehrerer Anbieter je Kanal:
  - **aWATTar** – Day-Ahead-Preise für Deutschland und Österreich (kein API-Schlüssel erforderlich)
  - **Energy Charts** – Day-Ahead-Preise für zahlreiche europäische Handelszonen (kein API-Schlüssel erforderlich)
- Gruppenobjekte je Kanal:
  - Aktueller Strompreis (ct/kWh, DPT 9.x)
  - Durchschnittspreis heute (ct/kWh, DPT 9.x)
  - Mindestpreis heute (ct/kWh, DPT 9.x)
  - Höchstpreis heute (ct/kWh, DPT 9.x)
  - Preisniveau (0 = günstig, 1 = normal, 2 = teuer, DPT 5.010)
  - Morgen-Preise verfügbar (DPT 1.001)
  - Beginn des günstigsten zusammenhängenden Preisfensters (DPT 10.001)
- Konfigurierbare Preisgrenzen für „günstig" und „teuer"
- Konfigurierbare Länge des günstigsten Fensters (1–12 Stunden)
- Automatische Aktualisierung (30 Minuten / stündlich / täglich)
- Manuelle Aktualisierung per KNX-Gruppenobjekt
- Mehrere unabhängige Kanäle möglich

## Abhängigkeiten

Das Modul setzt [OFM-Network](https://github.com/OpenKNX/OFM-Network) oder [OFM-WLANModule](https://github.com/OpenKNX/OFM-WLANModule) für die Internetverbindung voraus.

## Konfiguration (ETS)

| Parameter | Beschreibung |
|---|---|
| Strompreisanbieter | aWATTar oder Energy Charts |
| Land / Handelszone | Preisregion (abhängig vom Anbieter) |
| Automatische Aktualisierung | Keine / 30 Min / Stündlich / Täglich |
| Günstig bis | Preisschwelle in ct/kWh × 0,1 für Stufe „günstig" |
| Teuer ab | Preisschwelle in ct/kWh × 0,1 für Stufe „teuer" |
| Günstigste zusammenhängende Stunden | Länge des gesuchten günstigen Fensters (1–12 h) |

## Hardware Unterstützung

| Prozessor | Status | Anmerkung |
|-----------|--------|-----------|
| RP2040    | Beta   |           |
| ESP32     | Beta   |           |

Getestete Hardware:
- [OpenKNX REG1 Basismodul LAN+TP](http://device.openknx.de/REG1-LAN-TP-Base)

## Einbindung in die Anwendung

In das Anwendungs-XML muss OFM-EnergyPriceModule aufgenommen werden:

```xml
<op:define prefix="EP" ModuleType="30"
  share=   "../lib/OFM-EnergyPriceModule/src/EnergyPriceModule.share.xml"
  template="../lib/OFM-EnergyPriceModule/src/EnergyPriceModule.templ.xml"
  NumChannels="5"
  KoOffset="800">
  <op:verify File="../lib/OFM-EnergyPriceModule/library.json" ModuleVersion="0.1" />
</op:define>
```

**Hinweis:** Pro Kanal werden 7 KO's benötigt. Dies muss bei nachfolgenden Modulen bei `KoOffset` entsprechend berücksichtigt werden.

In `main.cpp` muss das EnergyPriceModule hinzugefügt werden:

```cpp
#include "EnergyPriceModule.h"
// ...

void setup()
{
    // ...
    openknx.addModule(4, openknxEnergyPriceModule);
    // ...
}
```

## Lizenz

[GNU GPL v3](LICENSE)
