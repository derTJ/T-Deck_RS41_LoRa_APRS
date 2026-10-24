// settings.h -- allgemeine einstellbare Werte fuer tdeck_rs41_v2.ino
//
// Hier stehen (fast) alle Timings, Schwellwerte, Popup-/Menue-Anzeigedauern, Farben, Puffer-/Listengroessen
// und WLAN-Zugangsdaten gesammelt an einem Ort, statt verstreut im ganzen Sketch. Zukuenftige Erweiterungen
// bauen auf diesen Werten auf bzw. ergaenzen hier neue Konstanten, statt einzelne Zahlen mitten im Code zu
// aendern. Im Unterschied zu config.h (persoenliches Rufzeichen + Heimat-Koordinaten, siehe dort) ist diese
// Datei NICHT in .gitignore -- sie wird normal mit committet, denn sie enthaelt keine persoenlichen Geheimnisse
// (die Standard-WLAN-Zugangsdaten unten sind ohnehin Platzhalter, die man vor dem ersten eigenen Einsatz aendern
// sollte, siehe Kommentar dort). Pins, LoRa/FSK-Funkprotokoll-Konstanten (SF/CR/Syncword/Preamble), das RS41-
// Frameformat (Frame-/Blocklaengen, Sync-Bytes) und fest zusammengehoerende Code-Tabellen (z. B. die Menue-
// Tastenkuerzel) bleiben bewusst im .ino, weil sie keine "Einstellung" sind, sondern Protokoll/Struktur.
//
// Muss im selben Ordner liegen wie tdeck_rs41_v2.ino (Arduino zeigt sie dann als eigenen Tab in der IDE).
//
// Feature v4.95 (Wunsch Okt. 2026, "settings auch als ini ins dateisystem, updatebar per upload"): ein Teil
// dieser Werte ("weiche" Werte -- Timings, Schwellwerte, Farben, Transparenz, ein/aus-aehnliche Zahlen) steht
// hier bewusst NICHT mehr als #define (reine Textersetzung beim Compilieren, danach nicht mehr aenderbar),
// sondern als "static TYPE NAME = Compile-Default;" -- eine ganz normale, beschreibbare Variable mit demselben
// Namen wie vorher, damit an den ueber 9000 Zeilen Verwendungsstellen im .ino NICHTS geaendert werden musste.
// Diese Variablen sind unten mit "-- CFG: per config.ini aenderbar, sofort wirksam" markiert; siehe
// CFG_REGISTRY[]/applyConfigLine()/webCfgPage() weiter unten im .ino fuer den Mechanismus. Pins, Puffer-/
// Array-Groessen, Code-Tabellen und alles, was den RS41-Sondensuchlauf als zusammenhaengendes Ganzes bildet
// (siehe Abschnitt "Scan-Parameter" unten), bleiben bewusst echte #define-Konstanten -- das waren sie auch
// vorher schon konzeptionell nicht "nur eine Einstellung", sondern Programmstruktur (siehe Erklaerung oben).

#ifndef TDECK_SETTINGS_H
#define TDECK_SETTINGS_H

// ========================================================================================================
// WLAN / Netzwerk
// ========================================================================================================
#define WIFI_SSID      "T-Deck-RS41"           // Name des eigenen Zugangspunkts (Menue W, Modus "eigenes Netz")
#define WIFI_PASS      "sonde433"              // mindestens 8 Zeichen -- bitte vor dem ersten Einsatz aendern
#define WIFI_HOST      "tdeck-rs41"            // Rechnername im Heimnetz: http://tdeck-rs41.local
static uint32_t WIFI_LOST_MS = 10000;                   // -- CFG: so lange darf die Heimnetz-Verbindung weg sein (z. B.
                                                // kurzer Aussetzer), bevor das WLAN als "Netz verlassen" gilt und
                                                // abgeschaltet wird
static uint32_t WIFI_IDLE_OFF_MS = (5UL * 60UL * 1000UL);  // -- CFG: eigener Zugangspunkt, MANUELL im Menue W gestartet:
                                                // geht nach so langer Zeit ohne verbundenes Geraet von selbst wieder
                                                // aus (spart Akku). Gilt nicht fuer das Heimnetz -- dort haengt das
                                                // T-Deck ohnehin am Strom, WLAN bleibt an, solange die Verbindung
                                                // steht (siehe WIFI_LOST_MS). Fuer den AUTOMATISCH beim Einschalten
                                                // geoeffneten Zugangspunkt siehe stattdessen WIFI_BOOT_AP_MS.
static uint32_t WIFI_BOOT_HOME_TIMEOUT_MS = (20UL * 1000UL);   // -- CFG: Boot-WLAN-Sequenz (Feature v4.88, siehe
                                                // wifiBootSeq/setup()): so lange wird beim Einschalten versucht,
                                                // das gespeicherte Heimnetz zu erreichen, bevor auf die
                                                // Boot-Hotspot-Phase umgeschaltet wird (siehe WIFI_BOOT_AP_MS)
