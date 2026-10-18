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

#ifndef TDECK_SETTINGS_H
#define TDECK_SETTINGS_H

// ========================================================================================================
// WLAN / Netzwerk
// ========================================================================================================
#define WIFI_SSID      "T-Deck-RS41"           // Name des eigenen Zugangspunkts (Menue W, Modus "eigenes Netz")
#define WIFI_PASS      "sonde433"              // mindestens 8 Zeichen -- bitte vor dem ersten Einsatz aendern
#define WIFI_HOST      "tdeck-rs41"            // Rechnername im Heimnetz: http://tdeck-rs41.local
#define WIFI_LOST_MS   10000                   // so lange darf die Heimnetz-Verbindung weg sein (z. B. kurzer
                                                // Aussetzer), bevor das WLAN als "Netz verlassen" gilt und
                                                // abgeschaltet wird
#define WIFI_IDLE_OFF_MS (5UL * 60UL * 1000UL)  // eigener Zugangspunkt (immer nur manuell im Menue W gestartet,
                                                // NIE automatisch): geht nach so langer Zeit ohne verbundenes
                                                // Geraet von selbst wieder aus (spart Akku). Gilt nicht fuer das
                                                // Heimnetz -- dort haengt das T-Deck ohnehin am Strom, WLAN
                                                // bleibt an, solange die Verbindung steht (siehe WIFI_LOST_MS).
#define NTP_SERVER     "pool.ntp.org"   // fuer die Uhrzeit beim Einschalten, solange noch kein GPS-Fix vorliegt
#define NTP_TIMEOUT_MS 8000    // so lange wird nach dem Verbinden auf eine gueltige NTP-Antwort gewartet, dann aufgegeben
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
#define VERBOSE_DUMP   0       // 1 = Rohdaten/entwuerfelte Frames als Hex seriell ausgeben (sehr viel Text), 0 = aus

// ========================================================================================================
// Anzeige / Farben (RGB565: 0xRRRR-Wert = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3))
// ========================================================================================================
#define TFT_INVERT     true    // Displayfarben invertieren -- vom konkreten Panel abhaengig, true fuer das T-Deck Plus

// Kartenmarker/Sondenpunkt:
#define COL_SONDE_OK      ST77XX_GREEN    // Sonde: gueltige Position vor < 4 s (Fuellfarbe, s. lastPosOkMs)
#define COL_SONDE_LATE    ST77XX_YELLOW   // Sonde: 4..19 s keine gueltige Position
#define COL_SONDE_LOST    ST77XX_RED      // Sonde: 20 s oder laenger keine gueltige Position
#define COL_SONDE_RING    ST77XX_BLACK    // Ring um den Sondenpunkt (normal)
#define COL_SONDE_RING_SIG ST77XX_GREEN   // Ring kurz gruen: Schritt 1 (Header) gerade empfangen, aber (noch)
                                           // keine gueltige Position in diesem Takt
#define SONDE_SIGNAL_FLASH_MS 1000UL      // so lange blinkt der Ring nach einem blossen Signal-Empfang (ohne Position)

// Eigene Position/Spur:
#define COL_ICH_FILL      0x07FF          // ich: Fuellung cyan -- gut sichtbar auf allen Kartenarten
#define COL_ICH_RING      ST77XX_BLACK    // ich: Ring
#define COL_LINE          ST77XX_RED      // Linie zwischen mir und der Sonde (2 Pixel breit)
#define COL_OWN_TRAIL     0xF81F          // eigener Track, magenta

// Sondenspur/Landevorhersage:
#define COL_TRAIL         0xFC60          // Spur der Sonde (orange)
#define COL_PRED          0x041F          // Landevorhersage (eigene Berechnung): Verlaengerung der Spur + Fadenkreuz (blau)
#define COL_PRED_ONLINE   ST77XX_YELLOW   // Online-Landevorhersage (Tawhiri/SondeHub-API): nur Fadenkreuz, gelb
#define CROSS_HALF        6               // Fadenkreuz Landepunkt: halbe Linienlaenge in Pixeln

// GPS-Radar:
#define COL_RADAR_RING    0x0400          // Radar: Ringe und Kreuz (dunkelgruen)
#define COL_RADAR_TEXT    0x8410          // Radar: Beschriftungen (grau)

// Hoehentrack:
#define COL_ALT_FILL      0x12EB          // Flaechenfuellung unter der Kurve (dunkles Petrol -- dieselbe Farbe
                                           // wie im Chart der Weboberflaeche, siehe loadAlt() im .ino)

