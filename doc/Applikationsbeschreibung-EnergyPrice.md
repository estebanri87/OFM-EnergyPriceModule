<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2026 Steffen Rittmeier -->

# Applikationsbeschreibung Strompreis (EnergyPrice)

Das Modul stellt je Kanal aktuelle Börsenstrompreise als KNX-Gruppenobjekte bereit.  
Die Preise werden von einem konfigurierbaren Anbieter abgerufen und aufbereitet.  
Automatisierungslogik (z. B. SG-Ready, Wallbox-Steuerung) wird nicht im Modul abgebildet – dafür ist das Logikmodul zuständig.

Folgende Anbieter stehen zur Auswahl:
* [aWATTar](#awattar) – Deutschland und Österreich, kein API-Key erforderlich

---

# Anbieter

<!-- DOC -->
## aWATTar

aWATTar liefert stündliche EPEX-SPOT-Preise für Deutschland (`api.awattar.de`) und Österreich (`api.awattar.at`).  
Es wird kein API-Key benötigt.  
Die Preise werden täglich gegen 14–15 Uhr für den Folgetag veröffentlicht.

**Nutzungsbedingungen:** Ausschließlich nicht-kommerzielle Nutzung ohne API-Key.  
Weitere Informationen: https://www.awattar.de

<!-- DOCEND -->

---

# Kanaleinstellungen

<!-- DOC -->
## Stromanbieter

Wählt den Datenanbieter für diesen Kanal.

| Wert | Bedeutung |
|------|-----------|
| Deaktiviert | Kanal ist inaktiv |
| aWATTar | EPEX-SPOT-Preise via aWATTar API |

<!-- DOCEND -->

<!-- DOC -->
## Land

Nur relevant bei aWATTar: Wählt das Preisgebiet.

| Wert | Bedeutung |
|------|-----------|
| Deutschland | api.awattar.de |
| Österreich | api.awattar.at |

<!-- DOCEND -->

<!-- DOC -->
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

<!-- DOC -->
## Günstig bis (ct/kWh × 10)

Preisschwelle für das Preisniveau „Günstig" in ct/kWh, multipliziert mit 10.  
Beispiel: Wert `150` entspricht 15,0 ct/kWh.

<!-- DOCEND -->

<!-- DOC -->
## Teuer ab (ct/kWh × 10)

Preisschwelle für das Preisniveau „Teuer" in ct/kWh, multipliziert mit 10.  
Beispiel: Wert `300` entspricht 30,0 ct/kWh.

<!-- DOCEND -->

<!-- DOC -->
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