static uint32_t WIFI_BOOT_AP_MS = (5UL * 60UL * 1000UL);        // -- CFG: Boot-WLAN-Sequenz: so lange bleibt der
                                                // automatisch (ohne erreichbares Heimnetz bzw. nach dessen
                                                // Fehlschlag) geoeffnete eigene Zugangspunkt beim Einschalten an --
                                                // danach geht das WLAN GANZ aus (anders als WIFI_IDLE_OFF_MS: ein
                                                // FESTES Zeitfenster, unabhaengig davon, ob gerade ein Geraet
                                                // verbunden ist), um den Akku zu schonen, wenn niemand zugreift
#define NTP_SERVER     "pool.ntp.org"   // fuer die Uhrzeit beim Einschalten, solange noch kein GPS-Fix vorliegt
static uint32_t NTP_TIMEOUT_MS = 8000;    // -- CFG: so lange wird nach dem Verbinden auf eine gueltige NTP-Antwort gewartet, dann aufgegeben
#define TZ_RULE        "CET-1CEST,M3.5.0,M10.5.0/3"   // Ortszeit fuer Dateizeiten und Logzeilen (Mitteleuropa mit Sommerzeit)

// ========================================================================================================
// Verzeichnisse / Dateien / Logging
// ========================================================================================================
#define SHOT_DIR       "/shots"                // Screenshots (Taste C)
#define GPX_DIR        "/gpx"                  // GPX-Tracks (eigene Position + Sonde)
#define LOG_DIR        "/log"                  // Text-Logs (an/aus im Menue P; je Neustart ein frisches Log)
#define TITLE_TEXT     "RS41 | Enter=Menue  Space=Ansicht"   // Kopfzeile der Textansicht

#define LOG_LEVEL      0       // 0 = eine Zeile pro Frame, 1 = zusaetzlich alle Blockdetails (siehe VLOG()/VLOGLN() im .ino)
#define LOG_FALLBACK_MAX 99    // Platzhalter-Lognummer (01..LOG_FALLBACK_MAX), solange noch keine Uhrzeit (GPS/NTP) vorliegt
#define SD_LOG         1       // 1 = Zeile pro Frame zusaetzlich auf die SD-Karte schreiben (/log/rs41_log_*.txt), 0 = aus
                                // (bewusst NICHT per config.ini: ist bereits ueber Menue P + Preferences/NVS zur
                                // Laufzeit umschaltbar und persistent, siehe sdLogOn/loadSettings()/saveSettings()
                                // im .ino -- hier nur noch der Start-/Fallback-Wert)
#define VERBOSE_DUMP   0       // 1 = Rohdaten/entwuerfelte Frames als Hex seriell ausgeben (sehr viel Text), 0 = aus

// ========================================================================================================
// Anzeige / Farben
// ========================================================================================================
// Das Display versteht intern nur RGB565 (16 Bit), aber die uebliche, fuer Menschen gewohnte Schreibweise
// ist RGB888 (0xRRGGBB, je 1 Byte pro Farbkanal, wie in jedem Grafikprogramm/Colorpicker). Darum hier NICHT
// mehr die alten 16-Bit-Werte direkt hinschreiben, sondern das Makro RGB(0xRRGGBB) -- rechnet beim
// Compilieren (kostet zur Laufzeit nichts) auf den tatsaechlich benoetigten RGB565-Wert um. Dieselbe
// Umrechnung passiert bei den "-- CFG:"-Farben auch zur Laufzeit (rgb24To565()/rgb565To24() im .ino), damit
// /config.ini bzw. das Webformular ebenfalls ganz normales 0xRRGGBB zeigen/annehmen (Wunsch Okt. 2026,
// "ungebraeuchliches Format, bitte 0xrrggbb nutzen") -- Achtung: RGB565 kennt nur 5/6/5 statt 8/8/8 Bit je
// Kanal, ein eingegebener Wert wird also leicht abgerundet (unteren Bits gehen verloren, sichtbar meist
// nicht). Die Werte unten sind genau so gewaehlt, dass sie exakt dieselbe Darstellung ergeben wie vorher.
#define RGB(hex24) (uint16_t)( (((uint32_t)(hex24) & 0xF80000UL) >> 8) | \
                                (((uint32_t)(hex24) & 0x00FC00UL) >> 5) | \
                                (((uint32_t)(hex24) & 0x0000F8UL) >> 3) )

#define TFT_INVERT     true    // Displayfarben invertieren -- vom konkreten Panel abhaengig, true fuer das T-Deck Plus
                                // (bewusst fest: haengt am verbauten Panel, nicht am Geschmack/Einsatzzweck)

// Kartenmarker/Sondenpunkt:
static uint16_t COL_SONDE_OK      = RGB(0x00FF00);   // -- CFG: Sonde: gueltige Position vor < 4 s (Fuellfarbe,
                                           // s. lastPosOkMs) -- Gruen
static uint16_t COL_SONDE_LATE    = RGB(0xFFFF00);   // -- CFG: Sonde: 4..19 s keine gueltige Position -- Gelb
static uint16_t COL_SONDE_LOST    = RGB(0xFF0000);   // -- CFG: Sonde: 20 s oder laenger keine gueltige Position -- Rot
static uint16_t COL_SONDE_RING    = RGB(0x000000);   // -- CFG: Ring um den Sondenpunkt (normal) -- Schwarz
static uint16_t COL_SONDE_RING_SIG = RGB(0x00FF00);  // -- CFG: Ring kurz gruen: Schritt 1 (Header) gerade empfangen,
                                           // aber (noch) keine gueltige Position in diesem Takt