// LoRa-APRS-Stationspunkte, Farbe je nach Empfangsart (siehe aprsRecvKind[] im .ino):
#define COL_APRS_DIRECT   0x001F          // direkt per Funk gehoert (blau)
#define COL_APRS_IGATE    ST77XX_RED      // als Fremdverkehr ueber ein iGate/APRS-IS relayt (rot)
#define COL_APRS_DIGI     0xFC60          // ueber einen genutzten Digipeater-Hop weitergereicht (orange, bewusst = COL_TRAIL)

// ========================================================================================================
// Toast-/Popup-/Menue-Meldungen -- wie lange eine kurze Rueckmeldung am Geraet sichtbar bleibt
// ========================================================================================================
#define BOOT_SPLASH_MS      2500   // Begruessungsbildschirm beim Einschalten
#define TOAST_MS_DEFAULT    1800   // normale kurze Rueckmeldung unten/oben im Bild (z. B. "Ton: an")
#define TOAST_MS_LONG       5000   // laenger lesbare Rueckmeldung, z. B. Trackballklick "Position senden" --
                                    // sonst waehrend der Fahrt/des Laufens kaum zu lesen (siehe toastFor() im .ino)
#define MENU_NOTE_MS        2500   // Hinweiszeile im Einstellungsmenue (z. B. "Foto: ..." nach einem Screenshot)

// ========================================================================================================
// Akku (Spannungsteiler-Kalibrierung, Menue-Statuszeile)
// ========================================================================================================
const float BAT_SCALE  = 2.0f;    // Teilerfaktor des Spannungsteilers
const float BAT_OFFSET = 0.0f;    // Korrektur in Volt (mit Multimeter abgleichen; gemessene Spannung steht in der Logzeile)
const float BAT_USB_V  = 4.30f;   // ab dieser gemessenen Spannung gilt USB/Ladespannung als angesteckt

// ========================================================================================================
// Tastatur-Beleuchtung / Trackball
// ========================================================================================================
#define KB_LEVEL       120     // Helligkeit des Tastenlichts (0..255)
#define KB_LEVEL_DIM    25     // Modus 2 "Taste+dunkel": gedimmte Grundhelligkeit, solange keine Taste gedrueckt wird
#define KB_ON_MS       6000    // so lange leuchten die Tasten nach dem letzten Tastendruck
#define KB_BRIGHT_MS   4000    // Modus 2: so lange bleibt es nach einem Tastendruck hell, bevor es wieder dimmt
#define KB_DARK_ELEV   (-1.0)  // "dunkel" = Sonne tiefer als dieser Winkel unter dem Horizont [Grad]
#define TB_PULSES      2       // so viele Trackball-Impulse gelten als ein Schritt (bei Bedarf anpassen)

// ========================================================================================================
// RS41-Sondensuche: Scan-Parameter (zweistufig -- erst grob mit breitem Filter, dann fein um die Kandidaten)
// ========================================================================================================
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
#define ACQ_MS   1350                   // Decodierversuch pro Kandidat [ms] (RS41 sendet 1 Frame/s)
#define LOST_MS  5000                   // so lange ohne Frame, dann zurueck zum Scan [ms]
#define STICKY_MS (20UL * 1000UL)       // nach einer decodierten Sonde: so lange noch gezielt auf ihrer letzten
                                         // Frequenz weiterlauschen (kein voller Suchlauf), bevor wirklich aufgegeben wird
#define SV_MIN  4                       // Sondenposition erst ab so vielen Satelliten (sonst kommt (0,0,0) = "kein Fix")
#define SETTLE_US 1000                  // Einschwingzeit vor der RSSI-Messung [us]

// Sonden-ID-Filter (Menue S "Suche" mit fester ID): wie lange eine erkannte FREMDE Sonde blockiert wird,
// bevor wieder nach der eigenen gesucht wird (siehe findConfirm()/die Decoder-Schleife im .ino):
#define FILTER_FOREIGN_BLOCK_MS      90000UL   // Fremd-ID klar erkannt (ID lesbar, weicht ab): so lange blockieren
#define FILTER_FOREIGN_BLOCK_UNK_MS  30000UL   // ID nicht lesbar/nicht bestaetigt: kuerzer blockieren
#define FILTER_UNKNOWN_RETRY_N       8         // so viele erfolglose Versuche (ID nicht lesbar), bevor blockiert wird

