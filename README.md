# T-Deck RS41 – Wettersonden-Empfänger & LoRa-APRS-Tracker

Firmware für den [LilyGO T-Deck Plus](https://github.com/Xinyuan-LilyGO/T-Deck) (ESP32-S3, SX1262-Funkmodul),
die das Gerät in zwei Betriebsarten verwandelt:

- **Sondenempfang**: empfängt, dekodiert und decodiert Vaisala-RS41-Wettersonden (433 MHz, FSK) direkt über
  den Lautsprecher-losen SX1262 – Position, Höhe, Steig-/Sinkrate und ID live auf der Karte, inklusive
  Landevorhersage beim Sinken.
- **LoRa-APRS**: eigenständiger APRS-Tracker/Empfänger auf 433,775 MHz (europäische LoRa-APRS-Konvention),
  zeigt gehörte Stationen auf der Karte, sendet die eigene Position als Beacon (SmartBeaconing-artig nach
  Tempo getaktet) – ganz ohne Internetverbindung, rein über Funk.

Kein zusätzliches Board, kein Zusatzmodul: läuft komplett auf der im T-Deck Plus verbauten Hardware
(Display, Tastatur, Trackball, GPS, SD-Karte, Akku).

## Features

- RS41-Dekodierung inkl. CRC-Prüfung, GPS-Zeit/-Position/-Höhe, Steig-/Sinkrate, Landevorhersage (lokal und
  optional online per Tawhiri/SondeHub-Windvorhersage)
- Kartenansicht mit mehreren Kartenarten (Rad-/Luftbild-/Topo-/Relief-Kacheln), eigenem Track, Zoom und
  Fokus-Automatik (Sonde+Ich+Landung / Sonde+Ich / Sonde / Ich)
- GPS-Radar-Ansicht: Polarplot der einzelnen Satelliten (Azimut/Elevation) zur Empfangsdiagnose
- Höhentrack-Diagramm (Sondenmodus) bzw. Stationsliste mit Entfernung/Alter (LoRa-Modus), inkl. Einzelstations-
  Fokus auf der Karte
- LoRa-APRS: Empfang (inkl. komprimiertes Positionsformat), eigener Beacon mit automatischem
  Symbol-Wechsel nach Tempo (Stationär/Fußgänger/Fahrrad/Bus), Digipeating
- Sondenfund-Meldung per SondeHub-API (inkl. Freitext-Fundbericht), automatische Weiterleitung zu
  radiosondy.info
- GPX-Aufzeichnung (eigene Position + Sonde), Screenshots mit EXIF-Koordinaten, SD-Kartenlog mit
  GPS-/Akku-Diagnosedaten
- Weboberfläche: Live-Bild des Displays, vollständige Fernsteuerung aller Menüpunkte, Screenshot-/Log-Download
- Firmware-Update per WLAN (ArduinoOTA) ohne USB-Kabel, sobald einmal per USB geflasht

## Hardware

- [LilyGO T-Deck Plus](https://github.com/Xinyuan-LilyGO/T-Deck) (ESP32-S3, SX1262, GPS, Akku, Tastatur, Trackball)
- MicroSD-Karte (Screenshots, GPX, Logs)

## Flashen

1. `config.h.example` im Sketch-Ordner nach `config.h` kopieren und dort eigenes Rufzeichen sowie
   eigene Heimat-Koordinaten eintragen (`config.h` ist persönlich und per `.gitignore` von GitHub
   ausgenommen).
2. Einmalig per USB mit der Arduino-IDE flashen. Board- und Werkzeuge-Einstellungen (Arduino IDE 2.3.10):

   | Einstellung | Wert |
   |---|---|
   | Board | ESP32S3 Dev Module |
   | USB CDC On Boot | Enabled *(sonst kein Serial-Log über USB-C)* |
   | CPU Frequency | 240MHz (WiFi) |
   | Core Debug Level | None |
   | USB DFU On Boot | Disabled |
   | Erase All Flash Before Sketch Upload | Disabled |
   | Events Run On | Core 1 |
   | Flash Mode | QIO 80MHz |
   | Flash Size | 16MB (128Mb) |
   | JTAG Adapter | Disabled |
   | Arduino Runs On | Core 1 |
   | USB Firmware MSC On Boot | Disabled |
   | Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
   | PSRAM | OPI PSRAM |
   | Upload Mode | UART0 / Hardware CDC |
   | Upload Speed | 921600 |
   | USB Mode | Hardware CDC and JTAG |
   | Zigbee Mode | Disabled |
3. Danach: WLAN im Menü (Taste `W`) mit dem Heimnetz verbinden (Menü `E` für SSID/Passwort) – das Gerät
   taucht dann unter Werkzeuge → Port als Netzwerkport auf (ArduinoOTA, kein Passwort nötig).

## Bedienung (Kurzreferenz)

- **Space / Trackball-Klick**: Ansicht wechseln (Text → Karte → Höhentrack/Stationen → GPS-Radar)
- **Enter**: Konfigurationsmenü
- **A/D** bzw. **O/P**: Kartenzoom (links-/rechtshändig)
- **R** (in der Karte): automatischen Fokus weiterschalten; **R** im Stationsinfo-Popup der Stationsliste:
  Karte mit Fokus auf genau diese Station öffnen
- **Q**: Sondenfund melden / geordnetes Herunterfahren (je nach Kontext)
- **C**: Screenshot

Vollständige Tastenbelegung und Menüpunkte: Kopfkommentar in `tdeck_rs41_v2.ino`.

## Screenshots

### Sondenmodus

<img src="Bilder/Sondenmodus/shot_20261016_151218_karte.jpg" width="32%"> <img src="Bilder/Sondenmodus/shot_20261015_130831_karte.jpg" width="32%"> <img src="Bilder/Sondenmodus/shot_20261016_151223_hoehentrack.jpg" width="32%">

Kartenansicht mit Flugbahn und Landevorhersage (links/Mitte) und Höhentrack-Diagramm des Aufstiegs (rechts).

<details>
<summary>Weitere Sondenmodus-Screenshots</summary>

<img src="Bilder/Sondenmodus/shot_20261015_133858_karte.jpg" width="32%"> <img src="Bilder/Sondenmodus/shot_20261016_135007_karte.jpg" width="32%"> <img src="Bilder/Sondenmodus/shot_20261016_140233_karte.jpg" width="32%">
<img src="Bilder/Sondenmodus/shot_20261016_141241_karte.jpg" width="32%"> <img src="Bilder/Sondenmodus/shot_20261016_145113_karte.jpg" width="32%">

</details>

### LoRa-APRS-Modus

<img src="Bilder/LoRa%20Modus/shot_20261016_201429_karte.jpg" width="32%"> <img src="Bilder/LoRa%20Modus/shot_20261016_201455_karte.jpg" width="32%"> <img src="Bilder/LoRa%20Modus/shot_20261016_201357_hoehentrack.jpg" width="32%">

Gehörte Stationen auf der Karte (Übersicht und Einzelstations-Fokus) sowie die Stationsliste mit Entfernung
und Alter.

<details>
<summary>Weitere LoRa-APRS-Screenshots</summary>

<img src="Bilder/LoRa%20Modus/shot_20261016_201445_hoehentrack.jpg" width="32%"> <img src="Bilder/LoRa%20Modus/shot_20261016_151230_gps-radar.jpg" width="32%"> <img src="Bilder/LoRa%20Modus/shot_20261016_151259_text.jpg" width="32%">

Stations-Detailpopup, GPS-Radar (Empfangsdiagnose der einzelnen Satelliten) und die Textansicht (hier noch im
Sondenmodus-Frequenzbereich, Anzeige ist in beiden Modi gleich aufgebaut).

</details>

## Status

Aktive Hobby-Entwicklung, läuft im Feldeinsatz (Fuß/Fahrrad/Auto, Sondenjagd). Siehe Versionskommentare im
Quelltext für die laufende Änderungshistorie.

## Lizenz

GNU GPLv3, siehe [LICENSE](LICENSE).

## Autor

DL6TJ