static uint32_t SONDE_SIGNAL_FLASH_MS = 1000UL;      // -- CFG: so lange blinkt der Ring nach einem blossen Signal-Empfang (ohne Position)

// Eigene Position/Spur:
static uint16_t COL_ICH_FILL      = RGB(0x00FCF8);   // -- CFG: ich: Fuellung cyan -- gut sichtbar auf allen Kartenarten
static uint16_t COL_ICH_RING      = RGB(0x000000);   // -- CFG: ich: Ring -- Schwarz
static uint16_t COL_LINE          = RGB(0xFF0000);   // -- CFG: Linie zwischen mir und der Sonde (2 Pixel breit) -- Rot
static uint16_t COL_OWN_TRAIL     = RGB(0xF800F8);   // -- CFG: eigener Track -- Magenta

// Sondenspur/Landevorhersage:
static uint16_t COL_TRAIL         = RGB(0xF88C00);   // -- CFG: Spur der Sonde -- Orange
static uint16_t COL_PRED          = RGB(0x0080F8);   // -- CFG: Landevorhersage (eigene Berechnung): Verlaengerung der
                                           // Spur + Fadenkreuz -- Blau
static uint16_t COL_PRED_ONLINE   = RGB(0xFFFF00);   // -- CFG: Online-Landevorhersage (Tawhiri/SondeHub-API): nur
                                           // Fadenkreuz -- Gelb
static int CROSS_HALF             = 6;               // -- CFG: Fadenkreuz Landepunkt: halbe Linienlaenge in Pixeln

// GPS-Radar:
static uint16_t COL_RADAR_RING    = RGB(0x008000);   // -- CFG: Radar: Ringe und Kreuz -- dunkelgruen
static uint16_t COL_RADAR_TEXT    = RGB(0x808080);   // -- CFG: Radar: Beschriftungen -- grau

// Hoehentrack:
static uint16_t COL_ALT_FILL      = RGB(0x105C58);   // -- CFG: Flaechenfuellung unter der Kurve -- dunkles Petrol
                                           // (dieselbe Farbe wie im Chart der Weboberflaeche, siehe loadAlt() im .ino)

// LoRa-APRS-Stationspunkte, Farbe je nach Empfangsart (siehe aprsRecvKind[] im .ino):
static uint16_t COL_APRS_DIRECT   = RGB(0x0000F8);   // -- CFG: direkt per Funk gehoert -- Blau
static uint16_t COL_APRS_IGATE    = RGB(0xFF0000);   // -- CFG: als Fremdverkehr ueber ein iGate/APRS-IS relayt -- Rot
static uint16_t COL_APRS_DIGI     = RGB(0xF88C00);   // -- CFG: ueber einen genutzten Digipeater-Hop weitergereicht --
                                           // Orange (bewusst = COL_TRAIL als Compile-Default)

// Begruessungsbildschirm (bootSplash() im .ino, siehe auch BOOT_SPLASH_MS oben):
static uint16_t COL_BOOT_CALLSIGN = RGB(0xFF0000);   // -- CFG: Feature v4.93: eigenes Rufzeichen gross oben (vorher
                                           // fest Cyan direkt im Funktionsaufruf, jetzt hier einstellbar) -- Rot

// Statusblock-Untergrund (Batterie/GPS/Uhrzeit, drawBattery()/drawGps()/drawClock() im .ino, siehe statusBgFill()):
static int STATUS_BG_TRANSPARENCY_PCT = 75;   // -- CFG: Feature v4.94 (Wunsch Okt. 2026): wie durchsichtig der
                                           // schwarze Untergrund hinter Batterie/GPS/Uhrzeit ist. 0 = blickdicht
                                           // schwarz (wie vor diesem Feature), 100 = gar kein Untergrund mehr (nur
                                           // noch Batterie/GPS/Uhrzeit selbst, Kartenbild komplett durchscheinend).
                                           // Wirkt nur in der Kartenansicht (dort wird direkt in den Bildpuffer
                                           // gemischt); in der Textansicht/im Menue bleibt es ohnehin blickdicht
                                           // schwarz, da der Untergrund dort schon schwarz ist (sieht man nicht).

// ========================================================================================================
// Toast-/Popup-/Menue-Meldungen -- wie lange eine kurze Rueckmeldung am Geraet sichtbar bleibt
// ========================================================================================================
static uint32_t BOOT_SPLASH_MS      = 2500;   // -- CFG: Begruessungsbildschirm beim Einschalten
static uint32_t TOAST_MS_DEFAULT    = 1800;   // -- CFG: normale kurze Rueckmeldung unten/oben im Bild (z. B. "Ton: an")
static uint32_t TOAST_MS_LONG       = 5000;   // -- CFG: laenger lesbare Rueckmeldung, z. B. Trackballklick "Position
                                    // senden" -- sonst waehrend der Fahrt/des Laufens kaum zu lesen (siehe
                                    // toastFor() im .ino)