// ========================================================================================================
// GPS-Filter (eigene Position) -- siehe gpsFilterFix()/posJumpOk()/gpsFixGoodEnoughToSend() im .ino
// ========================================================================================================
static const float GPS_MAX_SPEED_MS = 55.0f;         // ~200 km/h + Marge, deckt Auto/Rad/Fuss grosszuegig ab --
                                                       // Obergrenze fuer "plausible" Bewegung zwischen zwei Fixes
static const float GPS_JUMP_MARGIN_M = 50.0f;        // zusaetzliche Toleranz gegen normales GPS-Jittern
static const uint32_t GPS_GLITCH_TIMEOUT_MS = 120000UL;  // nach 2 Min. auch unbestaetigt uebernehmen (z. B. nach Tunnel/Empfangsloch)
static const float GPS_FILTER_MAX_HDOP = 3.5f;       // Geometrie-Schranke: Sprung nur bei HDOP <= diesem Wert als
                                                       // "vom naechsten Fix bestaetigt" akzeptieren (kleiner = strenger)
static const int GPS_FILTER_MIN_SATS = 5;            // dieselbe Uebernahme zusaetzlich nur ab so vielen Satelliten
static const float GPS_ALT_JUMP_M = 40.0f;            // Hoehensprung, der als Ausreisser gilt
static const uint32_t GPS_ALT_GLITCH_TIMEOUT_MS = 120000UL;   // nach 2 Min. auch unbestaetigt uebernehmen
static const int GPS_SEND_MIN_SATS = 4;              // Mindest-Satellitenzahl, um die eigene Position ueberhaupt
                                                       // zu senden (SondeHub-Listener/LoRa-APRS-Beacon)
static const float GPS_SEND_MAX_HDOP = 2.5f;         // <1 exzellent, 1..2 gut, 2..5 brauchbar, >5 fragwuerdig (Richtwerte)
#define SAT_MAX 64                 // maximale Anzahl Satelliten im GPS-Radar/der Satellitenliste
#define SAT_TIMEOUT_MS 8000UL      // Satellit aus der Liste entfernen, wenn so lange keine neuen Daten ($--GSV) kamen

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
#define PRED_MIN_SINK 1.0               // eigene Landevorhersage erst ab dieser Sinkrate [m/s] (= 3,6 km/h)
#define PRED_SMOOTH_IN_ALPHA  0.25f      // Glaettung der Geschwindigkeit (0..1, kleiner = traeger/ruhiger)
#define PRED_SMOOTH_OUT_ALPHA 0.3f       // Glaettung des berechneten Landepunkts selbst
#define KEEP_RECENT_KM 5.0f              // (siehe Verwendung im .ino: wie weit "in der Naehe" noch zaehlt)

// Online-Landevorhersage (Tawhiri/SondeHub-API):
#define TAWHIRI_URL              "https://api.v2.sondehub.org/tawhiri"
#define TAWHIRI_RETRY_MS         300000UL     // normal alle 5 Minuten eine neue Online-Vorhersage versuchen
#define TAWHIRI_RETRY_FAST_MS     60000UL     // unterhalb TAWHIRI_FAST_ALT_M im Sinkflug: alle 1 Minute
#define TAWHIRI_FAST_ALT_M         3000.0     // Hoehenschwelle fuer den schnelleren Takt
#define TAWHIRI_BURST_ALT_DEFAULT_M 30000.0   // Schaetzwert fuer die Platzhoehe, solange die Sonde noch steigt (grobe Hausnummer)
#define TAWHIRI_FALLBACK_VZ0          5.0     // Schaetzwert fuer die Sinkrate NACH dem (noch nicht erfolgten) Platzen [m/s]
#define TAWHIRI_MAX_SONDE_AGE_MS 300000UL     // keine Anfrage mehr, wenn seit > 5 Minuten kein Paket mehr kam
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
#define DRAFT_IDLE_MS 3000          // Texteingabe-Entwurf wird so lange nach dem letzten Tastendruck auf die SD gesichert

#define GPX_RESUME_GRACE_S  300     // Karenzzeit [s] nach einem Neustart, in der derselbe GPX-Track fortgesetzt wird
#define GPX_BUF_CAP  300            // PSRAM-Zwischenspeicher je Spur [Punkte], bevor spaetestens auf die SD geschrieben wird
#define GPX_FLUSH_MS 30000          // automatisch auf die SD schreiben, spaetestens alle 30 s

// ========================================================================================================
// LoRa-APRS
// ========================================================================================================
const float APRS_FREQ_MHZ = 433.775;   // Sendefrequenz des eigenen LoRa-APRS-Beacons/Empfangskanal
const float APRS_BW_KHZ   = 125.0;     // Bandbreite des LoRa-APRS-Kanals

