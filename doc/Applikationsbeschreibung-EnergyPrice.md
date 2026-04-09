<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2026 Steffen Rittmeier -->

# Applikationsbeschreibung Strompreis (EnergyPrice)

Das Modul stellt je Kanal aktuelle Börsenstrompreise als KNX-Gruppenobjekte bereit.  
Die Preise werden von einem konfigurierbaren Anbieter abgerufen und aufbereitet.  
Automatisierungslogik (z. B. SG-Ready, Wallbox-Steuerung) wird nicht im Modul abgebildet – dafür ist das Logikmodul zuständig.

Folgende Anbieter stehen zur Auswahl:
* [aWATTar](#awattar) – Deutschland und Österreich, kein API-Key erforderlich
* [Energy Charts](#energy-charts) – 17 europäische Handelszonen, kein API-Key erforderlich

---

## Inhaltsverzeichnis

- [Anbieter](#anbieter)
  - [aWATTar](#awattar)
  - [Energy Charts](#energy-charts)
- [ETS-Parameter](#stromanbieter)
  - [Stromanbieter](#stromanbieter)
  - [Land](#land)
  - [Handelszone](#handelszone)
  - [Automatische Aktualisierung](#automatische-aktualisierung)
  - [Günstig bis](#günstig-bis-ctkwh--10)
  - [Teuer ab](#teuer-ab-ctkwh--10)
  - [Günstigste zusammenhängende Stunden](#günstigste-zusammenhängende-stunden)
- [Gruppenobjekte](#gruppenobjekte)

---

# Anbieter

<!-- DOC HelpContext="aWATTar" -->
## aWATTar

aWATTar liefert stündliche EPEX-SPOT-Preise für Deutschland (`api.awattar.de`) und Österreich (`api.awattar.at`).  
Es wird kein API-Key benötigt.  
Die Preise werden täglich gegen 14–15 Uhr für den Folgetag veröffentlicht.

**Nutzungsbedingungen:** Ausschließlich nicht-kommerzielle Nutzung ohne API-Key.  
Weitere Informationen: https://www.awattar.de

<!-- DOCEND -->

---

<!-- DOC HelpContext="Energy-Charts" -->
## Energy Charts

Energy Charts (Fraunhofer ISE) liefert stündliche EPEX-SPOT-Preise für 17 europäische Gebotszonen.  
Es wird kein API-Key benötigt.  
Der Datenabruf erfolgt über die offene REST-API unter https://api.energy-charts.info  
Die verfügbaren Handelszonen sind: DE-LU, AT, CH, BE, FR, NL, DK1, DK2, NO1–NO5, SE1–SE4.

Weitere Informationen: https://www.energy-charts.info

<!-- DOCEND -->

---

<!-- DOC HelpContext="Stromanbieter" -->
## Stromanbieter

Wählt den Datenanbieter für diesen Kanal.

| Wert | Bedeutung |
|------|-----------|
| Deaktiviert | Kanal ist inaktiv |
| aWATTar | EPEX-SPOT-Preise via aWATTar API |
| Energy Charts | EPEX-SPOT-Preise via Fraunhofer ISE Energy Charts API |

<!-- DOCEND -->

<!-- DOC HelpContext="Land" -->
## Land

Nur relevant bei aWATTar: Wählt das Preisgebiet.

| Wert | Bedeutung |
|------|-----------|
| Deutschland | api.awattar.de |
| Österreich | api.awattar.at |

<!-- DOCEND -->

<!-- DOC HelpContext="Handelszone" -->
## Handelszone

Nur relevant bei Energy Charts: Wählt die europäische Gebotszone (Bidding Zone).

| Wert | Gebotszone |
|------|------------|
| Deutschland/Luxemburg (DE-LU) | EPEX DE-LU |
| Österreich (AT) | EPEX AT |
| Schweiz (CH) | EPEX CH |
| Belgien (BE) | EPEX BE |
| Frankreich (FR) | EPEX FR |
| Niederlande (NL) | EPEX NL |
| Dänemark West (DK1) | Nord Pool DK1 |
| Dänemark Ost (DK2) | Nord Pool DK2 |
| Norwegen 1–5 (NO1–NO5) | Nord Pool NO |
| Schweden 1–4 (SE1–SE4) | Nord Pool SE |

<!-- DOCEND -->

<!-- DOC HelpContext="Automatische-Aktualisierung" -->
## Automatische Aktualisierung

Legt fest, in welchem Intervall die Preise neu abgerufen werden.

| Wert | Intervall |
|------|-----------|
| Keine | Nur manuell über KO |
| 30 Minuten | Alle 30 Minuten |
| Jede Stunde | Stündlich |
| Täglich | Einmal täglich |

Empfohlen: **Jede Stunde** – aWATTar liefert ohnehin nur stündliche Werte.

<!-- DOCEND -->

<!-- DOC HelpContext="Guenstig-bis" -->
## Günstig bis (ct/kWh × 10)

Preisschwelle für das Preisniveau „Günstig" in ct/kWh, multipliziert mit 10.  
Beispiel: Wert `150` entspricht 15,0 ct/kWh.

<!-- DOCEND -->

<!-- DOC HelpContext="Teuer-ab" -->
## Teuer ab (ct/kWh × 10)

Preisschwelle für das Preisniveau „Teuer" in ct/kWh, multipliziert mit 10.  
Beispiel: Wert `300` entspricht 30,0 ct/kWh.

<!-- DOCEND -->

<!-- DOC HelpContext="Guenstigstes-Preisfenster" -->
## Günstigste zusammenhängende Stunden

Anzahl der aufeinanderfolgenden Stunden, für die das günstigste zusammenhängende Preisfenster berechnet wird.  
Das Ergebnis (Startuhrzeit) wird über KO „Günstigstes Preisfenster" ausgegeben.  
Bereich: 1–12.

<!-- DOCEND -->

---

# Gruppenobjekte

| Nr. | Name | DPT | Richtung | Beschreibung |
|-----|------|-----|----------|--------------|
| 0 | Aktueller Strompreis | 9.x | Ausgang | Börsenpreis der aktuellen Stunde in ct/kWh |
| 1 | Durchschnittspreis Heute | 9.x | Ausgang | Durchschnitt aller Stunden des heutigen Tages in ct/kWh |
| 2 | Mindestpreis Heute | 9.x | Ausgang | Günstigste Stunde des heutigen Tages in ct/kWh |
| 3 | Höchstpreis Heute | 9.x | Ausgang | Teuerste Stunde des heutigen Tages in ct/kWh |
| 4 | Preisniveau | 5.x | Ausgang | 0 = günstig, 1 = normal, 2 = teuer |
| 5 | Morgen verfügbar | 1.001 | Ausgang | EIN wenn Preise für morgen bereits vorliegen |
| 6 | Günstigstes Preisfenster | 10.001 | Ausgang | Startuhrzeit des günstigsten N-Stunden-Fensters |
| – | Aktualisieren | 1.017 | Eingang | Trigger: Preise sofort neu abrufen |