static uint32_t MENU_NOTE_MS        = 2500;   // -- CFG: Hinweiszeile im Einstellungsmenue (z. B. "Foto: ..." nach
                                    // einem Screenshot)

// ========================================================================================================
// Akku (Spannungsteiler-Kalibrierung, Menue-Statuszeile)
// ========================================================================================================
static float BAT_SCALE  = 2.0f;    // -- CFG: Teilerfaktor des Spannungsteilers
static float BAT_OFFSET = 0.0f;    // -- CFG: Korrektur in Volt (mit Multimeter abgleichen; gemessene Spannung steht in der Logzeile)
static float BAT_USB_V  = 4.30f;   // -- CFG: ab dieser gemessenen Spannung gilt USB/Ladespannung als angesteckt

// ========================================================================================================
// Tastatur-Beleuchtung / Trackball
// ========================================================================================================
static int KB_LEVEL       = 120;     // -- CFG: Helligkeit des Tastenlichts (0..255)
static int KB_LEVEL_DIM   = 25;      // -- CFG: Modus 2 "Taste+dunkel": gedimmte Grundhelligkeit, solange keine Taste gedrueckt wird
static uint32_t KB_ON_MS       = 6000;    // -- CFG: so lange leuchten die Tasten nach dem letzten Tastendruck
static uint32_t KB_BRIGHT_MS   = 4000;    // -- CFG: Modus 2: so lange bleibt es nach einem Tastendruck hell, bevor es wieder dimmt
static double KB_DARK_ELEV = (-1.0);  // -- CFG: "dunkel" = Sonne tiefer als dieser Winkel unter dem Horizont [Grad]
#define TB_PULSES      2       // so viele Trackball-Impulse gelten als ein Schritt (bei Bedarf anpassen)
                                // (bewusst fest: Hardware-Kalibrierung des konkreten Trackball-Encoders)

// ========================================================================================================
// RS41-Sondensuche: Scan-Parameter (zweistufig -- erst grob mit breitem Filter, dann fein um die Kandidaten)
// ========================================================================================================
// Bewusst ALLE unveraendert als #define/compile-time const: diese Werte haengen rechnerisch zusammen
// (z. B. NCOARSE = Funktion von COARSE_STEP + Bandgrenzen, siehe Kommentar dort) und bilden den eigentlichen
// Sondensuchlauf -- also Programmstruktur/Protokoll im Sinne der Datei-Kopfzeile, keine freie "Einstellung",
// und deshalb explizit NICHT Teil der config.ini (Entscheidung Okt. 2026, Feature v4.95).
const float COARSE_STEP = 0.2;          // Schrittweite Grobscan [MHz]
#define NCOARSE 31                      // Anzahl Grobscan-Schritte -- (406-400)/COARSE_STEP + 1, bei Aenderung
                                         // von COARSE_STEP/Bandgrenzen hier mitrechnen!
const float COARSE_BW   = 467.0;        // Empfaengerbandbreite im Grobscan [kHz], breiter Filter = wenige Schritte noetig
const float FINE_SPAN   = 0.12;         // Feinscan: Bereich links/rechts vom Kandidaten [MHz]
#define FINE_PASSES 4                   // Feinscan-Durchlaeufe (Maximum je Frequenz); die Sonde sendet nicht durchgehend
const float BAND_MIN    = 400.15;       // Wettersonden-Band beginnt hier [MHz]; darunter werden keine Kandidaten gewaehlt
const float FINE_STEP   = 0.010;        // Feinscan-Schrittweite [MHz]
#define MAX_CAND 4                      // so viele Kandidaten werden nacheinander geprueft
const float REL_MARGIN  = 4.0;          // dB ueber dem Median (Rauschen) gilt im Grobscan als Signal
#define SCAN_SWEEPS 5                   // Grobscan wird gemittelt ueber so viele Durchlaeufe (laenger = empfindlicher)
const float FINE_MARGIN = 6.0;          // Feinscan: Peak muss so viele dB ueber dem Median liegen, sonst kein Decodierversuch
static uint32_t RS41_ONESYNC = 1;                       // -- CFG: RS41-Empfang: 0 = altes Zwei-Schritt-Verfahren (Rest im NAECHSTEN Frame, verliert jeden zweiten Frame),
                                                        // 1 = zweiter kurzer Sync im SELBEN Frame auf den Positionsblock (Standard). Bei Misserfolg faellt 1 automatisch
                                                        // auf 0 zurueck. Auch im Menue ("RS41-Empfang", Taste *). Idee: dl9rdz/rdz_ttgo_sonde (https://github.com/dl9rdz/rdz_ttgo_sonde).
static uint32_t SONDE_TYPES = 1;                        // -- CFG: Sondentypen: 0 nur RS41, 1 RS41 + DFM-06/09/17 (Standard), 2 nur DFM. Auch im Menue ("Sondentyp", Taste #).
                                                        // DFM-Decoder nach https://github.com/dl9rdz/rdz_ttgo_sonde (dl9rdz, GPL-2.0+). UNGETESTET auf dem SX1262.