#define APRS_MAX 32                 // max. gleichzeitig verwaltete Stationen
#define APRS_TRAIL_MAX 40           // max. Punkte je Stationsspur
#define APRS_TRAIL_KM 5.0f          // (siehe Verwendung im .ino)
#define APRS_MAX_AGE_MS (30UL * 60UL * 1000UL)   // Station komplett aus der Liste entfernen, wenn laenger nichts mehr kam
#define APRS_COMMENT_LEN 120        // max. Laenge der Zusatzinfo/des Kommentars je Station

#define OWN_SPEED_BIG_KMH 8.0f      // ab diesem Tempo wird die grosse Statuszeile mit Reise-Icon statt der
                                     // normalen kleinen Anzeige gezeigt (siehe drawBigOwnStatus())
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
#define APRS_BEACON_SLOW_MS   (30UL * 60UL * 1000UL)   // Stand/langsam (<=10 km/h): alle 30 min
#define APRS_BEACON_MED_MS    (1UL * 60UL * 1000UL)    // >10 km/h: alle 1 min
#define APRS_BEACON_FAST_MS   (30UL * 1000UL)          // >50 km/h: alle 30 s
#define APRS_BEACON_DIST_WALK_M  100.0                 // <=10 km/h (zu Fuss): zusaetzlich sofort nach 100 m Wegstrecke
#define APRS_BEACON_DIST_FAST_M 500.0                  // >10 km/h: zusaetzlich sofort nach 500 m Wegstrecke
#define APRS_BEACON_RETRY_MS   (5UL * 1000UL)          // Fix bei Faelligkeit nicht gut genug: bald wieder versuchen,
                                                         // statt ein volles Intervall (im Stand 30 min!) zu verschenken

#define APRS_SYMBOL_HYST_MS (1UL * 60UL * 1000UL)   // Zielstufe (siehe aprsSymbolMinKmh[]) muss so lange ununterbrochen
                                                      // anliegen, bevor umgeschaltet wird -- verhindert Hin-/Herspringen
                                                      // des Icons bei schwankendem Tempo (z. B. an einer Ampel)

#define TXPWR_DEFAULT_DBM 10        // Start-Sendeleistung des LoRa-APRS-Beacons [dBm] (im Menue U aenderbar)

// ========================================================================================================
// Ton (Empfangs-/Sendepiepser ueber den Lautsprecher)
// ========================================================================================================
#define BEEP_RATE   16000                     // Abtastrate
#define BEEP_MS     150                       // Tondauer
#define BEEP_HZ     1800                      // Tonhoehe (normaler Empfangs-/Sendeton)
#define BEEP_HZ_NEU_DIREKT   2600              // neue Station DIREKT per Funk gehoert (hoeher)
#define BEEP_HZ_NEU_REPEATED  900              // neue Station ueber Digipeater (Repeater) gehoert (tiefer)

// ========================================================================================================
// Fundmeldung: Warteliste ohne Internet
// ========================================================================================================
#define RECOVERY_QUEUE_PATH "/recovery_queue.txt"
#define RECOVERY_QUEUE_MAX  10          // max. Eintraege in der Warteliste
#define RECOVERY_RETRY_MS   60000       // sporadische Pruefung, ob wieder (echtes) Internet da ist
#define RECOVERY_DRAIN_GAP_MS 5000      // Abstand zwischen zwei automatischen Sendungen (keine Sperre riskieren)

// ========================================================================================================
// SondeHub-Listener-Meldung (eigener Standort als Empfangsstation)
// ========================================================================================================
#define SONDEHUB_LISTENER_URL       "https://api.v2.sondehub.org/listeners"
#define SONDEHUB_LISTENER_CHECK_MS    30000UL     // Bewegung wird alle 30 s geprueft
#define SONDEHUB_LISTENER_STATIC_RETRY_MS 900000UL      // im Stand seltener (alle 15 min) als mobile:false
#define SONDEHUB_MOVE_THRESHOLD_M    25.0         // ab dieser Distanz (je Pruefung) gilt die Station als "bewegt"
#define SONDEHUB_ANTENNA            "Comet SMA-24/tuned T-Deck Antenna"   // eigene Antenne, statt nur Geraetename

// ========================================================================================================
// Trackball-Sperre (Taste 0)
// ========================================================================================================
#define TBLOCK_DOUBLE_TAP_MS 2000UL   // Zeitfenster fuer den schnellen zweiten Tipp auf Taste 0 (an -> an+Tastatur)

#endif