#define ACQ_MS   1350                   // Decodierversuch pro Kandidat [ms] (RS41 sendet 1 Frame/s)
#define LOST_MS  5000                   // so lange ohne Frame, dann zurueck zum Scan [ms]
#define STICKY_MS (20UL * 1000UL)       // nach einer decodierten Sonde: so lange noch gezielt auf ihrer letzten
                                         // Frequenz weiterlauschen (kein voller Suchlauf), bevor wirklich aufgegeben wird
#define SV_MIN  4                       // Sondenposition erst ab so vielen Satelliten (sonst kommt (0,0,0) = "kein Fix")
#define SETTLE_US 1000                  // Einschwingzeit vor der RSSI-Messung [us]

// Sonden-ID-Filter (Menue S "Suche" mit fester ID): wie lange eine erkannte FREMDE Sonde blockiert wird,
// bevor wieder nach der eigenen gesucht wird (siehe findConfirm()/die Decoder-Schleife im .ino):
static uint32_t FILTER_FOREIGN_BLOCK_MS      = 90000UL;   // -- CFG: Fremd-ID klar erkannt (ID lesbar, weicht ab): so lange blockieren
static uint32_t FILTER_FOREIGN_BLOCK_UNK_MS  = 30000UL;   // -- CFG: ID nicht lesbar/nicht bestaetigt: kuerzer blockieren
static int FILTER_UNKNOWN_RETRY_N       = 8;         // -- CFG: so viele erfolglose Versuche (ID nicht lesbar), bevor blockiert wird

// ========================================================================================================
// GPS-Filter (eigene Position) -- siehe gpsFilterFix()/posJumpOk()/gpsFixGoodEnoughToSend() im .ino
// ========================================================================================================
static uint32_t GPS_FILTER = 1;                 // -- CFG: 1 = Ausreisserfilter fuer eigene Position/Hoehe aktiv, 0 = Rohwerte (auch Menue "GPS-Filter")
static float GPS_MAX_SPEED_MS = 55.0f;         // -- CFG: ~200 km/h + Marge, deckt Auto/Rad/Fuss grosszuegig ab --
                                                       // Obergrenze fuer "plausible" Bewegung zwischen zwei Fixes
static float GPS_JUMP_MARGIN_M = 50.0f;        // -- CFG: zusaetzliche Toleranz gegen normales GPS-Jittern
static uint32_t GPS_GLITCH_TIMEOUT_MS = 120000UL;  // -- CFG: nach 2 Min. auch unbestaetigt uebernehmen (z. B. nach Tunnel/Empfangsloch)
static float GPS_FILTER_MAX_HDOP = 3.5f;       // -- CFG: Geometrie-Schranke: Sprung nur bei HDOP <= diesem Wert als
                                                       // "vom naechsten Fix bestaetigt" akzeptieren (kleiner = strenger)
static int GPS_FILTER_MIN_SATS = 5;            // -- CFG: dieselbe Uebernahme zusaetzlich nur ab so vielen Satelliten
static float GPS_ALT_JUMP_M = 40.0f;            // -- CFG: Hoehensprung, der als Ausreisser gilt
static uint32_t GPS_ALT_GLITCH_TIMEOUT_MS = 120000UL;   // -- CFG: nach 2 Min. auch unbestaetigt uebernehmen
static int GPS_SEND_MIN_SATS = 4;              // -- CFG: Mindest-Satellitenzahl, um die eigene Position ueberhaupt
                                                       // zu senden (SondeHub-Listener/LoRa-APRS-Beacon)
static float GPS_SEND_MAX_HDOP = 2.5f;         // -- CFG: <1 exzellent, 1..2 gut, 2..5 brauchbar, >5 fragwuerdig (Richtwerte)
static int GPS_SEND_MIN_EPOCHS = 3;            // -- CFG: so viele gute Fixe (Sat/HDOP/Tempo) in Folge noetig, bevor gesendet wird; jedes "kein Fix" setzt zurueck
#define SAT_MAX 64                 // maximale Anzahl Satelliten im GPS-Radar/der Satellitenliste
static uint32_t SAT_TIMEOUT_MS = 8000UL;      // -- CFG: Satellit aus der Liste entfernen, wenn so lange keine neuen Daten ($--GSV) kamen

// ========================================================================================================
// Karte / Kacheln
// ========================================================================================================
#define BIG_TILES_MAX 8                     // Kachelraster-Groesse je Achse im RAM-Cache -- reicht fuer Bildschirm + 2 Kacheln Rand
#define BIG_REFILL_MAX_TILES (BIG_TILES_MAX * BIG_TILES_MAX)

#define TILE_TEST   0                  // 1 = Selbsttest beim Start (Ordnerlisten bei Zoom 17/18 dauern ca. 1 Minute), 0 = aus
#define TILE_DIR    "/tiles"                   // Basiskarte (OpenCycleMap), auch Ersatz fuer fehlende Kacheln
#define TILE_DIR_TOPO "/tiles_topo"            // Topografische Karte (z. B. DTK + Schummerung)
#define TILE_DIR_TOPPLUS "/tiles_topplus"      // TopPlusOpen (BKG, bundesweit)
#define TILE_DIR_RELIEF "/tiles_relief"        // Gelaendeschummerung
#define TILE_DIR_SAT "/tiles_sat"              // reines Luftbild
#define TILE_BLANK_BYTES 2048                  // JPG-Kacheln bis zu dieser Groesse sind leer (ausserhalb der Abdeckung) -> Basiskarte
#define TILE_ZOOM   16                 // Zoomstufe der Testanzeige (falls dort keine Kachel liegt: naechstniedrigere)

#define MAP_MIN_Z   6                  // kleinste Zoomstufe der Kartenansicht
#define MAP_MAX_Z   19                 // groesste Zoomstufe (echte Kacheln vorhanden, z. B. tiles_topo bis Stufe 19)
#define MAP_IDLE_Z  15                 // Zoomstufe der Karte, solange keine Sonde empfangen wurde (Menue Z ueberschreibt)
#define PAN_DIR      1                 // Scrollrichtung: 1 = Karte folgt der Trackball-Richtung, -1 = umgekehrt
#define BUNDLE_MAX_TILE (400 * 1024)   // groesste Kachel, die aus einem Bundle gelesen wird (PSRAM-Puffer)

// ========================================================================================================
// Landevorhersage
// ========================================================================================================
static double PRED_MIN_SINK = 1.0;               // -- CFG: eigene Landevorhersage erst ab dieser Sinkrate [m/s] (= 3,6 km/h)
static float PRED_SMOOTH_IN_ALPHA  = 0.25f;      // -- CFG: Glaettung der Geschwindigkeit (0..1, kleiner = traeger/ruhiger)
static float PRED_SMOOTH_OUT_ALPHA = 0.3f;       // -- CFG: Glaettung des berechneten Landepunkts selbst
static float KEEP_RECENT_KM = 5.0f;              // -- CFG: (siehe Verwendung im .ino: wie weit "in der Naehe" noch zaehlt)

// Online-Landevorhersage (Tawhiri/SondeHub-API):
#define TAWHIRI_URL              "https://api.v2.sondehub.org/tawhiri"
static uint32_t TAWHIRI_RETRY_MS         = 300000UL;     // -- CFG: normal alle 5 Minuten eine neue Online-Vorhersage versuchen
static uint32_t TAWHIRI_RETRY_FAST_MS     = 60000UL;     // -- CFG: unterhalb TAWHIRI_FAST_ALT_M im Sinkflug: alle 1 Minute
static double TAWHIRI_FAST_ALT_M         = 3000.0;     // -- CFG: Hoehenschwelle fuer den schnelleren Takt
static double TAWHIRI_BURST_ALT_DEFAULT_M = 30000.0;   // -- CFG: Schaetzwert fuer die Platzhoehe, solange die Sonde
                                                        // noch steigt (grobe Hausnummer)
static double TAWHIRI_FALLBACK_VZ0          = 5.0;     // -- CFG: Schaetzwert fuer die Sinkrate NACH dem (noch nicht
                                                        // erfolgten) Platzen [m/s]
static uint32_t TAWHIRI_MAX_SONDE_AGE_MS = 300000UL;     // -- CFG: keine Anfrage mehr, wenn seit > 5 Minuten kein Paket mehr kam
#define ONLINE_PRED_PATH_MAX 48               // max. Anzahl Stuetzpunkte der geparsten Abstiegsstrecke

// ========================================================================================================
// Spuren / Puffer- und Listengroessen
// ========================================================================================================
#define TRAIL_MAX 800              // max. Punkte der angezeigten Sondenspur
#define OWN_MAX 600                // max. Punkte der angezeigten eigenen Spur
#define ALT_TRACK_MAX 14400        // max. Punkte im Hoehentrack-Puffer
#define MISS_CACHE_N 64             // Cache fehlender Kartenkacheln (verhindert wiederholte, erfolglose SD-Zugriffe)
#define SD_MAX_OPEN_FILES 12        // max. gleichzeitig offene SD-Dateien

#define FIND_PICK_MAX 40            // max. Eintraege in der Sonden-Auswahlliste (Taste Q)
#define FIND_PICK_RECENT_MAX 20     // max. zuletzt empfangene Sonden-IDs (vermeidet SD-Suche im Normalfall)
#define FIND_TRACK_MAX 300          // max. Punkte der angezeigten Spur in der Fundmeldungs-Auswahl
#define FIND_BORDER 4               // Rahmenbreite [Px] der Fundmeldungs-Bildschirme (findPick/textInput/findConfirm) --
                                     // bewusst anderer Look als die Kartenansicht, bei Sonnenlicht im Feld gut zu unterscheiden
static uint32_t DRAFT_IDLE_MS = 3000;          // -- CFG: Texteingabe-Entwurf wird so lange nach dem letzten Tastendruck auf die SD gesichert

static uint32_t GPX_RESUME_GRACE_S  = 300;     // -- CFG: Karenzzeit [s] nach einem Neustart, in der derselbe GPX-Track
                                     // fortgesetzt wird (stillschweigend, ohne Nachfrage -- kurzer Neustart/Watchdog/Reset)
static uint32_t GPX_RESUME_ASK_S  = (24UL * 3600UL);   // -- CFG: Feature v4.90: laenger als GPX_RESUME_GRACE_S, aber
                                     // nicht aelter als das hier (Default 24 h) -- z. B. bewusstes Abschalten im
                                     // Gebaeude (schlechter GPS-Empfang) und spaeteres Wiedereinschalten: dann
                                     // fragt confirmGpxResumeAsk() beim Start blockierend J/N, ob derselbe Track
                                     // fortgesetzt werden soll
static uint32_t GPX_RESUME_ASK_MS = (15UL * 1000UL);   // -- CFG: Feature v4.90: Laenge von Countdown + Fortschrittsbalken
                                     // der obigen Abfrage -- reagiert niemand rechtzeitig, wird automatisch
                                     // fortgesetzt (wie "J"), damit das Geraet nicht unbeaufsichtigt an dieser
                                     // Abfrage haengen bleibt
#define GPX_BUF_CAP  300            // PSRAM-Zwischenspeicher je Spur [Punkte], bevor spaetestens auf die SD geschrieben wird
static uint32_t GPX_FLUSH_MS = 30000;          // -- CFG: automatisch auf die SD schreiben, spaetestens alle 30 s

// ========================================================================================================
// LoRa-APRS
// ========================================================================================================
const float APRS_FREQ_MHZ = 433.775;   // Sendefrequenz des eigenen LoRa-APRS-Beacons/Empfangskanal (Funkprotokoll-
                                        // Parameter, bewusst NICHT per config.ini -- siehe loraFreqMHz/Menue F fuer
                                        // die Laufzeit-Einstellung mit NVS-Speicherung)
const float APRS_BW_KHZ   = 125.0;     // Bandbreite des LoRa-APRS-Kanals (ebenfalls Funkprotokoll-Parameter)

#define APRS_MAX 32                 // max. gleichzeitig verwaltete Stationen
#define APRS_TRAIL_MAX 40           // max. Punkte je Stationsspur
#define APRS_TRAIL_KM 5.0f          // (siehe Verwendung im .ino)
static uint32_t APRS_MAX_AGE_MS = (30UL * 60UL * 1000UL);   // -- CFG: Station komplett aus der Liste entfernen, wenn
                                     // laenger nichts mehr kam
#define APRS_COMMENT_LEN 120        // max. Laenge der Zusatzinfo/des Kommentars je Station

static float OWN_SPEED_BIG_KMH = 8.0f;      // -- CFG: ab diesem Tempo wird die grosse Statuszeile mit Reise-Icon
                                     // statt der normalen kleinen Anzeige gezeigt (siehe drawBigOwnStatus())
#define NSYMBOL 4                   // Anzahl Bewegungsmodus-Icons (Stationaer/Fussgaenger/Fahrrad/Bus) -- MUSS zur
                                     // Anzahl der case-Zweige in drawTravelIcon() und zu aprsSymbolChars[]/
                                     // aprsSymbolNames[] im .ino passen; hier nicht aendern, ohne dort mitzuziehen.
static const float aprsSymbolMinKmh[NSYMBOL] = {0.0f, 2.0f, 10.0f, 30.0f};   // Tempo-Schwellen (km/h) je Icon/
                                     // APRS-Symbol, absteigend gestaffelt (Reihenfolge MUSS zu aprsSymbolChars[]/
                                     // aprsSymbolNames[] im .ino passen: Stationaer/Fussgaenger/Fahrrad/Bus)

// Sendeintervall dynamisch nach Tempo (SmartBeaconing-Prinzip): im Stand reicht selten (Akku/Kanal schonen),
// schnell unterwegs soll die eigene Position auf der Karte der anderen nicht zu grob hinter der Realitaet
// zurueckbleiben. Zusaetzlich (unabhaengig vom Timer): sobald seit dem letzten Beacon die jeweilige Distanz
// zurueckgelegt wurde, wird sofort gesendet -- je nachdem, was zuerst zutrifft.
static uint32_t APRS_BEACON_SLOW_MS   = (30UL * 60UL * 1000UL);   // -- CFG: Stand/langsam (<=10 km/h): alle 30 min
static uint32_t APRS_BEACON_MED_MS    = (1UL * 60UL * 1000UL);    // -- CFG: >10 km/h: alle 1 min
static uint32_t APRS_BEACON_FAST_MS   = (30UL * 1000UL);          // -- CFG: >50 km/h: alle 30 s
static double APRS_BEACON_DIST_WALK_M  = 100.0;                 // -- CFG: <=10 km/h (zu Fuss): zusaetzlich sofort nach 100 m Wegstrecke
static double APRS_BEACON_DIST_FAST_M = 500.0;                  // -- CFG: >10 km/h: zusaetzlich sofort nach 500 m Wegstrecke
static uint32_t APRS_BEACON_RETRY_MS   = (5UL * 1000UL);          // -- CFG: Fix bei Faelligkeit nicht gut genug: bald
                                                         // wieder versuchen, statt ein volles Intervall (im Stand
                                                         // 30 min!) zu verschenken

static uint32_t APRS_SYMBOL_HYST_MS = (1UL * 60UL * 1000UL);   // -- CFG: Zielstufe (siehe aprsSymbolMinKmh[]) muss so
                                                      // lange ununterbrochen anliegen, bevor umgeschaltet wird --
                                                      // verhindert Hin-/Herspringen des Icons bei schwankendem
                                                      // Tempo (z. B. an einer Ampel)

#define TXPWR_DEFAULT_DBM 10        // Start-Sendeleistung des LoRa-APRS-Beacons [dBm] (im Menue U aenderbar; ist
                                     // bereits ueber Preferences/NVS persistent, siehe txPowerDbm/loadSettings())
static uint32_t LORA_PAUSE_MS      = (1UL * 60UL * 1000UL);   // -- CFG: Feature v4.88: Taste L in der Kartenansicht
                                                     // pausiert LoRa Upload so lange, dann schaltet es sich von
                                                     // selbst wieder an (siehe loraPauseUntilMs/loraPauseTick() im .ino)
static int LORA_PAUSE_WARN_S   = 5;                       // -- CFG: Feature v4.89: in den letzten so vielen Sekunden
                                                     // der Pause piept es einmal je Sekunde (siehe loraPauseTick())
                                                     // -- Warnung rechtzeitig vor dem automatischen
                                                     // Wiedereinschalten, z. B. falls man gerade die Antenne wechselt
static int LORA_PAUSE_WARN_HZ  = 2200;                    // -- CFG: Tonhoehe dieses Warntons (bewusst anders als
                                                     // BEEP_HZ/BEEP_HZ_NEU_* oben, damit er klar unterscheidbar ist)
static uint32_t LORA_TXRESUME_ASK_MS = (15UL * 1000UL);        // -- CFG: Feature v4.94 (Wunsch Okt. 2026, "so einen
                                                     // Timeout wie beim GPX-Track, default dann NO"): Laenge von
                                                     // Countdown + Fortschrittsbalken der confirmLoraTxResume()-
                                                     // Abfrage beim Start -- reagiert niemand rechtzeitig, verfaellt
                                                     // sie automatisch auf "N" (NICHT senden, siehe dort), anders als
                                                     // bei GPX_RESUME_ASK_MS (dort Default "J")

// ========================================================================================================
// Ton (Empfangs-/Sendepiepser ueber den Lautsprecher)
// ========================================================================================================
#define BEEP_RATE   16000                     // Abtastrate (Hardware-Parameter des Ton-Generators, bewusst fest)
#define BEEP_MS     150                       // Tondauer -- bewusst fest (nicht per config.ini): dimensioniert
                                               // den Tonpuffer static int16_t buf[2*(BEEP_RATE*BEEP_MS/1000)]
                                               // (beepInit() im .ino), muss daher ein Compile-Zeit-Konstante bleiben
static int BEEP_HZ     = 1800;                      // -- CFG: Tonhoehe (normaler Empfangs-/Sendeton)
static int BEEP_HZ_NEU_DIREKT   = 2600;              // -- CFG: neue Station DIREKT per Funk gehoert (hoeher)
static int BEEP_HZ_NEU_REPEATED  = 900;              // -- CFG: neue Station ueber Digipeater (Repeater) gehoert (tiefer)

// ========================================================================================================
// Fundmeldung: Warteliste ohne Internet
// ========================================================================================================
#define RECOVERY_QUEUE_PATH "/recovery_queue.txt"
#define RECOVERY_QUEUE_MAX  10          // max. Eintraege in der Warteliste
static uint32_t RECOVERY_RETRY_MS   = 60000;       // -- CFG: sporadische Pruefung, ob wieder (echtes) Internet da ist
static uint32_t RECOVERY_DRAIN_GAP_MS = 5000;      // -- CFG: Abstand zwischen zwei automatischen Sendungen (keine Sperre riskieren)

// ========================================================================================================
// SondeHub-Listener-Meldung (eigener Standort als Empfangsstation)
// ========================================================================================================
#define SONDEHUB_LISTENER_URL       "https://api.v2.sondehub.org/listeners"
static uint32_t SONDEHUB_LISTENER_CHECK_MS    = 30000UL;     // -- CFG: Bewegung wird alle 30 s geprueft
static uint32_t SONDEHUB_LISTENER_STATIC_RETRY_MS = 900000UL;      // -- CFG: im Stand seltener (alle 15 min) als mobile:false
static double SONDEHUB_MOVE_THRESHOLD_M    = 25.0;         // -- CFG: ab dieser Distanz (je Pruefung) gilt die Station als "bewegt"
#define SONDEHUB_ANTENNA            "Comet SMA-24/tuned T-Deck Antenna"   // eigene Antenne, statt nur Geraetename

// ========================================================================================================
// Trackball-Sperre (Taste 0)
// ========================================================================================================
static uint32_t TBLOCK_DOUBLE_TAP_MS = 2000UL;   // -- CFG: Zeitfenster fuer den schnellen zweiten Tipp auf Taste 0 (an -> an+Tastatur)

#endif
