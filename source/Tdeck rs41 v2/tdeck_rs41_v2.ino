// T-Deck Plus – RS41 Empfang v2 (SX1262, FSK)
//
// ===========================================================================
// VOR DEM ERSTEN COMPILIEREN:
//   config.h.example im selben Ordner nach config.h kopieren und darin das
//   eigene Rufzeichen sowie die eigenen Heimat-Koordinaten eintragen.
//   config.h ist persoenlich (steht in .gitignore, landet NICHT auf GitHub).
// ===========================================================================
//
// Neu gegenueber dem Empfangstest:
//  - Bytes werden bitweise gedreht (RS41 sendet LSB zuerst) und mit der
//    64-Byte-XOR-Maske entwuerfelt
//  - Jeder Block wird per CRC16 geprueft und mit ID angezeigt (+ = CRC ok)
//  - Anzeige von Sonden-ID, Framenummer und GPS-Zeit
//  - Karte: Spur der Sonde, Tempo in km/h und Richtung, Steig-/Sinkrate in m/s
//  - Landevorhersage beim Sinken: blaue Verlaengerung der Spur + Fadenkreuz (Karte)
//  - Bedienung: Space/Trackball-Klick = Ansicht (Text -> Karte -> Hoehentrack/Stationen -> GPS-Radar; im LoRa-
//    Modus ohne die sinnlose Textansicht: nur Karte -> Stationsliste -> GPS-Radar), Enter = Konfigurationsmenue.
//  - GPS-Radar (eigene Ansicht): Polarplot der einzelnen Satelliten (Azimut/Elevation aus $--GSV), Mitte =
//    Zenit, Ringe alle 30 Grad bis zum Horizont am Rand, dazu eine Textliste mit PRN/Elevation/Azimut/SNR.
//    Im Menue waehlt der Buchstabe nur den Punkt (S Suche, F Frequenz, B Bandbreite, K Fehlerkorrektur,
//    I Funkmodus, V Vorhersage, L Landehoehe, Z Zoom, Y Zoomschritt, G Scroll-Empfindlichkeit, O Eigener Track, X Ton, M Kartenart, H Helligkeit,
//    T Tastenlicht, J GPX-Aufzeichnung, P SD-Log, W WLAN, E Netz, Q Herunterfahren).
//  - Sonde gefunden (Taste Q, ausserhalb des Menues): sendet Sonden-ID, Position zum Zeitpunkt des
//    Tastendrucks (= letzte empfangene Sondenposition), Melder-Name (wird abgefragt/gemerkt) und einen
//    frei eingegebenen Fundbericht per PUT an https://api.v2.sondehub.org/recovered. Wird von SondeHub
//    automatisch auch zu radiosondy.info weitergereicht. Braucht das Heimnetz (Menue W) mit Internetzugang.
//  - Geordnetes Herunterfahren (Menue Q, zweimal Space bestaetigen): sichert den GPX-Zwischenspeicher,
//    beendet WLAN und Funkempfang und schaltet das Board dann wirklich ab (GPIO10/BOARD_POWERON auf LOW,
//    haelt laut LilyGO-Doku im Batteriebetrieb die Stromversorgung -- LOW schaltet sie ab), Tiefschlaf nur
//    als Rueckfall, falls das Board trotzdem weiterlaeuft (z. B. am USB-Kabel).
//    Werte: Ziffern tippen + Enter, oder Trackball links/rechts (klein) und hoch/runter (gross); Space = weiter.
//  - Karte: A/O = herauszoomen, D/P = hineinzoomen (A/D links, O/P rechts auf der Tastatur fuer Einhandbedienung;
//    Schrittweite im Menue Y, wirkt aber erst sichtbar ab der naechsten ganzen Zoomstufe), Trackball = Karte scrollen
//    (Empfindlichkeit im Menue G). R schaltet zurueck auf automatisch und zykelt dabei durch Sonde+Ich+Landepunkt /
//    Sonde+Ich / Sonde / Ich. Bei einer neuen Sonde beginnt die Karte wieder bei Sonde+Ich+Landepunkt. Liegt ein
//    Punkt ausserhalb des Bildes, zeigt ein Pfeil am Bildrand in seine Richtung. Der manuelle Zoom wird nicht gespeichert.
//    Eigener Track (Menue O): die Spur des T-Deck (GPS) wird in der Karte als violette Linie gezeichnet.
//  - GPX-Aufzeichnung (Menue J, Standard an): eigene Position und Sonde werden zusaetzlich als
//    <Datum_Uhrzeit>.gpx (Ich, Zeitpunkt des ersten GPS-Fix nach dem Einschalten) und <Sondennummer>.gpx
//    (Sonde) auf der SD-Karte gespeichert, zum spaeteren Import in Kartenprogramme. Jeder Punkt wird ungekuerzt gespeichert
//    (unabhaengig von der Ausduennung der angezeigten Kartenspuren), nur auf der SD, keine Anzeige.
//    Punkte werden im PSRAM gepuffert und erst alle 30 s (oder wenn der Puffer voll ist) tatsaechlich
//    geschrieben; Taste U schreibt beide Puffer sofort weg -- vor dem Ausschalten druecken!
//  - Screenshot: Taste C aus jedem Bildschirm (JPEG mit EXIF-Koordinaten von Sonde bzw. T-Deck in /shots);
//    Menue W: WLAN aus / Heimnetz / eigener Zugangspunkt; Menue E: Heimnetz (Name, Passwort) eintippen;
//    ueber die Webseite sind Screenshots und Logs im Browser abrufbar
//  - Fernsteuerung (Webseite /menu): alle Menuepunkte ausser "Herunterfahren" (Sicherheit) und "Netz"
//    (SSID/Passwort-Eingabe blockiert das Geraet auf die eingebaute Tastatur) lassen sich von dort aus
//    genauso verstellen wie am Geraet, dazu ein Live-Bild des aktuellen Displays (/live.jpg, ohne SD-Karte,
//    direkt aus dem Bildpuffer kodiert)
//  - Kartenarten (Menue M, in der Karte direkt mit Taste M): Rad (/tiles, OpenCycleMap, PNG), Luftbild (/tiles_sat), Topo (/tiles_topo), Relief (/tiles_relief), TopPlusOpen (/tiles_topplus), sonst JPG oder PNG;
//    fehlende oder leere Kacheln (ausserhalb der Abdeckung) werden durch die Basiskarte (/tiles) ersetzt
//  - Schritt 2: im naechsten Frame Sync auf Block 0x7D, dann Rest des Frames
//    (enthaelt den GPS-Positionsblock 0x7B; zu weit hinten fuer 255 Byte ab Header)
//  - LoRa-APRS (Menue I schaltet Sondenempfang <-> LoRa um, nicht gleichzeitig moeglich): europaeische
//    Konvention 433.775 MHz / SF12 / BW125 / CR4-5 (noch nicht an echtem Empfang getestet, siehe aprsRadioInit()/
//    aprsParsePosition()). Auf der Karte wird die Sonde in diesem Modus nicht angezeigt, gehoerte Stationen
//    bekommen Punkt + Rufzeichen + eine auf 5 km gekappte Spur. Menue A ("SondeHub-Upload") ist im LoRa-Modus
//    umgewidmet zu "LoRa Upload": schickt die eigene Position als APRS-Beacon -- nur ueber das Funkmodul,
//    keine Internetverbindung. Sendeintervall dynamisch nach Tempo (SmartBeaconing): im Stand/<=10 km/h
//    alle 30 min, >10 km/h alle 2 min, >50 km/h alle 30 s, zusaetzlich sofort bei zurueckgelegter Strecke
//    seit dem letzten Beacon (<=10 km/h/zu Fuss: 100 m, sonst 1 km) -- je nachdem, was zuerst zutrifft
//    (siehe aprsBeaconTick()). Taucht eine Station NEU in der Liste auf (vorher unbekannt oder laenger als
//    APRS_MAX_AGE_MS nicht gehoert), piept es zusaetzlich zum normalen Empfangston kurz auf -- hoeher bei
//    direktem Funkempfang, tiefer bei Empfang ueber einen Digipeater (siehe aprsNewStationBeep()). Der
//    eigentliche SondeHub-Upload bleibt unberuehrt und steht nach Ruecksprung in den Sondenempfang wieder
//    genauso da wie vorher. Nach jedem Moduswechsel und jedem Neustart ist LoRa Upload immer aus (bewusst
//    nicht gespeichert). Keine APRS-IS/iGate-Weiterleitung -- rein lokaler Funkbetrieb.
//  - Weboberflaeche /menu: das Live-Bild aktualisiert sich automatisch (Dropdown: aus/30 s/60 s), und der
//    Browser piept per Web Audio API bei einem neuen Sondenfund (/mstat-Abfrage alle 5 s, siehe sondeFoundSeq).
//  - Firmware-Update per Arduino-IDE ueber WLAN (ArduinoOTA, Teil des ESP32-Kernpakets, keine zusaetzliche
//    Bibliothek): das T-Deck taucht als Netzwerkport (Werkzeuge -> Port) auf, sobald es im Heimnetz oder im
//    eigenen Zugangspunkt per mDNS erreichbar ist. Bewusst ohne Passwort, siehe otaStart().
//  - Hoehentrack: dritte Ansicht (Space/Trackball-Klick, nach Karte), siehe altTrackUpdate(). Im
//    Sondenmodus ein Hoehendiagramm seit dem Fund der aktuellen Sonde (altTrackAdd(), roh im PSRAM
//    gespeichert, keine Ausduennung), im LoRa-Modus stattdessen die Liste der gehoerten APRS-Stationen mit
//    Entfernung und "zuletzt gehoert". Dieselbe Unterscheidung spiegelt sich auf der Menueseite (/menu) als
//    Feld neben dem Live-Bild (Endpunkt /malt, siehe webAltStat()), im selben Takt wie das Bild. Das
//    Hoehendiagramm skaliert bei jedem Zeichnen die GESAMTE Kurve (erstes bis letztes Paket) neu auf die
//    Chartbreite -- ohne neuen Empfang bleibt der letzte Wert am rechten Rand stehen, nach einer Funkpause
//    verbindet eine gerade Linie die Punkte davor/danach. Die Flaeche unter der Kurve wird bis zur tiefsten
//    empfangenen Hoehe gefuellt, fuer bessere Ablesbarkeit.
//  - Unbekannte Sonden (ID noch nie per Statusblock bestaetigt, Platzhalter "????????") erzeugen keine
//    GPX-Logeintraege mehr -- siehe trackSonde(). Taucht spaeter doch noch eine gueltige ID auf, beginnt
//    ab da ganz normal ein neuer GPX-Track.
//  - ArduinoOTA-Fix: der Dienst wurde doppelt bei mDNS angemeldet (einmal von ArduinoOTA.begin() selbst,
//    einmal zusaetzlich von Hand, siehe otaStart()) -- das fuehrte zu einem Absturz direkt nach einem
//    Upload (kein Log, automatischer Rollback auf die vorherige Firmware) und dazu, dass die Arduino-IDE
//    faelschlich ein Passwort verlangte. Die doppelte Anmeldung ist raus. Zusaetzlich: der
//    Hoehentrack-Puffer wird jetzt ueber heap_caps_malloc(..., MALLOC_CAP_SPIRAM) statt "new[]" angelegt,
//    das bei Speichermangel abstuerzen wuerde statt wie erwartet nullptr zurueckzugeben.
//  - Temporaere Diagnose-Haltepunkte in setup()/loop() (siehe die "### setup():"/"### loop():"-Log-Zeilen):
//    landen vollstaendig im SD-Log (siehe logLine(), jede Zeile einzeln geoeffnet/geschrieben/geschlossen),
//    auch ohne Serial Monitor. Damit gefunden: ein Haenger in inputBegin() (I2C-Tastenlicht), aber nur mit
//    hinterlegtem Heimnetz -- WiFi.begin() lief bis dahin VOR inputBegin() und zog beim Radio-Einschalten
//    kurz deutlich mehr Strom (Brownout-/Buskollisions-Verdacht auf dem T-Deck mit Akku+Display+LoRa+SD+I2C
//    gleichzeitig aktiv). Der WiFi-Verbindungsaufbau in setup() startet jetzt erst NACH inputBegin().
//    Zweiter Diagnose-Durchgang (T-Deck dauerhaft an USB, Stromversorgung also kein Faktor): der Haenger
//    trat NUR nach einem WLAN/OTA-Upload auf, nie nach seriellem Upload. Unterschied: ein serieller Upload
//    loest einen echten Hardware-Reset (EN-Pin) aus, ArduinoOTA/esp_restart() nur einen Software-Reset.
//    GPIO10 (PIN_POWER_ON) schaltet laut LilyGO die Stromversorgung der Peripherie (Tastatur/Trackball);
//    bei einem Hardware-Reset wird diese Leitung dabei kurz stromlos (echter Power-Cycle der Peripherie),
//    bei einem Software-Reset kann sie durchgehend HIGH bleiben -- die I2C-Tastatur wird dann nie neu
//    eingeschaltet und kann in einem haengenden Zustand verbleiben. setup() schaltet GPIO10 jetzt beim
//    Start ausdruecklich erst LOW, dann (nach 50 ms) wieder HIGH, um bei jedem Start unabhaengig von der
//    Reset-Art einen echten Power-Cycle dieser Schiene zu erzwingen.
//  - Dieser GPIO10-Power-Cycle hat den Haenger NICHT behoben (gleiches Ergebnis wie zuvor) -- die bisherigen
//    Diagnose-Haltepunkte waren nur grob (vor/nach inputBegin() als Ganzes). inputBegin() hat jetzt eigene,
//    feinere Haltepunkte (vor/nach Wire.begin(), nach kbSetRaw(0), nach den Trackball-pinMode()-Aufrufen,
//    nach attachInterrupt()), um einzukreisen, welcher einzelne Aufruf darin nach einem WLAN/OTA-Upload haengt.
//  - Drittes Diagnose-Ergebnis: Wire.begin() und der I2C-Transfer (kbSetRaw(0)) laufen beide durch, der
//    Haenger liegt also NICHT in der I2C-Tastatur, sondern irgendwo bei den fuenf Trackball-pinMode()-
//    Aufrufen danach -- fuer sich genommen ungewoehnlich, da pinMode() normalerweise nie blockiert. Jeder
//    einzelne pinMode()-Aufruf hat jetzt seinen eigenen Haltepunkt (inkl. GPIO-Nummer in der Log-Zeile).
//  - Viertes Diagnose-Ergebnis: trotz der feineren Haltepunkte blieb das Log dreimal identisch stehen bei
//    "nach kbSetRaw(0)" -- keiner der Haltepunkte NACH dem ersten pinMode()-Aufruf (PIN_TB_CLICK = GPIO0)
//    erschien je. Da logLine() selbst die einzige bisherige Diagnosequelle ist und somit nicht sauber von
//    einem Haenger IN pinMode() unterschieden werden kann (zirkulaer), schreibt inputBegin() jetzt zusaetzlich
//    einen vom SD-Log unabhaengigen Haltepunkt direkt aufs TFT (ueber line(), das bereits fuer "Radio OK"/
//    "SD-Karte OK" beweisbar funktioniert) unmittelbar vor und nach genau diesem einen pinMode()-Aufruf.
//    Erscheint "vor" aber nie "nach" auf dem Display, haengt pinMode(GPIO0) selbst (Boot-Strapping-Pin);
//    erscheinen beide, sitzt der Haenger stattdessen in der folgenden logLine().
//  - Fuenftes Diagnose-Ergebnis (geloest): "vor" erschien, "nach" nie -- pinMode(PIN_TB_CLICK=GPIO0) haengt
//    tatsaechlich selbst. Zusaetzlich am USB-Kabel seriell mitgelesen (unabhaengig vom TCP/OTA-Upload moeglich,
//    da die USB-Verbindung durchgehend steht): kein Absturz/Panic, der Reset-Grund direkt nach dem OTA-Upload
//    ist "rst:0xc (RTC_SW_CPU_RST)". Das ist die Ursache: RTC_SW_CPU_RST (von esp_restart(), das ArduinoOTA
//    nach einem erfolgreichen Schreibvorgang selbst aufruft) resettet auf diesem Chip NUR den CPU-Kern, nicht
//    aber die GPIO-/IO-Mux-Peripherieregister -- anders als ein echter Hardware-Reset ueber den EN-Pin (wie ihn
//    ein serieller Upload ausloest), der die gesamte Peripherie sauber neu initialisiert. Die ALTE, noch
//    laufende Firmware hatte GPIO0 (und die anderen vier Trackball-Pins) zum Zeitpunkt des OTA-Updates bereits
//    per pinMode()+attachInterrupt() konfiguriert (Pullup + Flanken-Interrupt aktiv); diese rohe
//    Hardwarekonfiguration ueberlebt den CPU-Reset unveraendert, und pinMode() allein (gpio_set_direction()/
//    gpio_set_pull_mode()) setzt eine noch aktive Interrupt-Freigabe nicht zurueck. Fix: inputBegin() ruft jetzt
//    vor der eigentlichen Konfiguration fuer jeden Trackball-Pin gpio_reset_pin() auf, das IO-Mux-Funktion,
//    Pull-Widerstaende UND Interrupt-Freigabe hart auf den Default zuruecksetzt -- unabhaengig vom Reset-Grund.
//    Die TFT-Diagnosezeilen sind wieder raus, die SD-Log-Haltepunkte bleiben vorerst zur Bestaetigung drin.
//  - Bestaetigt (Test vom 13.10., vollstaendiges Bootlog per OTA, kein Haenger mehr): alle temporaeren
//    SD-Log-Diagnose-Haltepunkte in inputBegin()/setup()/loop() sind jetzt wieder raus, der gpio_reset_pin()-
//    Fix selbst bleibt natuerlich drin.
//  - LoRa-APRS-Rohempfangs-Diagnose: bisher wurde ein empfangenes Paket NUR geloggt, wenn aprsHandlePacket()
//    daraus erfolgreich eine Station+Position machte -- bei Empfangsstille liess sich so nicht unterscheiden,
//    ob ueberhaupt Pakete ankommen. aprsPoll() loggt jetzt JEDES vom Funkmodul mit korrektem CRC gelieferte
//    Paket (Laenge, RSSI, SNR, druckbare Rohvorschau aprsRawPreview()) sowie CRC-/Lesefehler mit RSSI/SNR;
//    aprsRxTotal/aprsRxOk zaehlen mit und erscheinen jetzt oben in der Stationen-Ansicht (altTrackUpdate())
//    und im /malt-JSON (rxtot/rxok/rxage/rssi), sichtbar auch auf der Menueseite im Browser. Parameter
//    (433.775 MHz / SF12 / BW125 / CR4-5 / Syncword 0x12 / Preamble 8) entsprechen der in Deutschland/Europa
//    ueblichen LoRa-APRS-Konvention (z. B. Firmware lora-aprs/LoRa_APRS_iGate), sollten also zu gaengigen
//    Digipeatern/iGates wie DB0JKB-10 oder DK7HS passen -- ob tatsaechlich etwas ankommt, zeigt jetzt
//    aprsRxTotal direkt an, unabhaengig davon, ob der Positions-Parser das Paket erkennt.
//  - LoRa-APRS-Objektreports: ein eigenes iGate (CA2RXU-Firmware, "Objekte werden uebertragen" im Display)
//    verbreitet fremde Positionen (z. B. aus APRS-IS/SondeHub geholte Daten) als APRS-Objektreport (Infofeld
//    beginnt mit ';', eigenes Format nach APRS-Spec Kap. 11) -- NICHT als eigene Positionsmeldung der
//    sendenden Station ("!"/"="). aprsParsePosition() erkannte nur Letzteres, Objektreports wurden also vom
//    T-Deck komplett verworfen (genau das liess sich an der Rohempfangs-Diagnose aus 4.19 ablesen: Pakete
//    kommen an, aber "0 ok"). Neue aprsParseObject() liest jetzt auch Objektreports, Objektname (statt
//    Rufzeichen der sendenden Station) landet in der Stationsliste.
//  - Tatsaechliche Ursache gefunden (Rohempfangs-Diagnose aus 4.19 zeigte den Inhalt): die Pakete sind kein
//    Objektreport, sondern APRS-"Fremdverkehr" (Third-Party Traffic, Infofeld beginnt mit '}') -- das eigene
//    iGate reicht darueber aus APRS-IS/Internet gehoerte Stationen unveraendert als eingebettete TNC2-Zeile
//    ("}URSPRUNGS-RUFZEICHEN>PFAD:INFO") per LoRa weiter. aprsHandlePacket() erkennt das jetzt, entpackt die
//    eingebettete Zeile und wertet sie genauso aus wie ein direkt empfangenes Paket (aprsStoreFromInfo(),
//    gemeinsame Speicher-/Log-Logik fuer direkten und Fremdverkehr). Die Rohvorschau in der Diagnose (4.19)
//    ist dabei von 40 auf 100 Zeichen vergroessert, verschachtelter Fremdverkehr wurde sonst abgeschnitten.
//  - Bug in der LoRa-APRS-Stationsliste behoben: war der Puffer voll (APRS_MAX = 16 Stationen), wurde bei
//    einer neuen, unbekannten Station bisher immer Index 0 ueberschrieben (Kommentar nannte das faelschlich
//    "die aelteste Station") -- nach einer Weile Laufzeit ist Index 0 aber nur zufaellig "die zuletzt dort
//    neu angelegte", wodurch es aussah, als wuerde nur eine einzelne Zeile aktualisiert, statt die am
//    laengsten inaktive Station zu ersetzen. aprsFindOrAdd() sucht jetzt wirklich die Station mit dem
//    kleinsten aprsSeenMs (am laengsten nicht gehoert) zum Ersetzen. Zusaetzlich zeigen sowohl die
//    Stationen-Ansicht am Geraet (altTrackUpdate()) als auch die /malt-Webtabelle die Stationen jetzt nach
//    zuletzt gehoert sortiert (neueste zuerst) statt in fester Array-Reihenfolge, wie bei einer normalen
//    APRS-Stationsliste erwartet.
//  - Radar-Ansicht komplett entfernt (ungenutzt) -- Space/Trackball-Klick zykelt jetzt nur noch durch 3 statt
//    4 Ansichten: Text -> Karte -> Hoehentrack/Stationen. Dafuer im LoRa-APRS-Modus Taste R jetzt mit echter
//    Funktion: zykelt zwischen "nur Ich" (wie bisher), "Ich + alle gehoerten Stationen" und "Ich + nur die
//    DIREKT per Funk gehoerten Stationen" (ohne per iGate/Fremdverkehr relayte, siehe aprsRecvKind[]).
//    mapViewSetup() kann dafuer jetzt beliebig viele Punkte einrahmen (vorher starr auf 3 begrenzt, genug fuer
//    den Sondenmodus, aber nicht fuer "Ich + bis zu 16 Stationen"); fitZoom() konnte das schon immer.
//  - LoRa-APRS-Stationen auf der Karte: Rufzeichen stehen jetzt auf einem halbtransparenten schwarzen
//    Hintergrund (50% Deckung, siehe blend50Black()/fillRectBlend50Black()) statt direkt auf dem
//    Kartenuntergrund -- weisse Schrift bleibt so auch auf hellen Kartenarten lesbar. Punktfarbe zeigt jetzt
//    die Empfangsart: blau = direkt per Funk gehoert, orange = ueber einen genutzten ("*"-markierten)
//    Digipeater-Hop weitergereicht, rot = als Fremdverkehr ueber ein iGate/APRS-IS hereingekommen
//    (aprsRecvKind[], siehe COL_APRS_*). Dazu ein neuer aktiver Verfall: eine Station wird jetzt 15 Minuten
//    nach ihrer letzten Meldung komplett aus der Liste entfernt (aprsPrune(), alle 10 s geprueft) -- vorher
//    blieb sie bis zum naechsten vollen Puffer stehen. Betrifft Stationsliste (Geraet/Web) und Karte
//    gleichermassen, da beide aus demselben Array lesen; der Puffer bleibt bei maximal APRS_MAX = 16
//    gleichzeitig angezeigten Stationen.
//  - Bug auf der Weboberflaeche (/menu) behoben: im LoRa-APRS-Modus erschien neben dem Live-Bild weiterhin das
//    (dort leere/ungenutzte) Hoehentrack-Diagramm statt der Stationsliste -- das hing bisher komplett von
//    JavaScript ab (loadAlt() versteckte das Canvas erst NACH dem ersten erfolgreichen /malt-Abruf), webMenu()
//    liefert Titel ("LoRa-APRS-Stationen" statt "Hoehentrack") und Canvas-Sichtbarkeit (display:none) jetzt
//    schon beim ersten Ausliefern passend zum aktuellen radioMode mit aus. Dabei auch das Canvas von 280x180
//    auf 320x240 vergroessert -- genau die Aufloesung des T-Deck-Displays, statt einer willkuerlichen Groesse
//    (die Zeichenroutine liest c.width/c.height ohnehin dynamisch, skaliert also automatisch mit).
//  - NOCH UNGEKLAERT, Diagnose eingebaut: gemeldeter Haenger beim Druecken von R im LoRa-Modus, wenn
//    "besonders viele Stationen" empfangen wurden (Puffer voll, 16 Stationen) -- Bildschirm friert komplett
//    ein (Video zeigt 2,9 s unveraendertes Bild). Ursache trotz gruendlicher Durchsicht von mapViewSetup()/
//    fitZoom()/prepareMapBackground()/drawAprsStations() NICHT gefunden (alle Schleifen dort sind durch
//    APRS_MAX/feste Canvas-/Bildschirmgroessen beschraenkt, keine erkennbare Endlosschleife). Da sich der
//    Haenger hier nicht nachstellen laesst (kein Geraet), stattdessen scharf geschaltete Checkpoint-Logs
//    eingebaut ("DIAG R-Haenger: ..."): focusCycleOrRecenter() scharft bei jedem R-Druck im LoRa-Modus
//    (focusDbgArm) den NAECHSTEN Bildaufbau; idleMapUpdate() loggt dann (logLine(), also auch auf die SD, nicht
//    nur seriell) bei genau diesem EINEN Durchlauf jeden Zwischenschritt (Punktliste, mapViewSetup(),
//    prepareMapBackground(), drawOwnTrail(), drawAprsStations(), blit()) und entwaffnet sich danach wieder
//    selbst -- kein Dauerlogging. Tritt der Haenger erneut auf, zeigt das Log den letzten erreichten Schritt.
//    Nach Fund der Ursache wieder entfernen.
//  - APRS-Objektnamen (aprsParseObject(), z. B. aus SondeHub-/Ballon-Gateways) werden jetzt beim ERSTEN
//    Leerzeichen abgeschnitten: manche Quellen haengen dort einen Typbuchstaben an (z. B. "DG8DQ B" fuer
//    Ballon, "DB0BS C" fuer Chase-Car, sichtbar in der Web-Stationsliste als "DG8DQ  B"/"DB0BS  C") -- auf
//    Karte, Geraete-Stationsliste und /malt steht jetzt nur noch das eigentliche Rufzeichen davor.
//  - VERMUTLICHE URSACHE des R-Haengers (s. o.) gefunden und behoben, dank eines Logs ohne Haenger, das aber
//    eine Objektposition mit Laenge "inf" zeigte: aprsParsePosition()/aprsParseObject() lasen die Minuten-
//    Nachkommastelle bisher per atof(p) OHNE Laengenbegrenzung -- folgt auf die Laengenangabe zufaellig ein
//    'E' (Ost-Zeichen) direkt gefolgt von weiteren Ziffern (Kurs/Geschwindigkeit im Rest des Pakets), liest
//    die libc das als wissenschaftliche Notation ("12.345E123"), was durch Ueberlauf "inf" ergibt. Eine
//    Station mit nicht-endlicher Position als Punkt im Fokus "Ich + alle Stationen" (Taste R) haette ueber
//    mapViewSetup()/prepareMapBackground() vermutlich zu einer nicht-endlichen Kartenmitte und darueber zu
//    einer riesigen/haengenden Kachelschleife gefuehrt. Fix: neue aprsReadMin5() liest GENAU die 5 Zeichen
//    des Minutenfelds, dazu in beiden Parsern + in idleMapUpdate() eine zweite Sicherung (isfinite()+Bereich).
//    Die DIAG-R-Haenger-Checkpoints (s. o.) bleiben vorerst drin, falls doch noch eine andere Ursache existiert.
//  - BESTAETIGT: R-Haenger tritt seit obigem Fix nicht mehr auf. DIAG-R-Haenger-Checkpoints (focusDbgArm,
//    die zusaetzlichen logLine()-Aufrufe in focusCycleOrRecenter()/idleMapUpdate()) wieder entfernt --
//    weniger SD-Kartenschreibzugriffe im Betrieb.
//  - Im LoRa-Modus wird die Textansicht (dort nur sinnloser Sonden-Frequenzscan) beim Umschalten
//    (Space/Trackball-Klick, auch per Webseite /vspace) jetzt komplett uebersprungen -- advanceView()
//    zykelt in diesem Modus nur noch zwischen Karte und Hoehentrack/Stationen. Zusaetzlich zur Absicherung:
//    Beim Hochfahren im LoRa-Modus mit zuletzt gespeicherter Textansicht wird stattdessen die Karte geladen,
//    und ein Wechsel IN den LoRa-Modus (Menue I bzw. Web), waehrend gerade die Textansicht offen ist,
//    schaltet sofort auf die Karte um (radioApplyMode()).
//  - LoRa Upload (eigene Position, Menue A im LoRa-Modus) sendet jetzt dynamisch statt starr alle 60 s:
//    Intervall nach aktuellem Tempo (effSpeedKmh()) gestaffelt -- Stand/<=10 km/h alle 30 min, >10 km/h
//    alle 2 min, >50 km/h alle 30 s -- zusaetzlich sofort, sobald seit dem letzten Beacon eine bestimmte
//    Strecke zurueckgelegt wurde (haversineKm() gegen den bei jedem Sendeversuch nachgezogenen Anker), je
//    nachdem, was zuerst zutrifft. Spart Akku/Kanalbelegung im Stand, haelt die eigene Position fuer andere
//    bei hohem Tempo trotzdem aktuell. Siehe aprsBeaconTick().
//  - Die Wegstrecke-Ausloesung oben zusaetzlich nach Tempo gestaffelt: zu Fuss/<=10 km/h schon ab 100 m
//    (sonst wuerde ein Spaziergang bei 30-min-Takt viel zu selten senden), schneller (Rad/Auto, >10 km/h)
//    weiterhin ab 1 km -- APRS_BEACON_DIST_WALK_M/APRS_BEACON_DIST_FAST_M in aprsBeaconTick().
//  - BUG behoben: der eigene LoRa-Beacon (aprsSendBeacon()) wurde zwar erfolgreich ueber Funk gesendet
//    (Log zeigte "Status 0"), aber vermutlich nie vom iGate erkannt -- die Rohpayload hatte den 3-Byte-Header
//    0x3C/0xFF/0x01 vor der TNC2-Zeile gefehlt, den aprsStripHeader() beim EMPFANG abschneidet (verbreitete
//    LoRa-APRS-Konvention, u. a. vom OE5BPA-iGate erwartet) -- unser eigenes Paket war also nicht im selben
//    Format wie die empfangenen. Fix: aprsSendBeacon() stellt dem Paket jetzt denselben Header voran.
//  - BUG behoben: das Abschneiden des Objektnamens beim ERSTEN Leerzeichen (s. o., gegen den "DG8DQ B"/
//    "DB0BS C"-Typbuchstaben-Anhang) war zu grob -- Objekte, die das 9-Zeichen-Namensfeld fuer etwas mit
//    einem Leerzeichen MITTENDRIN nutzen (z. B. ein Relais "OV 438.225-" mit Frequenzangabe), wurden dadurch
//    auf den ersten Wortteil verstuemmelt ("OV" statt "OV 438.225-"). aprsParseObject() erkennt den
//    Typbuchstaben-Anhang jetzt nur noch am engen Muster "Leerzeichen + genau EIN Grossbuchstabe GENAU am
//    Namensende" und schneidet nur den ab -- alles andere bleibt unangetastet.
//  - Schriftgroesse der Stationsliste auf der Weboberflaeche (/menu, #altlist) verkleinert (12px statt
//    Browser-Standard), damit mehr Zeilen ohne Scrollen sichtbar sind.
//  - Karte im LoRa-Modus beim Fokus "Ich+alle Stationen"/"Ich+direkt gehoert" (Taste R, aprsFocus 1/2): "Ich"
//    bleibt jetzt IMMER im Bildzentrum, auch beim Reinzoomen -- bisher zentrierte mapViewSetup() bei mehreren
//    Punkten (np > 1) immer auf die Mitte der Bounding-Box aller Punkte (wie im Sondenmodus "Sonde+Ich"
//    gedacht), bei ungleichmaessig verteilten Stationen konnte das die eigene Position deutlich aus der Mitte
//    schieben. Neue fitZoomCentered() berechnet den Zoom stattdessen so, dass alle Punkte um einen FESTEN
//    Mittelpunkt (Ich) passen; mapViewSetup() bekommt dafuer den neuen Parameter centerOnFirst (true im
//    LoRa-Mehrpunkt-Fall, siehe idleMapUpdate()) und zentriert bei rerender dann auf plat[0]/plon[0] statt auf
//    die Bounding-Box-Mitte. Wirkt automatisch auch beim manuellen Reinzoomen (A/D), da enterManual() den
//    bereits Ich-zentrierten Ausschnitt als Startpunkt fuer den manuellen Modus uebernimmt.
//  - Hoehentrack-Flaechenfuellung am Geraet (altTrackUpdate()) an die Weboberflaeche angeglichen: neue Farbe
//    COL_ALT_FILL (dunkles Petrol 0x12EB, RGB565-Entsprechung von #145e5e aus loadAlt()) statt des bisherigen
//    COL_TRAIL (orange, eigentlich die Farbe der Sondenspur auf der Karte). Kurvenlinie (gelb) und aktuelle
//    Hoehe (weiss) waren schon identisch zur Website, jetzt sieht auch die Flaeche darunter gleich aus.
//  - Textansicht bei fester Frequenz (fixedFreq, Menue S) war bisher bis auf die Frequenzzeile leer (kein
//    Suchlauf -> drawSpectrum() wird nie aufgerufen). Neue drawFixedPanel() zeigt stattdessen in derselben
//    Flaeche einen Live-Pegelbalken (Momentan-RSSI bei der festen Frequenz, auch ohne decodiertes Paket)
//    sowie eine kleine Empfangsstatistik (Pakete gesamt, letzter Empfang mit Alter/Pegel, letzte Sonden-ID).
//  - Neuer Menuepunkt "Sendeleistung" (Taste U, MI_TXPWR): stellt die LoRa-APRS-Beacon-Sendeleistung
//    zwischen -9 und +22 dBm ein (SX1262-Hardwaregrenzen), Standard weiterhin 10 dBm wie bisher (RadioLibs
//    eigener Default). Wird in aprsSendBeacon() per radio.setOutputPower() direkt vor jedem Senden gesetzt
//    (radio.begin() setzt bei jedem Moduswechsel wieder den Default), ueber Preferences ("txpwr") gespeichert
//    und dank der generischen Menue-Architektur automatisch auch auf der Weboberflaeche (/menu) einstellbar.
//  - Das APRS-Symbol des eigenen LoRa-Beacons wechselt jetzt automatisch nach Tempo statt immer "Auto" (fest
//    codiertes '>') zu sein: Stationaer (Haus-Symbol) im Stand, ab 2 km/h Fussgaenger, ab 10 km/h Fahrrad, ab
//    30 km/h Bus (primaere Symboltabelle "/", die aprsEncodePosition() schon fest einbaut). Neue aprsSymbolTick()
//    (jeden Durchlauf aus handleInput() aufgerufen) bestimmt aus effSpeedKmh() die passende Stufe, schaltet aber
//    erst um, wenn diese mindestens APRS_SYMBOL_HYST_MS (5 Minuten) ununterbrochen anliegt -- sonst wuerde das
//    Symbol bei schwankendem Tempo (Ampel, Stop-and-go) staendig hin- und herspringen. aprsSendBeacon() setzt
//    das aktuelle Symbolzeichen jetzt direkt hinter die Positionsangabe ins Paket statt des alten festen '>'.
//  - Compile-Fehler behoben: drawFixedPanel() (s. o., v4.37) benutzte sondeRssi/sondeId/sondeFnr/lastRxMs,
//    die aber erst viel weiter unten im Sketch deklariert waren -- bei globalen Variablen gibt es (anders als
//    bei Funktionen) keine automatischen Prototypen, die Deklaration muss also vor der ersten Verwendung stehen.
//    Jetzt direkt vor drawFixedPanel() deklariert.
//  - Beim Start im LoRa-Modus (radioMode == 1), wenn der LoRa-Beacon beim letzten Mal aktiv war (loraTxWasOn,
//    neuer Prefs-Schluessel "loratx", mitgeschrieben in saveSettings()): neue confirmLoraTxResume() fragt
//    blockierend J/N ab, ob wieder gesendet werden soll -- loraTxOn selbst bleibt nach jedem Neustart weiterhin
//    bewusst aus (s. Deklaration), damit nicht unbeabsichtigt gesendet wird, aber so muss man nach einem
//    gewollten Neustart nicht jedesmal von Hand ins Menue. J aktiviert mit der eingestellten Sendeleistung
//    (txPowerDbm/MI_TXPWR, wird ohnehin vor jedem Senden neu angewendet), N laesst es aus und merkt sich das
//    (loraTxWasOn = false), damit beim naechsten Start nicht wieder gefragt wird.
//  - BUG behoben: Dateien im ZIP-Download (Screenshots/GPX/Logs, webZipSend()) standen unabhaengig vom
//    tatsaechlichen Zeitstempel immer auf dem festen Platzhalter 1980-01-01 -- der war im ZIP-Header schlicht
//    hartcodiert. Neue dosDateTime() wandelt den echten SD-Zeitstempel (FileInfo.t/ZipEntry.t, kommt aus
//    getLastWrite()) ins ZIP-Datumsformat um. Zusaetzlich bekamen neu angelegte GPX-Dateien (gpxOpenSegment())
//    und Screenshots (takeScreenshot()) bisher NIE einen Zeitstempel nachgetragen -- nur die Logdatei wurde
//    beim ersten GPS-Zeitfix per touchFileTime() korrigiert (siehe onTimeSet()). Jetzt touchFileTime() auch
//    direkt nach dem Anlegen von GPX-Datei und Screenshot, damit beide von Anfang an ein echtes Datum tragen.
//  - Diagnose fuer GPS-Empfangsausfaelle (0 Sat ueber laengere Zeit, auch an Stellen mit sonst gutem Empfang):
//    neue gsvUpdate()/gsvVisibleTotal() werten jetzt auch $--GSV-Saetze aus ("sichtbare" Satelliten je
//    Konstellation, unabhaengig vom Fix) statt sie wie bisher zu verwerfen. gps.sats (aus GGA) zaehlt nur die
//    beim FIX benutzten Satelliten und ist bei "kein Fix" immer 0 -- das Log zeigt ab jetzt zusaetzlich, wie
//    viele die Antenne ueberhaupt SIEHT ("t... GPS: kein Fix (N sichtbar)"): 0 sichtbar deutet auf Antenne/
//    Kabel/Sichtfeld hin, N > 0 sichtbar aber trotzdem kein Fix eher auf Mehrwegeempfang/Abschattung/Stoerung.
//  - confirmLoraTxResume() (LoRa-Sende-Startabfrage) deutlich groesser/klarer gestaltet: Titel jetzt in
//    Schriftgroesse 2, J und N nicht mehr untereinander sondern nebeneinander als zwei grosse, farblich
//    umrahmte Kaestchen (J gruen, N rot) mit riesigen Buchstaben (Schriftgroesse 5) und der Erklaerung je
//    darunter -- auf einen Blick erkennbar statt kleingedruckt.
//  - Die LoRa-Sende-Startabfrage kommt jetzt NICHT mehr bei einem Neustart durch Firmware-Upload (ArduinoOTA/
//    USB-Flash): esp_reset_reason() == ESP_RST_SW erkennt den reinen Software-Neustart (kein echter
//    Stromzyklus, Antennenkonstellation unveraendert) und aktiviert das Senden in dem Fall automatisch wieder
//    wie bei "J" -- die Abfrage bleibt weiterhin bestehen fuer jeden "echten" Neustart (Stromzyklus, Reset-
//    Taster, Brownout usw.), bei dem sich am Setup tatsaechlich etwas geaendert haben koennte.
//  - Stationaer-Symbol gewechselt: nicht mehr Haus ('-', feste QTH-Station) -- das Geraet ist dabei portabel
//    und nur gerade nicht in Bewegung, keine feste Station. Jetzt "/" (roter Punkt), die uebliche generische
//    Positionsmarkierung ohne Bewegungs- oder Gebaeude-Aussage. Ausserdem die Umschaltverzoegerung
//    APRS_SYMBOL_HYST_MS von 5 auf 2 Minuten verkuerzt -- das Symbol reagiert jetzt schneller auf echte
//    Tempoaenderungen, bleibt aber immer noch stabil gegen kurze Ampel-/Stop-and-go-Schwankungen.
//  - Beim Senden des eigenen Positions-Beacons (aprsSendBeacon()) piept es jetzt genauso wie beim Sondenfund,
//    mit derselben Einstellung (beepPlay(), Menue B/MI_BEEP): bei "aus" bleibt es stumm, sonst piept jeder
//    gesendete eigene Beacon einmal.
//  - Qualitaets-Gate fuer die eigene Position beim SondeHub-Listener-Upload UND beim LoRa-APRS-Beacon
//    umgestellt: statt nur stumpf die Satellitenanzahl zu zaehlen (bisher SondeHub: >6 Sat, LoRa: gar keine
//    Schranke), pruefen beide jetzt per neuer gpsFixGoodEnoughToSend() den HDOP (Satelliten-GEOMETRIE) -- die
//    Sat-Zahl bleibt nur noch als grobe Untergrenze (GPS_SEND_MIN_SATS = 4, das theoretische Minimum fuer
//    einen 3D-Fix), waehrend GPS_SEND_MAX_HDOP = 2.5 die eigentliche Guete entscheidet. Dafuer liefert
//    parseNmea() den HDOP jetzt zusaetzlich ins neue, global sichtbare Feld gps.hdop (vorher nur lokal in
//    parseNmea() ausgewertet). Grund: 5 gut verteilte Satelliten koennen einen genaueren Fix liefern als 7
//    eng zusammenstehende -- reine Anzahl ist ein schwacher Indikator fuer die tatsaechliche Positionsguete.
//  - Trackball-Sperre per Taste 0 (ohne Menue) cycelt jetzt mit Zeitfenster statt stumpfem Rundlauf: aus Stufe 1
//    ("an") fuehrt ein SCHNELLER zweiter Tipp (binnen TBLOCK_DOUBLE_TAP_MS = 2 s) gleich weiter auf Stufe 2
//    (an+Tastatur) -- steht Stufe 1 dagegen schon laenger, geht ein Tipp auf 0 wie bisher zuerst auf aus
//    zurueck. Aus Stufe 2 fuehrt weiterhin ausschliesslich Taste 0 direkt auf aus (unveraendert).
//  - Trackballklick sendet im LoRa-Modus mit eingeschaltetem Upload (Menue A) jetzt sofort die eigene Position
//    (bei brauchbarem GPS-Fix, s. gpsFixGoodEnoughToSend() -- sonst nur ein Hinweis-Toast), unabhaengig vom
//    Zeit-/Wegstrecke-Takt in aprsBeaconTick(), der unveraendert im Hintergrund weiterlaeuft. Ausserhalb von
//    LoRa-Modus+Upload wechselt ein Klick weiterhin wie bisher die Ansicht; die Leertaste wechselt immer die
//    Ansicht, unabhaengig von Modus/Upload.
//  - LoRa-APRS-Stationen liefern oft einen Freitext-Kommentar hinter Position+Symbol (Kurs/Tempo, Hoehe
//    "/A=......", oder einfach freier Text wie der eigene Beacon ihn anhaengt, siehe aprsSendBeacon()) --
//    bisher von aprsStoreFromInfo() komplett verworfen (aprsExtractComment()), jetzt je Station gemerkt
//    (aprsComment[]). In der Stationsliste (gfxView 1 im LoRa-Modus) waehlt der Trackball (hoch/runter)
//    jetzt eine Station aus (aprsListMoveSel(), bisher dort ungenutzt), ein Klick zeigt ihren Kommentar als
//    Popup an (wortumbrochen, wrapTextLines()), ein weiterer Klick blendet es wieder aus. Ohne Kommentar
//    oder Auswahl kommt stattdessen ein Hinweis-Toast. Fuer den Sofort-Senden-Klick (s. o.) aendert sich
//    nichts -- der bleibt der Kartenansicht vorbehalten, in der Stationsliste geht ein Klick jetzt an die
//    Zusatzinfo statt ans Senden.
//  - Stationsliste zeigt jetzt auch die Empfangsart vor dem Rufzeichen, farblich wie auf der Karte
//    (aprsRecvKind[], COL_APRS_*): D = direkt per Funk gehoert (blau), R = ueber einen Digipeater-Hop
//    weitergereicht (orange), I = Fremdverkehr ueber ein iGate/APRS-IS relayt (rot) -- bleibt auch in der
//    markierten (hervorgehobenen) Zeile farblich erhalten.
//  - Abgeschnittene Zusatzinfo-Texte behoben: aprsComment[] fasste bisher nur 47 Zeichen (APRS_COMMENT_LEN),
//    deutlich weniger, als Stationen in der Praxis an Freitext anhaengen -- jetzt 119 Zeichen. Der rohe
//    Empfangspuffer selbst (aprsPoll(), buf[256]) war nie die Grenze, das Kuerzen geschah ausschliesslich
//    beim Ablegen in aprsComment[] (aprsExtractComment()).
//  - Firmware-Version jetzt dauerhaft sichtbar statt nur kurz im Bootsplash: im Geraetemenue (drawMenu())
//    klein unter dem Statusblock oben rechts, und auf beiden Weboberflaechen (webIndex()/webMenu()) als
//    kleine Fusszeile ganz unten.
//
// Frame-Positionen (Offsets im Frame inkl. 8 Byte Header) nach rs1729/RS:
//   0x39 Statusblock (0x79), 0x3B Framenummer, 0x3D Sonden-ID (8 Zeichen),
//   GPS-Infoblock (0x7C) mit Woche/TOW, 0x114..0x11F ECEF X/Y/Z (cm)
//
// Bibliotheken: RadioLib 7.x, Adafruit GFX Library, Adafruit ST7735 and ST7789 Library,
//   JPEGDEC (Kartenkacheln, nur noch gepackte Bundles, siehe pack_tiles.py), JPEGENC (Screenshots),
//   SD/WiFi/WebServer (Teil des ESP32-Pakets)

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <utime.h>
#include <SD.h>
#include <Wire.h>              // I2C: Tastatur des T-Deck
#include <Preferences.h>      // Einstellungen dauerhaft speichern (Teil des ESP32-Pakets)
#include <JPEGDEC.h>         // Bibliotheksverwalter: "JPEGDEC" (Larry Bank), fuer Luftbild-Kacheln (.jpg)
#include <JPEGENC.h>         // Bibliotheksverwalter: "JPEGENC" (Larry Bank), fuer Screenshots (.jpg)
#include <WiFi.h>            // WLAN-Zugangspunkt zum Abrufen der Screenshots und Logs (Teil des ESP32-Pakets)
#include <WebServer.h>
#include <ESPmDNS.h>          // Name tdeck-rs41.local im Heimnetz, auch fuer die ArduinoOTA-Erkennung genutzt
#include <ArduinoOTA.h>       // Firmware-Update direkt aus der Arduino-IDE (Werkzeuge -> Port -> Netzwerkport)
                               // -- Teil des ESP32-Kernpakets, keine zusaetzliche Bibliothek noetig. Bewusst
                               // OHNE Passwort (siehe otaStart()): nur im eigenen WLAN (Zugangspunkt oder
                               // Heimnetz) erreichbar, und wer Hardware-Zugriff aufs T-Deck hat, braucht
                               // ohnehin keinen Netzwerk-Umweg.
#include <HTTPClient.h>       // fuer die SondeHub-Fundmeldung (Taste Q)
#include <NetworkClientSecure.h>
#include <esp_sleep.h>        // fuer das geordnete Herunterfahren (Menue Q)
#include <esp_heap_caps.h>    // heap_caps_malloc()/MALLOC_CAP_SPIRAM fuer den Hoehentrack-Puffer (siehe setup())
#include <esp_system.h>       // esp_reset_reason() -- Diagnose des OTA-Haengers in inputBegin()
#include "driver/gpio.h"      // gpio_reset_pin() -- setzt IO-Mux/Interrupt-Konfiguration einzelner Pins hart zurueck
#include <driver/i2s.h>       // Empfangston ueber den eingebauten Lautsprecher (Menue X)
#include <vector>
#include <algorithm>

// Muss vor der ersten Funktion stehen: die Arduino IDE erzeugt die Funktionsprototypen automatisch am Dateianfang
struct FileInfo { String name; uint32_t size; time_t t; };
// Eine Datei im ZIP-Download (webZipSend): path ist der volle SD-Pfad zum Lesen, arc der Name, unter dem
// sie im ZIP erscheint (bei einer gemischten Auswahl ueber mehrere Ordner hinweg der Dateiname ohne Pfad).
// t ist der Zeitstempel der SD-Datei (getLastWrite(), 0 = unbekannt/noch nicht gesetzt) fuer das ZIP-Datum,
// siehe dosDateTime() in webZipSend().
struct ZipEntry { String path; String arc; uint32_t size; time_t t = 0; };
// Ebenso vor die erste Funktion: die Arduino-IDE erzeugt Funktionsprototypen, die BundleIdx/BundlePart
// als Referenzparameter nennen, bevor die Struct-Definitionen weiter unten im Sketch erreicht wuerden.
struct BundleIdx { File tbi; bool ok = false; int style = -2, zoom = -2; uint32_t count = 0; uint8_t ext = 0; };
struct BundlePart { File tb; int style = -2, zoom = -2, part = -2; };
struct GpxTrack { File f; bool ok = false; bool failed = false; uint32_t insertOffset = 0; };
struct GpxPoint { float lat, lon, alt; time_t t; uint16_t ms; };   // t = 0, wenn beim Einreihen keine GPS-Uhrzeit vorlag;
                                                                    // ms = Millisekundenanteil (sonst faellt bei mehreren
                                                                    // Punkten/Sekunde -- z. B. 2-Hz-GPS -- die Zeit auf die
                                                                    // gleiche volle Sekunde, und jede Auswertung errechnet
                                                                    // daraus abwechselnd 0 km/h und Ausreisser nach oben)
// Vorwaertsdeklarationen: trailAdd() (weiter unten definiert) ruft diese GPX-Funktionen auf, die erst
// spaeter im Sketch folgen -- explizit hier deklariert, um nicht von der automatischen Prototyp-Erzeugung
// der Arduino-IDE abhaengig zu sein (die bei Struct-Parametern schon zweimal Probleme gemacht hat).
static void gpxAddPointRaw(GpxTrack &t, const char *idStr, const char *trkName, double lat, double lon, float alt, const char *timeStr);
static void gpxCloseTrack(GpxTrack &t);
static void gpxQueue(GpxTrack &t, GpxPoint *buf, int &count, const char *idStr, const char *trkName, double lat, double lon, float alt);
static void gpxDrain(GpxTrack &t, GpxPoint *buf, int &count, const char *idStr, const char *trkName);
static bool gpxEnsureIchId();
static GpxTrack gpxIch, gpxSonde;   // muss hier stehen: die Arduino-IDE hebt nur Funktions-, keine Variablendeklarationen nach oben
static char gpxSondeId[12] = "";        // ID, unter der die aktuell offene(n)/gepufferten Sonde-Punkte abgelegt werden
static char gpxIchId[20] = "";          // Datum+Uhrzeit (JJJJ-MM-TT_hhmmss), unter dem die "Ich"-Punkte dieser Sitzung abgelegt werden
static char gpxSondeName[24] = "?";      // <name> im GPX-Track: die Sonden-ID, sobald sie bekannt ist
static char gpxIchName[40] = "?";        // <name> im GPX-Track: Startdatum/-uhrzeit dieser Sitzung
static char gpxRecoveredBy[40] = "";    // Melder-Name/Rufzeichen fuer SondeHub-Fundmeldungen (Taste Q), hier weil schon loadSettings() darauf zugreift
#define GPX_BUF_CAP  300         // PSRAM-Zwischenspeicher je Spur (Punkte), bevor spaetestens auf die SD geschrieben wird
static GpxPoint *gpxBufIch = nullptr, *gpxBufSonde = nullptr;
static int gpxCountIch = 0, gpxCountSonde = 0;
static uint32_t gpxLastFlushMs = 0;

// Serielle Ausgabe ueber USB-C braucht in der Arduino IDE: Werkzeuge > USB CDC On Boot > Enabled
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 0)
#warning "USB CDC On Boot ist deaktiviert: Serial geht NICHT ueber USB-C. Einstellung auf Enabled aendern."
#endif

// ---------- Pins T-Deck / T-Deck Plus ----------
#define PIN_POWER_ON   10
#define PIN_SCK        40
#define PIN_MISO       38
#define PIN_MOSI       41
#define PIN_TFT_CS     12
#define PIN_TFT_DC     11
#define PIN_TFT_BL     42
#define PIN_SD_CS      39
#define PIN_LORA_CS     9
#define PIN_LORA_RST   17
#define PIN_LORA_BUSY  13
#define PIN_LORA_DIO1  45

#define PIN_I2S_BCK     7                    // Lautsprecher (I2S), laut offiziellem T-Deck-Pinout
#define PIN_I2S_WS      5
#define PIN_I2S_DOUT    6

// ---------- Einstellungen ----------
#define TFT_INVERT     true

// ---------- Farben der Kartenmarker (RGB565) ----------
// Eigene Farben: 0xRRRR-Wert = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3)
#define COL_SONDE_OK      ST77XX_GREEN    // Sonde: gueltige Position vor < 4 s (Fuellfarbe, s. lastPosOkMs)
#define COL_SONDE_LATE    ST77XX_YELLOW   // Sonde: 4..19 s keine gueltige Position
#define COL_SONDE_LOST    ST77XX_RED      // Sonde: 20 s oder laenger keine gueltige Position
#define COL_SONDE_RING    ST77XX_BLACK    // Ring um den Sondenpunkt (normal)
#define COL_SONDE_RING_SIG ST77XX_GREEN   // Ring kurz gruen: Schritt 1 (Header) gerade empfangen, aber (noch) keine
                                           // gueltige Position in diesem Takt -- s. lastRxMs/SONDE_SIGNAL_FLASH_MS
#define SONDE_SIGNAL_FLASH_MS 1000UL       // so lange blinkt der Ring nach einem blossen Signal-Empfang (ohne Position)
#define COL_ICH_FILL      0x07FF          // ich: Fuellung cyan (0, 255, 255) -- gut sichtbar auf allen Kartenarten,
                                          // frueher lila und kaum vom violetten Radwegenetz der OpenCycleMap zu unterscheiden
#define COL_ICH_RING      ST77XX_BLACK    // ich: Ring
#define COL_LINE          ST77XX_RED      // Linie zwischen mir und der Sonde (2 Pixel breit)
#define COL_OWN_TRAIL     0xF81F          // eigener Track, magenta (255, 0, 255) -- frueher violett, verschmolz mit dem
                                          // violetten Radwegenetz der OpenCycleMap und mit der (jetzt cyanen) Ich-Fuellung
#define COL_TRAIL         0xFC60          // Spur der Sonde (orange, 255/140/0)
#define COL_PRED          0x041F          // Landevorhersage: Verlaengerung der Spur + Fadenkreuz (blau, 0/128/255)
#define COL_PRED_ONLINE   ST77XX_YELLOW   // Online-Landevorhersage (Tawhiri/SondeHub-API): nur Fadenkreuz, gelb
#define CROSS_HALF        6               // Fadenkreuz Landepunkt: halbe Linienlaenge in Pixeln
#define COL_RADAR_RING    0x0400          // Radar: Ringe und Kreuz (dunkelgruen)
#define COL_RADAR_TEXT    0x8410          // Radar: Beschriftungen (grau)
#define COL_ALT_FILL      0x12EB          // Hoehentrack: Flaechenfuellung unter der Kurve (dunkles Petrol, #145e5e --
                                          // RGB565-Entsprechung derselben Farbe wie im Chart der Weboberflaeche, siehe
                                          // loadAlt()/ctx.fillStyle in webMenu(), damit Geraet und Website gleich aussehen)
// LoRa-APRS-Stationspunkte (drawAprsStations()), Farbe je nach Empfangsart (siehe aprsRecvKind[]):
#define COL_APRS_DIRECT   0x001F          // direkt per Funk gehoert (blau, 0/0/255)
#define COL_APRS_IGATE    ST77XX_RED      // als Fremdverkehr ueber ein iGate/APRS-IS relayt ("}"-Praefix, rot)
#define COL_APRS_DIGI     0xFC60          // ueber einen genutzten ("*"-markierten) Digipeater-Hop weitergereicht
                                          // (orange, 255/140/0 -- bewusst = COL_TRAIL, dieselbe Farbe wie die Sondenspur)
#define VERBOSE_DUMP   0       // 1 = Rohdaten/entwuerfelte Frames als Hex seriell ausgeben (lange Ausgabe), 0 = aus
#define PIN_BAT_ADC    4       // Batteriespannung ueber Spannungsteiler 1:2
const float BAT_SCALE  = 2.0f;         // Teilerfaktor
const float BAT_OFFSET = 0.0f;         // Korrektur in Volt (mit Multimeter abgleichen; die gemessene Spannung steht in der Logzeile)
const float BAT_USB_V  = 4.30f;        // ab dieser gemessenen Spannung gilt USB/Ladespannung als angesteckt
#define TZ_RULE        "CET-1CEST,M3.5.0,M10.5.0/3"   // Ortszeit fuer Dateizeiten und Logzeilen (Mitteleuropa mit Sommerzeit)
#define WIFI_SSID      "T-Deck-RS41"           // WLAN-Zugangspunkt des T-Deck (Menue W)
#define WIFI_PASS      "sonde433"              // mindestens 8 Zeichen, bitte aendern
#define SHOT_DIR       "/shots"                // Screenshots (Taste C)
#define GPX_DIR        "/gpx"                  // GPX-Tracks (eigene Position + Sonde)
#define LOG_DIR        "/log"                  // Text-Logs (an/aus im Menue P; je Neustart ein frisches Log)
#define WIFI_HOST      "tdeck-rs41"            // Rechnername im Heimnetz: http://tdeck-rs41.local
#define WIFI_LOST_MS   10000                   // so lange darf die Heimnetz-Verbindung weg sein (z. B. kurzer Aussetzer),
                                                // bevor das WLAN als "Netz verlassen" gilt und abgeschaltet wird
#define WIFI_IDLE_OFF_MS (5UL * 60UL * 1000UL)  // eigener Zugangspunkt (immer nur manuell im Menue W gestartet, NIE
                                                // automatisch): geht nach so langer Zeit ohne verbundenes Geraet
                                                // von selbst wieder aus (spart Akku). Gilt nicht fuer das Heimnetz --
                                                // dort haengt das T-Deck ohnehin am Strom, WLAN bleibt an, solange
                                                // die Verbindung steht (siehe WIFI_LOST_MS oben).
#define TITLE_TEXT     "RS41 | Enter=Menue  Space=Ansicht"
// ===========================================================================
// PERSOENLICHE EINSTELLUNGEN (Rufzeichen, Heimat-Koordinaten): siehe config.h
// Vor dem ersten Compilieren: config.h.example nach config.h kopieren und dort
// die eigenen Werte eintragen. config.h selbst ist in .gitignore eingetragen
// und wird NICHT auf GitHub hochgeladen -- so bleibt dieses .ino hier fuer
// jeden gleich, nur config.h ist persoenlich.
// ===========================================================================
#include "config.h"              // definiert FW_CALLSIGN, HOME_LAT, HOME_LON
#define FW_VERSION     "4.68"  // von Hand hochzaehlen, wenn eine Aenderung es wert ist -- Compile-Datum/-Zeit
                                // (siehe bootSplash()) kommt ohnehin automatisch vom Compiler (__DATE__/__TIME__)
// Tastatur (I2C) und Trackball des T-Deck
#define PIN_KB_SDA     18
#define PIN_KB_SCL     8
#define KB_ADDR        0x55
#define PIN_TB_UP      3
#define PIN_TB_DOWN    15
#define PIN_TB_LEFT    1
#define PIN_TB_RIGHT   2
#define PIN_TB_CLICK   0
#define KB_CMD_BRIGHT  0x01    // Tastatur-Controller (ESP32-C3): 2-Byte-Befehl 0x01 + Helligkeit 0..255 setzt das Tastenlicht
#define KB_LEVEL       120     // Helligkeit des Tastenlichts (0..255)
#define KB_LEVEL_DIM    25     // Modus 2 "Taste+dunkel": gedimmte Grundhelligkeit, solange keine Taste gedrueckt wird
#define KB_ON_MS       6000    // so lange leuchten die Tasten nach dem letzten Tastendruck
#define KB_BRIGHT_MS   4000    // Modus 2: so lange bleibt es nach einem Tastendruck hell, bevor es wieder dimmt
#define KB_DARK_ELEV   (-1.0)  // "dunkel" = Sonne tiefer als dieser Winkel unter dem Horizont [Grad]
#define TB_PULSES      2       // so viele Trackball-Impulse gelten als ein Schritt (bei Bedarf anpassen)
// Kein Mindest-Impulsschwellwert fuers Scrollen mehr (Okt. 2026): der sollte vor Ausloesen durch Stoesse/
// Vibration in der Autohalterung schuetzen, waehrend der Fahrt wird der Trackball aber ohnehin per Taste 0
// gesperrt (tbLockMode) -- jeder einzelne Trackball-Impuls scrollt die Karte jetzt sofort (siehe mapNavTick()).
#define NTP_SERVER     "pool.ntp.org"   // fuer die Uhrzeit beim Einschalten, solange noch kein GPS-Fix vorliegt
#define NTP_TIMEOUT_MS 8000    // so lange wird nach dem Verbinden auf eine gueltige NTP-Antwort gewartet, dann aufgegeben
#define LOG_LEVEL      0       // 0 = eine Zeile pro Frame, 1 = zusaetzlich alle Blockdetails
// Keine Platz-Rotation mehr: jeder Neustart beginnt ein frisches Log ohne eigene Groessenbegrenzung, benannt
// nach Datum/Uhrzeit, sobald GPS/NTP eine Uhrzeit liefert (siehe startLog()/onTimeSet()). Bis dahin (und wenn
// in einer Sitzung gar keine Uhrzeit zu bekommen ist) bekommt es stattdessen eine Platzhalter-Nummer
// 01..LOG_FALLBACK_MAX (die erste noch freie, einfach hochgezaehlt, nichts wird dabei ueberschrieben; nur im
// unrealistischen Fall, dass wirklich alle Nummern bis LOG_FALLBACK_MAX belegt sind, faengt es wieder bei 01 an).
#define LOG_FALLBACK_MAX 99
#define SD_LOG         1       // 1 = Zeile pro Frame zusaetzlich auf die SD-Karte schreiben (/log/rs41_log_*.txt)
#define VLOG(...)   do { if (LOG_LEVEL >= 1) Serial.printf(__VA_ARGS__); } while (0)
#define VLOGLN(s)   do { if (LOG_LEVEL >= 1) Serial.println(s); } while (0)
const float TCXO_V     = 1.8;
const float SCAN_START = 400.0;
const float SCAN_END   = 406.0;
const float BITRATE    = 4.8;
const float FREQ_DEV   = 2.4;
const float RX_BW_DEFAULT = 23.4;       // Standard-Bandbreite fuer Feinscan und Empfang (kHz), im Menue (B) einstellbar

// LoRa-APRS-Empfang (Menue I schaltet zwischen Sondenempfang und diesem Modus um, siehe radioApplyMode()).
// Parameter nach der europaeischen LoRa-APRS-Konvention (433.775 MHz / SF12 / BW125 / CR4-5). Der 3-Byte-
// Header 0x3C/0xFF/0x01 vor der eigentlichen TNC2-Textzeile (siehe aprsStripHeader()) sowie das Sync-Wort
// 0x12 sind eine verbreitete Konvention einzelner LoRa-APRS-Implementierungen (nicht Teil des offiziellen
// APRS-Protokolls) -- noch nicht an echtem Empfang getestet, siehe Versionshinweis am Dateianfang.
const float APRS_FREQ_MHZ = 433.775;
const float APRS_BW_KHZ   = 125.0;
#define APRS_SF        12
#define APRS_CR        5
#define APRS_SYNCWORD  0x12
#define APRS_PREAMBLE  8

// Zweistufiger Scan: erst grob mit sehr breitem Filter, dann fein nur um die Kandidaten
const float COARSE_STEP = 0.2;          // MHz
#define NCOARSE 31                      // (406-400)/0.2 + 1
const float COARSE_BW   = 467.0;        // kHz, breiter Filter = wenige Schritte noetig
const float FINE_SPAN   = 0.12;         // MHz links/rechts vom Kandidaten
#define FINE_PASSES 4                   // Feinscan-Durchlaeufe (Maximum je Frequenz); die Sonde sendet nicht durchgehend
const float BAND_MIN    = 400.15;       // Wettersonden-Band beginnt bei 400,15 MHz; darunter werden keine Kandidaten gewaehlt
const float FINE_STEP   = 0.010;        // MHz
#define MAX_CAND 4                      // so viele Kandidaten werden nacheinander geprueft
const float REL_MARGIN  = 4.0;          // dB ueber dem Median (Rauschen) gilt im Grobscan als Signal
#define SCAN_SWEEPS 5                   // Grobscan wird gemittelt ueber so viele Durchlaeufe (laenger = empfindlicher)
const float FINE_MARGIN = 6.0;          // Feinscan: Peak muss so viele dB ueber dem Median liegen, sonst kein Decodierversuch
#define ACQ_MS   1350                   // Decodierversuch pro Kandidat (RS41 sendet 1 Frame/s)
#define LOST_MS  5000                   // so lange ohne Frame, dann zurueck zum Scan
#define STICKY_MS (20UL * 1000UL)       // nach einer decodierten Sonde: so lange noch gezielt auf ihrer letzten
                                         // Frequenz weiterlauschen (kein voller Suchlauf), bevor wirklich aufgegeben
                                         // wird -- ein Grobscan ueber 400-406 MHz dauert mehrere Sekunden, in denen
                                         // nichts empfangen wird (macht den Sondenton unregelmaessig), waehrend ein
                                         // kurzer Empfangsausfall (Signalschwund, Antennendrehung der Sonde) meist
                                         // nur Sekunden dauert, die Sonde selbst aber noch da ist
#define SV_MIN  4                       // Sondenposition erst ab so vielen Satelliten (sonst kommt (0,0,0) = "kein Fix")
#define SETTLE_US 1000                  // Einschwingzeit vor der RSSI-Messung

#define FRAME_LEN  320      // Laenge eines normalen RS41-Frames
#define CAP1_LEN   180      // Schritt 1: Bytes nach dem Header (Frame-Pos. 8..187)
// Schritt 2 (im NAECHSTEN Frame): Sync auf ID/Laenge des Blocks 0x7D bei Frame-Position 0xB5
// (bei dieser Sonde per Log bestaetigt), danach der Rest des Frames bis Position 319.
#define SYNC2_POS  0xB5
#define SYNC2_ID   0x7D
#define SYNC2_LEN  0x59
#define CAP2_LEN   (FRAME_LEN - (SYNC2_POS + 2))   // 137 Bytes

// Sync-Woerter, so wie sie der SX1262 sieht (MSB-first, also bitgedreht gegenueber dem Frame)
const uint8_t SYNC_HDR[8] = {0x08, 0x6D, 0x53, 0x88, 0x44, 0x69, 0x48, 0x1F};
uint8_t syncPos[2];   // Sync fuer Schritt 2, wird im setup() aus SYNC2_POS/SYNC2_ID/SYNC2_LEN berechnet

// XOR-Maske (64 Byte), quelle: rs1729/RS bzw. radiosonde_auto_rx
const uint8_t MASK[64] = {
  0x96, 0x83, 0x3E, 0x51, 0xB1, 0x49, 0x08, 0x98,
  0x32, 0x05, 0x59, 0x0E, 0xF9, 0x44, 0xC6, 0x26,
  0x21, 0x60, 0xC2, 0xEA, 0x79, 0x5D, 0x6D, 0xA1,
  0x54, 0x69, 0x47, 0x0C, 0xDC, 0xE8, 0x5C, 0xF1,
  0xF7, 0x76, 0x82, 0x7F, 0x07, 0x99, 0xA2, 0x2C,
  0x93, 0x7C, 0x30, 0x63, 0xF5, 0x10, 0x2E, 0x61,
  0xD0, 0xBC, 0xB4, 0xB6, 0x06, 0xAA, 0xF4, 0x23,
  0x78, 0x6E, 0x3B, 0xAE, 0xBF, 0x7B, 0x4C, 0xC1
};

SX1262 radio = new Module(PIN_LORA_CS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY, SPI,
                          SPISettings(8000000, MSBFIRST, SPI_MODE0));
Adafruit_ST7789 tft(&SPI, PIN_TFT_CS, PIN_TFT_DC, -1);

volatile bool rxFlag = false;
void IRAM_ATTR onRx() { rxFlag = true; }

float spec[NCOARSE];
float candF[MAX_CAND], candR[MAX_CAND];
bool isCand[NCOARSE];
uint8_t raw1[CAP1_LEN];
uint8_t raw2[CAP2_LEN];
uint8_t fr[FRAME_LEN];          // entwuerfelter Frame, Index = Frame-Position
uint32_t frameCount = 0;
bool mapMode = false;          // true = Kartenansicht (Text/Spektrum werden nicht gezeichnet)

// ---------- Einstellungen zur Laufzeit (Konfigurationsmenue, werden gespeichert) ----------
static bool fixedFreq = false;          // false = Suchlauf automatisch, true = feste Frequenz
static float freqMHz = 405.300f;        // feste Frequenz
static float lastScanFreq = 0;          // letzte im Suchlauf versuchte Kandidatenfrequenz (0 = noch keine in dieser Sitzung)
static float lastRxFreq = 0;            // Frequenz, auf der zuletzt tatsaechlich ein RS41-Frame empfangen wurde
static bool predOn = true;              // Landevorhersage an/aus
static double landAlt = 100.0;          // Landehoehe fuer die Vorhersage (Startwert LAND_ALT_DEFAULT)
static bool ownTrailOn = true;          // eigenen Track (GPS des T-Deck) in der Karte zeichnen (Menue O)
static uint8_t beepSec = 0;             // Empfangston (Menue X): 0 aus, 1 bei jedem gueltigen Paket, 5..20 alle N Sekunden waehrend des Empfangs
static uint8_t beepSecMute = 5;         // fuer die $-Taste (Ton direkt an/aus): der Wert, zu dem "an" zurueckkehrt
static uint8_t panPx = 5;               // Scroll-Empfindlichkeit: Kartenpixel pro Trackball-Impuls (Menue G)
static uint8_t tbLockMode = 0;          // Trackball-Sperre (Menue R, gegen Vibration/Stoesse): 0 aus, 1 an (nur Rollkugel
                                         // ignoriert), 2 an + Tastatursperre (dann wirkt in handleInput() nur noch Taste 0 --
                                         // einzige Moeglichkeit, aus Stufe 2 wieder herauszukommen)
static uint32_t tbLock1SetMs = 0;       // Zeitpunkt, seit dem Taste 0 zuletzt auf Stufe 1 ("an") gewechselt hat --
                                         // fuer den Schnell-Doppeltipp aus/an/an+Tastatur in handleInput(), siehe dort
static uint8_t brightPct = 100;         // Displayhelligkeit in Prozent, 10..100 (nie ganz aus)
static uint8_t kbMode = 2;              // Tastenlicht: 0 aus, 1 bei Tastendruck, 2 bei Tastendruck nur wenn es dunkel ist
static bool kbLit = false;              // Tastenlicht ist gerade an (Modus 2: auch im gedimmten Zustand true)
static uint32_t kbBrightUntil = 0;      // Modus 2: bis wann hell nach einem Tastendruck, danach zurueck auf KB_LEVEL_DIM (0 = nicht aktiv)
static uint32_t kbOffAt = 0;            // Zeitpunkt, an dem es wieder ausgeht
#define NSTYLE 5
static uint8_t mapStyle = 0;            // Kartenart: 0 Rad (/tiles), 1 Luftbild (/tiles_sat), 2 Topo (/tiles_topo), 3 Relief (/tiles_relief), 4 TopPlusOpen (/tiles_topplus)
static bool styleAvail[NSTYLE] = {true, false, false, false, false};   // welche Kartenarten auf der SD-Karte vorhanden sind
static bool wifiOn = false;             // Webserver laeuft (Heimnetz verbunden oder Zugangspunkt), beim Start immer aus
static uint8_t wifiMode = 0;            // 0 aus, 1 Heimnetz (Station), 2 eigener Zugangspunkt (Menue W)
static bool wifiConnecting = false;     // Verbindung zum Heimnetz wird aufgebaut
static uint32_t wifiActivityMs = 0;     // eigener Zugangspunkt: Zeitpunkt des letzten verbundenen Geraets -- siehe wifiIdleTick()
static uint32_t wifiConnMs = 0;
static bool ntpWanted = false;          // beim Einschalten: Uhrzeit per NTP holen, solange noch kein GPS-Fix vorliegt
static bool ntpConfigured = false;      // configTime() wurde fuer die aktuelle WLAN-Verbindung schon aufgerufen
static uint32_t ntpStartMs = 0;
static char wifiSsid[33] = "";          // gespeichertes Heimnetz (Menue E)
static char wifiPass[64] = "";
static uint32_t shotCount = 0;          // Zaehler fuer Screenshot-Namen ohne Uhrzeit
static uint8_t savedView = 0;           // zuletzt benutzte Ansicht: 0 Text, 1 Karte, 2 Hoehentrack/Stationen, 3 GPS-Radar (wird beim Start wiederhergestellt)
static float rxBw = 23.4f;                // Empfangsbandbreite [kHz] (Menue)
static bool fixOn = true;                 // 1-Bit-Fehlerkorrektur der Bloecke (Menue)
static bool filterOn = false;             // nur eine Sonden-ID akzeptieren (Menue)
static char filterId[9] = "";             // die erlaubte Sonden-ID
static float okF = 0;                     // Frequenz, auf der die Filter-ID zuletzt bestaetigt wurde
static float foreignF = 0;                // Frequenz einer fremden Sonde
static uint32_t foreignUntil = 0;         // bis dahin wird diese Frequenz beim Suchlauf uebersprungen
// letzte gueltige Sondenposition fuer den Plausibilitaetstest
static bool prevPosValid = false;
static double prevPosLat = 0, prevPosLon = 0;
static uint32_t prevPosMs = 0;
static char prevPosId[9] = "";
static bool sdLogOn = SD_LOG;           // Log auf SD-Karte
static uint8_t sondehubMode = 0;        // SondeHub-Chase-Car-Upload (Menue A): 0 aus, 1 an -- Sonden-Telemetrie
                                         // wird seit v2.44 gar nicht mehr hochgeladen, siehe dort
                                         // (muss hier stehen, vor loadSettings() -- siehe SondeHub-Block weiter unten)
static volatile bool restartRx = false; // Empfangsschleife neu starten (Frequenz/Suchmodus geaendert)
static uint8_t radioMode = 0;           // 0 = Sondenempfang (RS41, FSK), 1 = LoRa-APRS-Empfang (Menue I)
static volatile bool radioModeChanged = false;  // Menue hat radioMode geaendert -- naechste loop()-Runde wendet es an
// Menue A ("SondeHub-Upload") ist im LoRa-Modus umgewidmet zu "LoRa Upload": eigene Position per LoRa-APRS
// senden (nur ueber das Funkmodul, keine Internetverbindung -- der SondeHub-Upload selbst/sondehubMode
// bleibt davon unberuehrt und steht bei Rueckwechsel in den Sondenempfang wieder genauso da wie vorher).
// loraTxOn selbst steht nach jedem Moduswechsel und jedem Neustart erst mal auf aus -- es soll nicht
// unbeabsichtigt gesendet werden, nur weil es beim letzten Mal eingeschaltet war. loraTxWasOn (siehe
// loadSettings()/saveSettings(), Prefs-Schluessel "loratx") merkt sich aber, ob es das WAR, und confirmLoraTxResume()
// fragt damit beim Start im LoRa-Modus nach, ob wieder gesendet werden soll (siehe setup()).
static bool loraTxOn = false;
static bool loraTxWasOn = false;
static uint32_t aprsBeaconNextMs = 0;
static double aprsBeaconLastLat = 0, aprsBeaconLastLon = 0;   // Position beim letzten Beacon-Sendeversuch (fuer die
static bool aprsBeaconLastSet = false;                        // 1-km-Wegstrecke-Ausloesung, siehe aprsBeaconTick())
// Sendeleistung fuer den LoRa-APRS-Beacon (Menue U, siehe menuValue()/menuStep() MI_TXPWR), in aprsSendBeacon()
// direkt vor jedem radio.transmit() angewendet. SX1262-Bereich -9..+22 dBm; Standardwert 10 dBm entspricht
// RadioLibs eigenem Default (radio.begin() ohne power-Parameter) -- also unveraendertes Verhalten, bis jemand
// am Menuepunkt dreht.
static int8_t txPowerDbm = 10;
// APRS-Symbol fuer den eigenen Beacon: wird automatisch aus dem aktuellen GPS-Tempo abgeleitet (siehe
// aprsSymbolTick()), keine manuelle Einstellung. Primaere Symboltabelle "/", die aprsEncodePosition() schon fest
// einbaut. Index 0..3 in aprsSymbolChars[]/aprsSymbolNames[] -- absteigend nach Tempo gestaffelt, damit sich der
// jeweilige Schwellenwert direkt neben dem passenden Symbol findet.
#define NSYMBOL 4
// Index 0 ("Stationaer") bewusst NICHT das Haus-Symbol ('-', feste QTH-Station) -- das Geraet ist dabei
// portabel und nur gerade nicht in Bewegung (z. B. beim Peilen/Suchen zu Fuss am Fundort), keine feste
// Station. "/" (roter Punkt, primaere Symboltabelle) ist der uebliche generische Platzhalter fuer genau
// diesen Fall: aktuelle Position ohne Bewegungsaussage, ohne ein Gebaeude zu suggerieren.
static const char aprsSymbolChars[NSYMBOL]       = {'/',            '[',            'b',        'v'};
static const char *const aprsSymbolNames[NSYMBOL] = {"Stationaer",   "Fussgaenger",  "Fahrrad",  "Bus"};
static const float aprsSymbolMinKmh[NSYMBOL]      = {0.0f,           2.0f,           10.0f,      30.0f};
static uint8_t aprsSymbolIdx = 0;          // aktuell AKTIVES Symbol (erst nach Hysterese umgeschaltet)
static uint8_t aprsSymbolCand = 0;         // aus dem Tempo gerade berechnete Zielstufe
static uint32_t aprsSymbolCandSinceMs = 0; // seit wann aprsSymbolCand ununterbrochen gilt (0 = gerade erst gewechselt)
Preferences prefs;

// ---------- Hilfsfunktionen ----------
static uint8_t rev8(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

// CRC16-CCITT, Startwert 0xFFFF, Polynom 0x1021 (nach meiner Erinnerung wie bei rs1729)
static uint16_t crc16(const uint8_t *d, int n) {
  uint16_t c = 0xFFFF;
  for (int i = 0; i < n; i++) {
    c ^= (uint16_t)d[i] << 8;
    for (int k = 0; k < 8; k++) c = (c & 0x8000) ? (c << 1) ^ 0x1021 : (c << 1);
  }
  return c;
}

static void unscramble(const uint8_t *raw, int n, int pos0) {
  for (int i = 0; i < n; i++) fr[pos0 + i] = rev8(raw[i]) ^ MASK[(pos0 + i) & 63];
}

// Block bei Position p: [ID][Laenge][Daten...][CRC lo][CRC hi]; CRC ueber die Daten
static bool blockCrc(int p) {
  int len = fr[p + 1];
  if (p + 3 + len >= FRAME_LEN) return false;
  uint16_t c = fr[p + 2 + len] | ((uint16_t)fr[p + 3 + len] << 8);
  return crc16(&fr[p + 2], len) == c;
}

// CRC pruefen; bei Fehler ein einzelnes falsches Bit (Daten oder CRC) suchen und umdrehen.
// Rueckgabe: 0 = falsch, 1 = ok, 2 = per 1-Bit-Korrektur repariert. (CRC16 erkennt jede 1-Bit-Fehlerstelle eindeutig.)
static int blockCrcFix(int p) {
  if (blockCrc(p)) return 1;
  if (!fixOn) return 0;
  int len = fr[p + 1];
  if (len > 100 || p + 3 + len >= FRAME_LEN) return 0;
  int nbytes = len + 2;                                  // Daten + CRC
  for (int i = 0; i < nbytes; i++) {
    for (int b = 0; b < 8; b++) {
      fr[p + 2 + i] ^= (uint8_t)(1 << b);
      if (blockCrc(p)) return 2;
      fr[p + 2 + i] ^= (uint8_t)(1 << b);
    }
  }
  return 0;
}

static int16_t le16(int p) {
  return (int16_t)((uint16_t)fr[p] | (uint16_t)fr[p + 1] << 8);
}

static int32_t le32(int p) {
  return (int32_t)((uint32_t)fr[p] | (uint32_t)fr[p + 1] << 8 |
                   (uint32_t)fr[p + 2] << 16 | (uint32_t)fr[p + 3] << 24);
}

void ecef2lla(double x, double y, double z, double &lat, double &lon, double &alt) {
  const double a = 6378137.0, fl = 1.0 / 298.257223563, e2 = fl * (2.0 - fl);
  lon = atan2(y, x);
  double p = sqrt(x * x + y * y);
  lat = atan2(z, p * (1.0 - e2));
  alt = 0;
  for (int i = 0; i < 6; i++) {
    double s = sin(lat);
    double N = a / sqrt(1.0 - e2 * s * s);
    alt = p / cos(lat) - N;
    lat = atan2(z, p * (1.0 - e2 * N / (N + alt)));
  }
  lat *= 180.0 / M_PI;
  lon *= 180.0 / M_PI;
}

// Hex-Dump auf Serial, Adressen = Frame-Position
void hexDump(const char *label, const uint8_t *d, int n, int startPos) {
  if (!VERBOSE_DUMP) return;
  Serial.printf("%s (%d Bytes)\n", label, n);
  for (int i = 0; i < n; i += 16) {
    Serial.printf("  %03X:", startPos + i);
    for (int k = i; k < i + 16 && k < n; k++) Serial.printf(" %02X", d[k]);
    Serial.println();
  }
}

// ---------- Bildpuffer und Anzeige ----------
// Jeder Bildschirm (Text, Karte, Hoehentrack/Stationen, Menue) wird zuerst in den Bildpuffer frameCanvas (PSRAM) gezeichnet und dann aufs
// Display kopiert. So laesst sich jeder Bildschirm als Screenshot speichern. Ohne Bildpuffer wird direkt gezeichnet.
GFXcanvas16 *bgCanvas = nullptr;            // Kartenhintergrund (Kacheln), liegt im PSRAM
GFXcanvas16 *frameCanvas = nullptr;         // aktueller Bildschirminhalt (auch Grundlage der Screenshots)

// "Grosser" Kartenhintergrund mit Rand ringsum (Trackball-Scroll-Diagnose Okt. 2026, siehe renderBackground()/
// bigRefillStart()): wird einmal mit zusaetzlichem Rand (bigMarginTiles Kacheln je Seite) gerendert; danach
// kostet ein Pan-Schritt innerhalb dieses Randes nur noch einen Speicher-Crop (kein erneutes Kacheldekodieren/
// SD-Zugriff) -- siehe prepareMapBackground(). Seit FW 3.0 gibt es ohnehin nur noch ganze Zoomstufen (der
// Kommazahl-/Zwischen-Zoom liess sich mit diesem Randpuffer nicht sauber/einfach genug kombinieren und wurde
// deshalb entfernt); der alte, randlose Weg ueber bgCanvas/renderBackground() bleibt nur noch als Rueckfall-
// ebene fuer den Fall, dass bigCanvas nicht alloziert werden konnte (zu wenig PSRAM).
GFXcanvas16 *bigCanvas = nullptr;
static int bigMarginTiles = 2;              // Wunschwert; wird in setup() bei Allozierungsfehlschlag automatisch verkleinert (2->1->0)
static int bigW = 0, bigH = 0;              // tatsaechliche Groesse von bigCanvas (320+2*M, 240+2*M); 0 = kein bigCanvas (PSRAM voll)
static double bigZ = -1, bigCx = 0, bigCy = 0;   // Zoom/Mitte, auf die bigCanvas zuletzt zentriert wurde
static int bigStyle = -1;                   // Kartenart, mit der bigCanvas zuletzt gefuellt wurde (-1 = nie); siehe bigTilesReady() --
                                             // ohne diesen Vergleich haelt ein Kartenart-Wechsel (Taste M) die alten Kacheln fuer
                                             // "schon geladen" und zeichnet sie nie neu, bis der Rand ohnehin woanders nachlaedt
#define BIG_TILES_MAX 8                     // Kachelraster-Grosse (je Achse) -- reicht fuer Bildschirm + 2 Kacheln Rand mit Luft
static bool bigTileLoaded[BIG_TILES_MAX][BIG_TILES_MAX];
static long bigTileBaseTx = 0, bigTileBaseTy = 0;   // Kachelkoordinate der oben-links-Kachel in bigCanvas (Raster-Ursprung)
#define BIG_REFILL_MAX_TILES (BIG_TILES_MAX * BIG_TILES_MAX)
static int16_t bigRefillTx[BIG_REFILL_MAX_TILES], bigRefillTy[BIG_REFILL_MAX_TILES];
static int16_t bigRefillSx[BIG_REFILL_MAX_TILES], bigRefillSy[BIG_REFILL_MAX_TILES];  // Zielposition in bigCanvas
static int bigRefillCount = 0, bigRefillIdx = 0;   // Warteschlange: Index 0 bis bigRefillIdx-1 schon erledigt, Rest offen
static int bigRefillZi = 0;                 // Zoomstufe, auf die sich die Warteschlange bezieht

static Adafruit_GFX *scr() { return frameCanvas ? (Adafruit_GFX *)frameCanvas : (Adafruit_GFX *)&tft; }

static void scrFlush(int x, int y, int w, int h) {          // Ausschnitt des Bildpuffers aufs Display kopieren
  if (!frameCanvas) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > 320) w = 320 - x;
  if (y + h > 240) h = 240 - y;
  if (w <= 0 || h <= 0) return;
  uint16_t *buf = frameCanvas->getBuffer();
  tft.startWrite();
  tft.setAddrWindow(x, y, w, h);
  for (int r = 0; r < h; r++) tft.writePixels(buf + (y + r) * 320 + x, w);
  tft.endWrite();
}

static void scrClear() {
  if (frameCanvas) frameCanvas->fillScreen(ST77XX_BLACK);
  tft.fillScreen(ST77XX_BLACK);
}

void line(int y, const String &s, uint16_t color = ST77XX_WHITE) {
  if (mapMode) return;
  Adafruit_GFX *g = scr();
  g->fillRect(0, y, (y < 22) ? 262 : 320, 10, ST77XX_BLACK);   // oben rechts bleibt der Statusblock stehen
  g->setTextSize(1);
  g->setTextColor(color);
  g->setCursor(2, y);
  g->print(s);
  scrFlush(0, y, 320, 10);
}

void drawSpectrum() {
  const int bw = 10, x0 = (320 - NCOARSE * bw) / 2, yBase = 222, hMax = 106;   // 31 Balken a 10 px = volle Breite
  Adafruit_GFX *g = scr();
  g->fillRect(0, 108, 320, 132, ST77XX_BLACK);
  for (int i = 0; i < NCOARSE; i++) {
    int h = constrain((int)map((long)spec[i], -130, -50, 0, hMax), 0, hMax);
    g->fillRect(x0 + i * bw, yBase - h, bw - 1, h, isCand[i] ? ST77XX_RED : ST77XX_GREEN);
  }
  g->drawFastHLine(x0, yBase, NCOARSE * bw, ST77XX_WHITE);
  g->setTextSize(1);
  g->setTextColor(ST77XX_CYAN);
  for (int k = 0; k <= 6; k++) {                        // jede volle MHz beschriften (400 ... 406)
    int x = x0 + k * 5 * bw;                            // 5 Balken = 1 MHz
    g->drawFastVLine(x, yBase, 3, ST77XX_WHITE);
    g->setCursor(constrain(x - 9, 0, 302), yBase + 5);
    g->print(400 + k);
  }
  scrFlush(0, 108, 320, 132);
}

// Vorgezogen vor drawFixedPanel() (Arduino-Reihenfolge-Falle bei globalen Variablen, anders als bei Funktionen
// gibt es dafuer keine automatischen Prototypen): sondeRssi/sondeId/sondeFnr/lastRxMs werden eigentlich erst
// weiter unten bei der Sondendecodierung befuellt, aber schon hier in drawFixedPanel() gelesen.
static float sondeRssi = 0;
static char sondeId[9] = "";
static int sondeFnr = 0;
static uint32_t lastRxMs = 0;                   // Zeitpunkt des letzten empfangenen Frames (auch ohne gueltige Position --
                                                 // Schritt 1 alleine reicht schon; fuer den kurzen gruenen Blink-Ring, s. u.)

// Bei fester Frequenz (fixedFreq, Menue S) laeuft kein Suchlauf -- drawSpectrum() wird dann nie aufgerufen
// und der Bereich bliebe bis auf die Frequenzzeile leer. Zeigt stattdessen, in derselben Bildflaeche wie
// drawSpectrum() (108..240), einen Live-Pegelbalken bei der festen Frequenz (liveDbm: Momentan-RSSI,
// radio.getRSSI(false), siehe loop() -- unabhaengig davon, ob gerade ein Frame decodiert wird) sowie eine
// kleine Empfangsstatistik (Pakete gesamt, letzter Empfang, letzte decodierte Sonde).
void drawFixedPanel(float liveDbm) {
  Adafruit_GFX *g = scr();
  g->fillRect(0, 108, 320, 132, ST77XX_BLACK);
  const int bx0 = 20, bx1 = 300, by = 128, bh = 22;
  g->setTextSize(1);
  g->setTextColor(ST77XX_CYAN);
  char lv[24];
  snprintf(lv, sizeof(lv), "Pegel: %.0f dBm", liveDbm);
  g->setCursor(bx0, by - 11);
  g->print(lv);
  g->drawRect(bx0, by, bx1 - bx0, bh, ST77XX_WHITE);
  int w = constrain((int)map((long)liveDbm, -130, -50, 0, bx1 - bx0 - 2), 0, bx1 - bx0 - 2);
  if (w > 0) g->fillRect(bx0 + 1, by + 1, w, bh - 2, ST77XX_GREEN);
  int y = by + bh + 16;
  g->setTextColor(ST77XX_WHITE);
  char ln[56];
  snprintf(ln, sizeof(ln), "Pakete gesamt: %lu", (unsigned long)frameCount);
  g->setCursor(bx0, y); g->print(ln); y += 14;
  if (lastRxMs) snprintf(ln, sizeof(ln), "Letzter Empfang: vor %lu s (%.0f dBm)", (unsigned long)((millis() - lastRxMs) / 1000), sondeRssi);
  else snprintf(ln, sizeof(ln), "Letzter Empfang: noch keiner");
  g->setCursor(bx0, y); g->print(ln); y += 14;
  if (sondeId[0] && sondeId[0] != '?') snprintf(ln, sizeof(ln), "Letzte Sonde: %s  Nr %d", sondeId, sondeFnr);
  else snprintf(ln, sizeof(ln), "Letzte Sonde: --");
  g->setCursor(bx0, y); g->print(ln);
  scrFlush(0, 108, 320, 132);
}

// ---------- Karten-Selbsttest: Kacheln auf der SD-Karte (/tiles/<z>/<x>/<y>.png, OSM-Schema) ----------
#define TILE_TEST   0                  // 1 = Selbsttest beim Start (Ordnerlisten bei Zoom 17/18 dauern ca. 1 Minute), 0 = aus
#define TILE_DIR    "/tiles"                   // Basiskarte (OpenCycleMap), auch Ersatz fuer fehlende Kacheln, PNG: /tiles/<z>/<x>/<y>.png
#define TILE_DIR_TOPO "/tiles_topo"            // Topografische Karte (z. B. DTK + Schummerung), JPG oder PNG: /tiles_topo/<z>/<x>/<y>.jpg
#define TILE_DIR_TOPPLUS "/tiles_topplus"      // TopPlusOpen (BKG, bundesweit), JPG oder PNG: /tiles_topplus/<z>/<x>/<y>.jpg
#define TILE_DIR_RELIEF "/tiles_relief"        // Gelaendeschummerung, JPG oder PNG: /tiles_relief/<z>/<x>/<y>.jpg
#define TILE_DIR_SAT "/tiles_sat"              // reines Luftbild, JPG: /tiles_sat/<z>/<x>/<y>.jpg
#define TILE_BLANK_BYTES 2048                  // JPG-Kacheln bis zu dieser Groesse sind leer (ausserhalb der Abdeckung) -> Basiskarte
#define TILE_ZOOM   16                 // Zoomstufe der Testanzeige (falls dort keine Kachel liegt: naechstniedrigere)
const double TEST_LAT = HOME_LAT;      // Position aus dem Empfangstest, nur zum Pruefen der Kacheln (HOME_LAT/LON siehe config.h)
const double TEST_LON = HOME_LON;

// Kartenansicht: "Ich" kommt vom GPS des T-Deck Plus, ohne Fix gilt die feste Ersatzposition
// HOME_LAT/HOME_LON kommen aus config.h (siehe #include "config.h" oben)
const double LAND_ALT_DEFAULT = 100.0;          // erwartete Landehoehe in "Sondenhoehe" (WGS84-Ellipsoid, in Deutschland etwa Meereshoehe + 47 m);
                                       // Tipp: Hoehe, die die Sonde am Boden am Startort anzeigt
#define PRED_MIN_SINK 1.0              // Landevorhersage erst ab dieser Sinkrate [m/s] (= 3,6 km/h)
#define PIN_GPS_RX  44                 // T-Deck Plus: GPS-TX -> ESP32-RX (Baudrate wird beim Start erkannt)
#define PIN_GPS_TX  43
#define MAP_MIN_Z   6                  // kleinste / groesste Zoomstufe der Kartenansicht
#define MAP_MAX_Z   19                 // echte Kacheln vorhanden (z. B. tiles_topo bis Stufe 19); fehlt eine Kachel
                                       // bei dieser Stufe fuer die gerade gewaehlte Kartenart, faellt drawTileStyled()
                                       // wie ueberall sonst auf die Basiskarte (Rad) zurueck -- kein Hochskalieren mehr
#define MAP_IDLE_Z  15                 // Zoomstufe der Karte, solange keine Sonde empfangen wurde (Menue Z ueberschreibt)

#define PAN_DIR      1                 // Scrollrichtung: 1 = Karte folgt der Trackball-Richtung, -1 = umgekehrt

// Seit FW 3.0 nur noch ganze Zoomstufen (kein Kommazahl-/"stufenloser" Anzeige-Zoom mehr, siehe effZoom()/
// renderBackground()/mapViewSetup()) -- der grosse Kartenhintergrund mit Rand (bigCanvas, schnelles Trackball-
// Scrollen) liess sich fuer skalierte Zwischen-Zoomstufen nicht sauber/einfach genug umsetzen. z bleibt als
// double (fuer latLonToPixel()/Weltkoordinaten und den Zoomtasten-Feinschritt in MI_ZOOM), landet aber vor der
// Anzeige immer bei einer ganzen Zahl. Weltpixel bei Zoom z: die ganze Welt ist 256 * 2^z Pixel breit
static void latLonToPixel(double lat, double lon, double z, double &px, double &py) {
  double n = 256.0 * exp2(z);
  px = (lon + 180.0) / 360.0 * n;
  double sn = sin(lat * M_PI / 180.0);
  py = (0.5 - log((1.0 + sn) / (1.0 - sn)) / (4.0 * M_PI)) * n;
}

static void pixelToLatLon(double px, double py, double z, double &lat, double &lon) {
  double n = 256.0 * exp2(z);
  lon = px / n * 360.0 - 180.0;
  lat = atan(sinh(M_PI * (1.0 - 2.0 * py / n))) * 180.0 / M_PI;
}

// Nur noch fuer den (unbenutzten) Kartentest tileSelfTest()/tileAtPositionReport() weiter unten -- der
// eigentliche Kartenaufbau laeuft komplett ueber die gepackten Bundles (siehe drawTileStyled() unten),
// einzelne Kachel-Dateien (PNG oder JPG) gibt es auf der SD-Karte nicht mehr.
static void tilePath(char *buf, size_t n, int z, long x, long y) {
  snprintf(buf, n, "%s/%d/%ld/%ld.png", TILE_DIR, z, x, y);
}

int drawX = 0, drawY = 0;
uint16_t *tgtBuf = nullptr;                 // Ziel fuer jpgDraw
int tgtW = 320, tgtH = 240;                 // Groesse des Ziels (Bildpuffer 320x240 oder Kachelpuffer 256x256)

// ---------- JPEG-Kacheln (Luftbild / Topo / Relief) ----------
JPEGDEC jpg;
File jpgFile;
static bool jpgBlank = false;                  // Kachel ist so klein, dass sie leer sein muss (ausserhalb der Abdeckung)

static void *jpgOpen(const char *filename, int32_t *size) {
  jpgFile = SD.open(filename);
  if (!jpgFile) { *size = 0; return nullptr; }
  *size = (int32_t)jpgFile.size();
  jpgBlank = (*size <= TILE_BLANK_BYTES);
  return &jpgFile;
}
static void jpgClose(void *handle) { if (jpgFile) jpgFile.close(); }
static int32_t jpgRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!jpgFile) return 0;
  return jpgFile.read(buffer, length);
}
static int32_t jpgSeek(JPEGFILE *handle, int32_t position) {
  if (!jpgFile) return 0;
  return jpgFile.seek(position);
}
// Zeichnet einen Block der Kachel in den Zielpuffer tgtBuf (tgtW x tgtH, RGB565), zugeschnitten
static int jpgDraw(JPEGDRAW *pDraw) {
  if (!tgtBuf) return 1;
  int x0 = drawX + pDraw->x;
  for (int r = 0; r < pDraw->iHeight; r++) {
    int y = drawY + pDraw->y + r;
    if (y < 0 || y >= tgtH) continue;
    int xs = max(0, x0), xe = min(tgtW, x0 + pDraw->iWidth);
    if (xe <= xs) continue;
    memcpy(tgtBuf + y * tgtW + xs, pDraw->pPixels + r * pDraw->iWidth + (xs - x0), (xe - xs) * 2);
  }
  return 1;
}
// JPG-Kachel zeichnen; false, wenn Datei fehlt, leer (sehr klein) oder nicht dekodierbar ist
static bool drawJpgTile(const char *path, int sx, int sy) {
  drawX = sx; drawY = sy;
  jpgBlank = false;
  if (!jpg.open(path, jpgOpen, jpgClose, jpgRead, jpgSeek, jpgDraw)) return false;
  if (jpgBlank) { jpg.close(); return false; }
  jpg.setPixelType(RGB565_LITTLE_ENDIAN);
  int rc = jpg.decode(0, 0, 0);
  jpg.close();
  return rc == 1;
}

// ---------- Kachel-Bundles (gepackt mit pack_tiles.py, statt Tausender Einzeldateien) ----------
// Layout je Kartenart: <ordner>/bundles/z<Zoom>.tbi (Index) + z<Zoom>_p<N>.tb (Daten, meist nur Teil 0;
// mehrere Teile nur bei sehr grossen Zoomstufen, weil FAT32 keine Datei ueber 4 GiB erlaubt).
// Header (16 Byte): Magic "TBI3", Anzahl Eintraege (u32), Zoomstufe (u32), Dateityp (u32: 0 PNG, 1 JPG).
// Datensatz (20 Byte): x, y, Teil, Offset im Teil, Laenge -- alle u32, little endian, sortiert nach (x, y).
// Fehlt ein Bundle (noch nicht gepackt) oder die Kachel darin, faellt drawTileStyled() automatisch auf die
// bisherigen Einzeldateien zurueck -- das Bundle-System ist rein optional und aendert nichts an Zoomstufen,
// die noch nicht gepackt sind.
#define BUNDLE_HDR_LEN  16
#define BUNDLE_REC_LEN  20
#define BUNDLE_MAX_TILE (400 * 1024)      // groesste Kachel, die aus einem Bundle gelesen wird (PSRAM-Puffer)

// Kachelordner je Kartenart -- muss vor den Bundle-Funktionen stehen, die ihn per Index ansprechen
static const char *const styleDir[NSTYLE] = {TILE_DIR, TILE_DIR_SAT, TILE_DIR_TOPO, TILE_DIR_RELIEF, TILE_DIR_TOPPLUS};

static BundleIdx bIdx[2];               // [0] aktuelle Kartenart (mapStyle), [1] Basiskarte (Fallback, style 0)
static BundlePart bPart[2];
static uint8_t *bundleTileBuf = nullptr;   // PSRAM-Puffer: eine Kachel aus dem Bundle, RAM-Quelle fuer JPEGDEC

static uint32_t bRdU32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

// Oeffnet/prueft den Index fuer (style, zoom); ein einmal fehlgeschlagener Versuch wird bis zum naechsten
// Style-/Zoomwechsel nicht wiederholt (kein SD-Zugriff pro Kachel, wenn fuer diese Stufe nichts gepackt ist).
static bool bundleIdxOpen(BundleIdx &b, int style, int zoom) {
  if (b.style == style && b.zoom == zoom) return b.ok;
  if (b.tbi) b.tbi.close();
  b.style = style; b.zoom = zoom; b.ok = false; b.count = 0;
  char path[80];
  snprintf(path, sizeof(path), "%s/bundles/z%d.tbi", styleDir[style], zoom);
  b.tbi = SD.open(path);
  if (!b.tbi) return false;
  uint8_t hdr[BUNDLE_HDR_LEN];
  if (b.tbi.read(hdr, BUNDLE_HDR_LEN) != BUNDLE_HDR_LEN || hdr[0] != 'T' || hdr[1] != 'B' || hdr[2] != 'I' || hdr[3] != '3') {
    b.tbi.close();
    return false;
  }
  b.count = bRdU32(hdr + 4);
  b.ext = (uint8_t)bRdU32(hdr + 12);
  b.ok = (b.count > 0);
  return b.ok;
}

// Binaersuche im offenen Index (sortiert nach x, dann y).
static bool bundleIdxFind(BundleIdx &b, long x, long y, int &part, uint32_t &offset, uint32_t &length) {
  if (!b.ok) return false;
  int32_t lo = 0, hi = (int32_t)b.count - 1;
  uint8_t rec[BUNDLE_REC_LEN];
  while (lo <= hi) {
    int32_t mid = lo + (hi - lo) / 2;
    if (!b.tbi.seek(BUNDLE_HDR_LEN + (uint32_t)mid * BUNDLE_REC_LEN)) return false;
    if (b.tbi.read(rec, BUNDLE_REC_LEN) != BUNDLE_REC_LEN) return false;
    uint32_t rx = bRdU32(rec), ry = bRdU32(rec + 4);
    if (rx == (uint32_t)x && ry == (uint32_t)y) {
      part = (int)bRdU32(rec + 8);
      offset = bRdU32(rec + 12);
      length = bRdU32(rec + 16);
      return true;
    }
    if (rx < (uint32_t)x || (rx == (uint32_t)x && ry < (uint32_t)y)) lo = mid + 1; else hi = mid - 1;
  }
  return false;
}

// Liest die Kachel-Bytes aus dem passenden Bundle-Teil in bundleTileBuf (slotIdx: 0 Kartenart, 1 Basiskarte).
static bool bundleReadTile(int slotIdx, int style, int zoom, int part, uint32_t offset, uint32_t length) {
  if (!bundleTileBuf || length == 0 || length > BUNDLE_MAX_TILE) return false;
  BundlePart &bp = bPart[slotIdx];
  if (!(bp.tb && bp.style == style && bp.zoom == zoom && bp.part == part)) {
    if (bp.tb) bp.tb.close();
    char path[80];
    snprintf(path, sizeof(path), "%s/bundles/z%d_p%d.tb", styleDir[style], zoom, part);
    bp.tb = SD.open(path);
    bp.style = style; bp.zoom = zoom; bp.part = part;
    if (!bp.tb) return false;
  }
  if (!bp.tb.seek(offset)) return false;
  return bp.tb.read(bundleTileBuf, length) == (int)length;
}

// Cache fuer Kacheln, die in diesem Bundle/Zoom nachweislich fehlen: spart die Binaersuche (mehrere SD-Seeks)
// fuer Kacheln, die Frame fuer Frame erneut angefragt werden (z.B. durch das staendige Neuzeichnen von
// tightFollow in Sondennaehe). Pro Slot (Kartenart/Basiskarte) ein kleiner Ringpuffer, bei Style-/Zoomwechsel
// automatisch ungueltig, weil der Treffer zusaetzlich auf (style, zoom) geprueft wird.
#define MISS_CACHE_N 64
struct MissEntry { int8_t style; int8_t zoom; int32_t x; int32_t y; };
static MissEntry missCache[2][MISS_CACHE_N];
static int missCacheN[2] = {0, 0};
static int missCacheNext[2] = {0, 0};

static bool missCacheHas(int slotIdx, int style, int z, long x, long y) {
  for (int i = 0; i < missCacheN[slotIdx]; i++) {
    MissEntry &m = missCache[slotIdx][i];
    if (m.style == style && m.zoom == z && m.x == (int32_t)x && m.y == (int32_t)y) return true;
  }
  return false;
}
static void missCacheAdd(int slotIdx, int style, int z, long x, long y) {
  int i = missCacheNext[slotIdx];
  missCache[slotIdx][i] = {(int8_t)style, (int8_t)z, (int32_t)x, (int32_t)y};
  missCacheNext[slotIdx] = (i + 1) % MISS_CACHE_N;
  if (missCacheN[slotIdx] < MISS_CACHE_N) missCacheN[slotIdx]++;
}

// Kachel aus einem Bundle zeichnen; false, wenn kein Bundle da ist, die Kachel fehlt, leer (ausserhalb
// der Abdeckung) oder nicht dekodierbar ist. Es gibt nur noch JPG-Bundles (b.ext == 1); alles andere
// (z.B. ein altes PNG-Bundle) wird als "nicht vorhanden" behandelt.
static bool drawBundleTile(int slotIdx, int style, int z, long x, long y, int sx, int sy) {
  if (missCacheHas(slotIdx, style, z, x, y)) return false;
  if (!bundleIdxOpen(bIdx[slotIdx], style, z)) return false;
  int part; uint32_t offset, length;
  if (!bundleIdxFind(bIdx[slotIdx], x, y, part, offset, length)) {
    missCacheAdd(slotIdx, style, z, x, y);
    return false;
  }
  BundleIdx &b = bIdx[slotIdx];
  if (b.ext != 1) return false;                   // nur JPG-Bundles werden unterstuetzt
  if (length <= TILE_BLANK_BYTES) {                // leere Kachel: wie "fehlt" behandeln
    missCacheAdd(slotIdx, style, z, x, y);
    return false;
  }
  if (!bundleReadTile(slotIdx, style, z, part, offset, length)) return false;
  drawX = sx; drawY = sy;
  if (!jpg.openRAM(bundleTileBuf, (int)length, jpgDraw)) return false;
  jpg.setPixelType(RGB565_LITTLE_ENDIAN);
  int rc = jpg.decode(0, 0, 0);
  jpg.close();
  return rc == 1;
}

// Kachel der gewaehlten Kartenart; fehlt sie oder ist sie leer (ausserhalb der Abdeckung), kommt die Basiskarte.
// Einzeldateien (JPG/PNG) gibt es nicht mehr auf der SD-Karte -- nur noch die gepackten Bundles.
static bool drawTileStyled(int z, long x, long y, int sx, int sy) {
  if (mapStyle > 0 && mapStyle < NSTYLE) {
    if (drawBundleTile(0, mapStyle, z, x, y, sx, sy)) return true;
  }
  return drawBundleTile(1, 0, z, x, y, sx, sy);
}

// ---------- SD-Karte ----------
// max_files hoeher als der Default (5) setzen: dauerhaft offen sind bis zu 2 Bundle-Indizes (bIdx) + 2
// Bundle-Datenteile (bPart) + die beiden laufenden GPX-Spuren (gpxIch/gpxSonde) = bis zu 6 Handles allein
// fuer die Kartendarstellung/Aufzeichnung, dazu kommen kurz lebende (Log, Listing, Screenshot, Webserver).
// Mit dem alten Default liefen beim Vollbetrieb (Karte + Aufzeichnung) irgendwann alle SD.open()-Aufrufe
// (auch das Log selbst) mit "no free file descriptors" ins Leere.
#define SD_MAX_OPEN_FILES 12
static bool sdReady = false;
static bool sdInit() {
  if (!sdReady) {
    sdReady = SD.begin(PIN_SD_CS, SPI, 4000000, "/sd", SD_MAX_OPEN_FILES);
    if (sdReady) {                              // Ordner fuer Logs, GPX und Screenshots sicherstellen
      if (!SD.exists(LOG_DIR)) SD.mkdir(LOG_DIR);
      if (!SD.exists(GPX_DIR)) SD.mkdir(GPX_DIR);
      if (!SD.exists(SHOT_DIR)) SD.mkdir(SHOT_DIR);
    }
  }
  return sdReady;
}

// Platzhaltername, solange noch keine Uhrzeit vorliegt: rs41_log_01.txt .. rs41_log_<LOG_FALLBACK_MAX>.txt
static void logFallbackPathFor(char *b, size_t n, int num) { snprintf(b, n, "%s/rs41_log_%02d.txt", LOG_DIR, num); }

// Tatsaechlicher Pfad des aktuellen Logs: wird beim Start in startLog() gesetzt (zunaechst der Platzhaltername)
// und von onTimeSet() einmalig auf den Namen mit Datum/Zeit umbenannt. Leer = noch kein Log fuer diese Sitzung
// angelegt (SD nicht bereit oder Log aus).
static char logActivePath[48] = "";
static bool logCurrentPath(char *b, size_t n) {
  if (!logActivePath[0]) return false;
  snprintf(b, n, "%s", logActivePath);
  return true;
}

// Eine Logzeile: seriell und (wenn Log an) angehaengt an das aktuelle Log auf der SD-Karte.
// Keine eigene Groessenbegrenzung mehr -- das Log waechst, solange das Geraet laeuft; ein frisches Log wird
// nur bei jedem Neustart begonnen (startLog() in setup()), nicht mehr zwischendurch nach Groesse.
static void logLine(const char *s) {
  Serial.println(s);
  char pth[48];
  if (sdLogOn && sdReady && logCurrentPath(pth, sizeof(pth))) {
    File f = SD.open(pth, FILE_APPEND);
    if (f) {
      char ts[24];
      logTimestamp(ts, sizeof(ts));
      f.print(ts);
      f.print(' ');
      f.println(s);
      f.close();
    }
  }
}

// ---------- GPS (NMEA GGA: Position; RMC: Uhrzeit, Datum und eigene Geschwindigkeit) ----------
// hdop <= 0 bedeutet "unbekannt/kein aktueller Wert" (noch kein GGA-Satz mit Fix verarbeitet) -- siehe parseNmea().
struct GpsState { bool fix; double lat, lon; float alt; int sats; uint32_t bytes; uint32_t lastFix; uint32_t lastByteMs; float speedKmh; float hdop; float courseDeg; };
static GpsState gps = {false, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1};   // courseDeg: -1 = noch kein gueltiger RMC-Kurs
                                                                    // (0 Grad ist ein gueltiger Wert, braucht also
                                                                    // einen eigenen "unbekannt"-Sentinel)
// letzte bekannte GPS-Position: Ersatz fuer "Ich", solange das GPS keinen Fix hat (wird gespeichert)
static double homeLat = HOME_LAT, homeLon = HOME_LON;
static bool homeDirty = false;
static char nmeaBuf[120];
static int nmeaLen = 0;

// Sichtbare Satelliten je Konstellation (aus $--GSV, Feld "sats in view" -- NICHT dasselbe wie gps.sats, das aus
// GGA kommt und nur die beim FIX tatsaechlich benutzten Satelliten zaehlt, also bei "kein Fix" immer 0 ist).
// Damit laesst sich unterscheiden, ob die Antenne ueberhaupt etwas sieht (sichtbar > 0, aber kein Fix -> z. B.
// Mehrwegeempfang/Abschattung reicht fuer eine Loesung nicht) oder wirklich gar nichts (sichtbar auch 0 ->
// Antenne/Kabel/Sichtfeld). Pro Konstellation (Talker-ID wie "GP"/"GL"/"GA"/"GB"/"GQ") wird der zuletzt gemeldete
// Wert mit Zeitstempel gehalten; gsvVisibleTotal() summiert nur noch frische Eintraege (< 5 s alt), damit eine
// nicht mehr gesendete Konstellation nicht ewig mitgezaehlt wird.
struct GsvEntry { char talker[3] = {0, 0, 0}; int inView = 0; uint32_t ms = 0; };
static GsvEntry gsvTable[6];

static void gsvUpdate(const char *talker, int inView) {
  uint32_t now = millis();
  for (auto &e : gsvTable) {
    if (e.ms && strncmp(e.talker, talker, 2) == 0) { e.inView = inView; e.ms = now; return; }
  }
  for (auto &e : gsvTable) {
    if (!e.ms) { e.talker[0] = talker[0]; e.talker[1] = talker[1]; e.inView = inView; e.ms = now; return; }
  }
  // Tabelle voll (>6 Konstellationen gleichzeitig, praktisch nie): aelteste Konstellation verdraengen
  GsvEntry *oldest = &gsvTable[0];
  for (auto &e : gsvTable) if (e.ms < oldest->ms) oldest = &e;
  oldest->talker[0] = talker[0]; oldest->talker[1] = talker[1]; oldest->inView = inView; oldest->ms = now;
}

static int gsvVisibleTotal() {
  uint32_t now = millis();
  int sum = 0;
  for (auto &e : gsvTable) if (e.ms && now - e.ms < 5000) sum += e.inView;
  return sum;
}

// ---------- Einzelsatelliten (Azimut/Elevation/SNR) aus $--GSV, fuer die GPS-Radaransicht (gfxView == 2) ----------
// Jede $--GSV-Zeile traegt bis zu 4 Satelliten (PRN, Elevation, Azimut, SNR); mehrere Zeilen ("Satz x von y")
// decken zusammen alle von dieser Konstellation sichtbaren Satelliten ab. SNR fehlt/ist 0, wenn der Satellit
// laut Almanach sichtbar, aber (noch) nicht stark genug fuer eine Dekodierung ist -- das heisst noch nicht
// automatisch "nicht im Fix benutzt" (das steht nur in $--GSA, wird hier nicht ausgewertet). Eintraege, die
// SAT_TIMEOUT_MS nicht neu bestaetigt wurden, gelten als veraltet (Konstellation/Satellit nicht mehr gemeldet)
// und werden beim naechsten Tabellenengpass zuerst verdraengt bzw. in der Radaransicht ausgeblendet.
// SAT_MAX war mit 32 zu knapp bemessen: ein Empfaenger, der gleichzeitig GPS+Galileo+BeiDou (+ggf. GLONASS)
// meldet, hat schnell 35-45 gleichzeitig SICHTBARE (nicht benutzte!) Satelliten -- war die Tabelle voll,
// verdraengte jeder neu eintreffende Satellit einen anderen, noch gar nicht abgelaufenen Eintrag wieder, was
// in der Radaransicht genau wie das urspruengliche Flacker-Problem aussah (Satelliten erscheinen/verschwinden
// jede Sekunde), obwohl der Redraw selbst (siehe satRadarUpdate()) schon gedrosselt war -- diesmal lag es an
// der Tabellengroesse, nicht am Redraw-Takt. 64 Plaetze (RAM-Kosten minimal) decken auch volle Mehrkonstellation
// mit Reserve ab.
#define SAT_MAX 64
#define SAT_TIMEOUT_MS 8000UL
struct SatInfo { char talker[3] = {0, 0, 0}; int prn = 0; int elev = -1; int azim = -1; int snr = 0; uint32_t ms = 0; };
static SatInfo satTable[SAT_MAX];

static void satUpdate(const char *talker, int prn, int elev, int azim, int snr) {
  uint32_t now = millis();
  SatInfo *slot = nullptr;
  for (auto &e : satTable) {
    if (e.ms && e.prn == prn && strncmp(e.talker, talker, 2) == 0) { slot = &e; break; }
  }
  if (!slot) for (auto &e : satTable) if (!e.ms) { slot = &e; break; }
  if (!slot) {                                      // Tabelle voll (praktisch nie bei 32 Plaetzen): aeltesten verdraengen
    slot = &satTable[0];
    for (auto &e : satTable) if (e.ms < slot->ms) slot = &e;
  }
  slot->talker[0] = talker[0]; slot->talker[1] = talker[1];
  slot->prn = prn; slot->elev = elev; slot->azim = azim; slot->snr = snr; slot->ms = now;
}

// Anzahl noch frischer Eintraege (siehe SAT_TIMEOUT_MS) -- fuer die Kopfzeile der Radaransicht
static int satFreshCount() {
  uint32_t now = millis();
  int n = 0;
  for (auto &e : satTable) if (e.ms && now - e.ms < SAT_TIMEOUT_MS) n++;
  return n;
}

// Systemuhr aus dem GPS: damit stimmen auch die Zeiten der Logdateien auf der SD-Karte
static bool timeSet = false;
static bool timeLogged = false;
static uint32_t timeSetMs = 0;

// Die SD-Bibliothek vergibt beim Anlegen einer Datei (also meist gleich beim Einschalten, lange bevor das
// GPS die Uhrzeit liefert) einen festen Zeitstempel -- danach steht i.d.R. dauerhaft 1980 als Datum, auch
// wenn spaeter weitergeschrieben wird. Setzt den Zeitstempel EINMALIG per POSIX utime() nach, sobald die
// Uhrzeit (per GPS) feststeht. arduinoPath ist der SD.open()-Pfad (z. B. "/log/rs41_log_3.txt"); die
// SD-Bibliothek haengt die Karte standardmaessig unter "/sd" ein (SD.begin() ohne eigenen mountpoint).
static void touchFileTime(const char *arduinoPath) {
  if (!timeSet) return;
  char full[64];
  snprintf(full, sizeof(full), "/sd%s", arduinoPath);
  struct utimbuf times;
  times.actime = times.modtime = time(nullptr);
  utime(full, &times);
}

static int64_t daysFromCivil(int y, int m, int d) {          // Tage seit 1970-01-01 (proleptisch gregorianisch)
  y -= (m <= 2);
  const int era = (y >= 0 ? y : y - 399) / 400;
  const int yoe = y - era * 400;
  const int doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + doe - 719468;
}

static void setSystemTimeUtc(int y, int mo, int d, int h, int mi, int s) {
  struct timeval tv;
  tv.tv_sec = (time_t)(daysFromCivil(y, mo, d) * 86400LL + h * 3600 + mi * 60 + s);
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
}

static void timeText(char *b, size_t n) {                     // hh:mm:ss in Ortszeit, sonst --:--:--
  if (!timeSet) { snprintf(b, n, "--:--:--"); return; }
  time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);
  snprintf(b, n, "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

static void dateTimeText(char *b, size_t n) {                 // JJJJ-MM-TT hh:mm:ss in Ortszeit
  if (!timeSet) { snprintf(b, n, "Uhrzeit unbekannt"); return; }
  time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);
  snprintf(b, n, "%04d-%02d-%02d %02d:%02d:%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

// Zeitstempel fuer die SD-Logzeilen: dd.mm.yyyy hh:mm:ss in Ortszeit, oder der Platzhalter xx.xx.xxxx xx:xx:xx,
// solange noch keine Uhrzeit (GPS/NTP) vorliegt -- damit jede Zeile im Log sofort erkennen laesst, ob/wann sie war.
static void logTimestamp(char *b, size_t n) {
  if (!timeSet) { snprintf(b, n, "xx.xx.xxxx xx:xx:xx"); return; }
  time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);
  snprintf(b, n, "%02d.%02d.%04d %02d:%02d:%02d", tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_year + 1900, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

// Systemzeit wurde gerade gesetzt (per GPS oder NTP): einmalig die aktuelle Logdatei (bis dahin der
// Platzhaltername rs41_log_NN.txt aus startLog(), weil beim Anlegen noch keine Uhrzeit vorlag) auf den
// Namen mit Datum/Zeit umbenennen, dann loggen und den SD-Zeitstempel der Datei nachtragen (stand bisher
// auf 1980, da die SD-Bibliothek den beim Anlegen vergibt). src: "GPS"/"NTP".
// Benennt das aktuelle Log von seinem Platzhalternamen (rs41_log_NN.txt) auf den Namen mit Datum/Uhrzeit um --
// sobald die Uhrzeit (GPS/NTP) feststeht. Eigene Funktion (vorher Teil von onTimeSet()), weil sie jetzt von
// ZWEI Stellen gebraucht wird: einmal von onTimeSet() selbst (erster Zeitstempel nach dem Einschalten), und
// erneut von logEnableNow() (SD-Log wird mitten im Lauf eingeschaltet, Uhrzeit steht da i. d. R. schon fest --
// onTimeSet() benennt dank timeLogged aber nur noch EINMAL im Geraetelauf um, siehe dort).
static void logRenameToTimestamp() {
  if (!sdReady || !logActivePath[0]) return;
  time_t tt = time(nullptr);
  struct tm tmv;
  localtime_r(&tt, &tmv);
  char newPth[48], stamp[20];
  snprintf(stamp, sizeof(stamp), "%04d-%02d-%02d_%02d%02d%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
           tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  snprintf(newPth, sizeof(newPth), "%s/rs41_log_%s.txt", LOG_DIR, stamp);
  if (SD.exists(newPth)) SD.remove(newPth);      // Kollision (gleiche Sekunde) waere extrem unwahrscheinlich, aber sicher ist sicher
  if (SD.rename(logActivePath, newPth)) strncpy(logActivePath, newPth, sizeof(logActivePath) - 1);
  touchFileTime(logActivePath);
}

static void onTimeSet(const char *src) {
  timeSet = true;
  timeSetMs = millis();
  if (!timeLogged) {
    timeLogged = true;
    logRenameToTimestamp();
    char t[40], m[72];
    dateTimeText(t, sizeof(t));
    snprintf(m, sizeof(m), "### Zeit gesetzt (%s): %s", src, t);
    logLine(m);
  }
}

// Schaltet das SD-Log waehrend des laufenden Betriebs ein (Menue P/Taste P oder Webseite): startLog() lief
// bisher nur EINMAL in setup() -- war das Log beim Einschalten noch aus und wurde erst spaeter per Menue
// aktiviert, blieb logActivePath bis zum naechsten Neustart leer, und logLine() schrieb trotz sdLogOn=true
// NICHTS auf die SD (siehe logCurrentPath()/logLine()) -- ein nachtraeglich eingeschaltetes Log blieb also
// wirkungslos. Legt jetzt sofort eine frische Datei an (mit "### Start"-Zeile) und benennt sie, wenn die
// Uhrzeit schon feststeht, gleich auf den Datum/Uhrzeit-Namen um (sonst bliebe sie dauerhaft beim
// Platzhalternamen rs41_log_NN.txt, siehe logRenameToTimestamp()).
static void logEnableNow() {
  if (!sdReady || logActivePath[0]) return;      // kein SD-Zugriff, oder laeuft (z. B. seit dem Einschalten) schon ein Log
  startLog();
  if (timeSet) logRenameToTimestamp();
}

static double nmeaToDeg(const char *s, char hemi) {
  double v = atof(s);                          // ddmm.mmmm bzw. dddmm.mmmm
  int deg = (int)(v / 100.0);
  double d = deg + (v - deg * 100.0) / 60.0;
  return (hemi == 'S' || hemi == 'W') ? -d : d;
}

static int twoDigits(const char *s) { return (s[0] - '0') * 10 + (s[1] - '0'); }

// ---------- Konsistenzfilter fuer die eigene GPS-Position ----------
// Bei schlechtem Empfang liefert das GPS-Modul hin und wieder einzelne, kurz aufblitzende Fehlpositionen
// (ein oder zwei Datensaetze weit daneben). Der Filter verwirft einen Fix nur dann, wenn er sowohl zur
// zuletzt akzeptierten Position unplausibel weit entfernt ist ALS AUCH vom jeweils naechsten Fix nicht
// bestaetigt wird - eine echte, anhaltende Bewegung (z.B. Auto auf der Autobahn) wird dadurch schon ab
// dem zweiten Fix wieder uebernommen, ein einzelner Ausreisser bleibt dagegen aussen vor.
static const float GPS_MAX_SPEED_MS = 55.0f;         // ~200 km/h + Marge, deckt Auto/Rad/Fuss grosszuegig ab
static const float GPS_JUMP_MARGIN_M = 50.0f;        // zusaetzliche Toleranz gegen normales GPS-Jittern
static const uint32_t GPS_GLITCH_TIMEOUT_MS = 120000UL;  // nach 2 Min. auch unbestaetigt uebernehmen (z.B. nach Tunnel/Empfangsloch)
// Satellitengeometrie-Schranke fuer die "Sprung vom naechsten Fix bestaetigt"-Uebernahme (siehe gpsFilterFix()):
// etwas grosszuegiger als GPS_SEND_MAX_HDOP (das Senden soll strenger sein als die reine Spuranzeige), aber
// streng genug, um eine mehrsekuendige, korrelierte Mehrwege-Drift (die sich sonst selbst "bestaetigt") abzuwehren.
static const float GPS_FILTER_MAX_HDOP = 5.0f;
// Mindest-Satellitenzahl fuer dieselbe Uebernahme -- NACHTRAEGLICH ergaenzt: ein Log zeigte mehrere ueber
// mehrere Sekunden "bestaetigte" Spruenge von 500 m bis ueber 12 km, alle mit nur 3 Satelliten. Mit 3 Satelliten
// ist nicht mal ein reguaerer 3D-Fix moeglich (x/y/z + Uhrfehler braucht 4) -- der gemeldete HDOP ist dann
// mathematisch kaum noch aussagekraeftig und kann trotzdem klein wirken, obwohl die Position frei "schwimmt".
// Ohne diese zusaetzliche Schranke konnte die HDOP-Pruefung allein genau diese Faelle nicht abfangen.
static const int GPS_FILTER_MIN_SATS = 4;

static bool ownFiltValid = false;                    // letzte akzeptierte (gefilterte) Position
static double ownFiltLat = 0, ownFiltLon = 0;
static uint32_t ownFiltMs = 0;

static bool ownPendValid = false;                    // noch unbestaetigter Ausreisser-Kandidat (Position)
static double ownPendLat = 0, ownPendLon = 0;
static uint32_t ownPendMs = 0;

// Hoehe wird UNABHAENGIG von der Position gefiltert: der obige Sprungfilter prueft nur die Distanz zur
// letzten Position, nicht die Hoehe. Bei kurzzeitig schwachem Empfang (z. B. Solution faellt kurz auf einen
// reinen 2D-Fix zurueck) liefert das GPS-Modul dabei gelegentlich eine einzelne, klar falsche Hoehe -- meist
// genau 0 m --, obwohl die Position weiter stimmt. Nur ein einzelner, unbestaetigter Sprung wird verworfen;
// wirklich 0 m (z. B. am Meer) bleibt erhalten, sobald der naechste Fix das bestaetigt.
static const float GPS_ALT_JUMP_M = 40.0f;                    // Hoehensprung, der als Ausreisser gilt
static const uint32_t GPS_ALT_GLITCH_TIMEOUT_MS = 120000UL;   // nach 2 Min. auch unbestaetigt uebernehmen

static bool ownAltFiltValid = false;                 // letzte akzeptierte (gefilterte) Hoehe
static float ownAltFiltVal = 0;
static uint32_t ownAltFiltMs = 0;

static bool ownAltPendValid = false;                 // noch unbestaetigter Ausreisser-Kandidat (Hoehe)
static float ownAltPendVal = 0;
static uint32_t ownAltPendMs = 0;

// Prueft, ob ein neuer Fix (lat/lon) zum Vergleichspunkt (refLat/refLon, refMs) passt: Abstand innerhalb
// der bei GPS_MAX_SPEED_MS seit refMs maximal moeglichen Strecke plus Toleranz.
static bool posJumpOk(double refLat, double refLon, uint32_t refMs, double lat, double lon, uint32_t now) {
  double dtS = (now - refMs) / 1000.0;
  if (dtS < 0.05) dtS = 0.05;
  double dm = haversineKm(refLat, refLon, lat, lon) * 1000.0;
  return dm < GPS_MAX_SPEED_MS * dtS + GPS_JUMP_MARGIN_M;
}

// Prueft einen neuen eigenen GPS-Fix auf Plausibilitaet; liefert in fLat/fLon/fAlt die zu verwendende
// (gefilterte) Position/Hoehe und meldet per Rueckgabewert, ob die neue Position direkt uebernommen wurde.
static bool gpsFilterFix(double lat, double lon, float alt, double &fLat, double &fLon, float &fAlt) {
  uint32_t now = millis();

  // --- Position ---
  bool posOk;
  if (!ownFiltValid) {                               // allererster Fix: ungeprueft uebernehmen
    ownFiltValid = true;
    ownFiltLat = lat; ownFiltLon = lon; ownFiltMs = now;
    ownPendValid = false;
    posOk = true;
  } else if (posJumpOk(ownFiltLat, ownFiltLon, ownFiltMs, lat, lon, now)) {   // Sprung im plausiblen Rahmen
    ownFiltLat = lat; ownFiltLon = lon; ownFiltMs = now;
    ownPendValid = false;
    posOk = true;
  } else {
    // Sprung ausserhalb des plausiblen Rahmens um die zuletzt akzeptierte Position: ob der naechste Fix ihn
    // "bestaetigt" (naeherungsweise dort landet), heisst allein noch nicht, dass er stimmt -- bei Mehrwege-
    // empfang (Bruecken, Haeuserschluchten, kurz nach einem Tunnel) driftet die gemeldete Position oft mehrere
    // Sekunden LANGSAM UND KORRELIERT in die falsche Richtung: aufeinanderfolgende Fixe liegen dann nah beieinander
    // (bestaetigen sich also gegenseitig), sind aber alle gleichermassen falsch -- genau das hat frueher dazu
    // gefuehrt, dass sich ein einzelner Ausreisser zu einer ganzen Kette aus Falschpositionen mit absurdem
    // Tempo (mehrere 100 km/h) aufschaukeln konnte. Ein bestaetigter Sprung wird daher nur noch akzeptiert,
    // wenn der bestaetigende Fix auch eine brauchbare Satellitengeometrie meldet (genug Satelliten UND HDOP) --
    // sonst bleibt er als (aktualisierter) Kandidat haengen und wartet auf einen Fix mit besserer Geometrie
    // oder auf den Timeout. Die Satzahl-Schranke (GPS_FILTER_MIN_SATS) kam NACHTRAEGLICH dazu: ein Log zeigte
    // mehrere ueber mehrere Sekunden "durch den naechsten Fix bestaetigte" Spruenge von 500 m bis >12 km, alle
    // mit nur 3 Satelliten -- HDOP allein reichte nicht, weil ein 3-Satelliten-"Fix" (kein regulaerer 3D-Fix
    // moeglich, braucht 4) trotzdem einen unauffaelligen HDOP melden kann, waehrend die Position frei schwimmt.
    bool pendConfirmed = ownPendValid && posJumpOk(ownPendLat, ownPendLon, ownPendMs, lat, lon, now);
    bool geomOk = gps.sats >= GPS_FILTER_MIN_SATS && gps.hdop > 0 && gps.hdop <= GPS_FILTER_MAX_HDOP;
    if (pendConfirmed && geomOk) {                      // vom naechsten Fix bestaetigt UND brauchbare Geometrie -> echte Bewegung
      double offM = haversineKm(ownFiltLat, ownFiltLon, ownPendLat, ownPendLon) * 1000.0;
      char m[110];
      snprintf(m, sizeof(m), "GPS-Filter: Sprung %.0f m bestaetigt -> uebernommen (%d Sat, HDOP %.1f)", offM, gps.sats, gps.hdop);
      logLine(m);
      ownFiltLat = lat; ownFiltLon = lon; ownFiltMs = now;
      ownPendValid = false;
      posOk = true;
    } else if (now - ownFiltMs > GPS_GLITCH_TIMEOUT_MS) {   // seit 2 Min. keine akzeptierte Position mehr: nicht ewig festhalten
      char m[72];
      snprintf(m, sizeof(m), "GPS-Filter: %lu s ohne akzeptierten Fix -> naechster Fix uebernommen", (unsigned long)((now - ownFiltMs) / 1000));
      logLine(m);
      ownFiltLat = lat; ownFiltLon = lon; ownFiltMs = now;
      ownPendValid = false;
      posOk = true;
    } else {
      // Diagnose: Ausreisser (noch) verworfen -- Abstand zur zuletzt akzeptierten Position samt Satqualitaet
      // mitloggen, damit sich spaeter (z. B. nach einer Tour) nachvollziehen laesst, WARUM die Position kurz
      // danebenlag (kurzer Empfangsausfall/Mehrwegeempfang an einer Bruecke o. ae.) statt es nur an der Spur
      // zu sehen. pendConfirmed && !geomOk: der Sprung passt zwar zum vorherigen Kandidaten, aber die Geometrie
      // (Satzahl und/oder HDOP) ist (noch) zu schlecht, um ihn zu uebernehmen -- genau der Fall, der frueher
      // faelschlich durchging (siehe GPS_FILTER_MIN_SATS oben).
      double offM = haversineKm(ownFiltLat, ownFiltLon, lat, lon) * 1000.0;
      char m[128];
      if (pendConfirmed) snprintf(m, sizeof(m), "GPS-Filter: Sprung %.0f m bestaetigt, aber Geometrie zu schlecht -> weiter verworfen (%d Sat, HDOP %.1f)", offM, gps.sats, gps.hdop);
      else snprintf(m, sizeof(m), "GPS-Filter: Positionsausreisser verworfen (%.0f m, %d Sat, HDOP %.1f)", offM, gps.sats, gps.hdop);
      logLine(m);
      ownPendValid = true;                                // (aktualisierter) Kandidat: als naechstes gegen DIESEN Fix pruefen
      ownPendLat = lat; ownPendLon = lon; ownPendMs = now;
      posOk = false;
    }
  }
  fLat = posOk ? lat : ownFiltLat;
  fLon = posOk ? lon : ownFiltLon;

  // --- Hoehe (unabhaengig von der Position) ---
  bool altOk;
  if (!ownAltFiltValid) {                              // allererste Hoehe: ungeprueft uebernehmen
    ownAltFiltValid = true;
    ownAltPendValid = false;
    altOk = true;
  } else if (fabsf(alt - ownAltFiltVal) < GPS_ALT_JUMP_M) {   // Sprung im plausiblen Rahmen: direkt uebernehmen
    ownAltPendValid = false;
    altOk = true;
  } else if (ownAltPendValid && fabsf(alt - ownAltPendVal) < GPS_ALT_JUMP_M) {   // vom naechsten Fix bestaetigt
    {
      char m[90];
      snprintf(m, sizeof(m), "GPS-Filter: Hoehensprung %.0f->%.0f m bestaetigt -> uebernommen", ownAltFiltVal, alt);
      logLine(m);
    }
    ownAltPendValid = false;
    altOk = true;
  } else if (now - ownAltFiltMs > GPS_ALT_GLITCH_TIMEOUT_MS) {   // seit 2 Min. keine akzeptierte Hoehe mehr
    ownAltPendValid = false;
    altOk = true;
  } else {
    {
      char m[90];
      snprintf(m, sizeof(m), "GPS-Filter: Hoehenausreisser verworfen (%.0f m statt zuletzt %.0f m)", alt, ownAltFiltVal);
      logLine(m);
    }
    ownAltPendValid = true;                              // Ausreisser (z. B. der einzelne 0-m-Punkt): verwerfen
    ownAltPendVal = alt; ownAltPendMs = now;
    altOk = false;
  }
  if (altOk) { ownAltFiltVal = alt; ownAltFiltMs = now; }
  fAlt = altOk ? alt : ownAltFiltVal;

  return posOk;
}

// ---------- Qualitaets-Gate fuer das Versenden der eigenen Position (SondeHub-Listener, LoRa-APRS-Beacon) ----------
// Reine Satellitenanzahl ist ein schwacher Indikator fuer die Fix-Guete -- massgeblich ist die Satelliten-
// GEOMETRIE (HDOP): 5 gut ueber den Himmel verteilte Satelliten koennen genauer sein als 7 eng zusammenstehende.
// Die Sat-Zahl bleibt nur noch als grobe Untergrenze (das theoretische Minimum fuer einen 3D-Fix), die
// eigentliche Entscheidung trifft HDOP. hdop <= 0 heisst "noch kein HDOP-Wert vorhanden" und gilt als schlecht.
static const int GPS_SEND_MIN_SATS = 4;              // theoretisches Minimum fuer einen 3D-Fix (x/y/z + Uhrfehler)
static const float GPS_SEND_MAX_HDOP = 2.5f;         // <1 exzellent, 1..2 gut, 2..5 brauchbar, >5 fragwuerdig (Richtwerte)
static bool gpsFixGoodEnoughToSend() {
  return gps.fix && gps.sats >= GPS_SEND_MIN_SATS && gps.hdop > 0 && gps.hdop <= GPS_SEND_MAX_HDOP;
}

// Prueft die NMEA-Pruefsumme (XOR aller Bytes zwischen '$' und '*', als 2 Hexziffern danach). Ohne diese
// Pruefung liest atoi()/atof() unten aus einem durch Bitfehler/abgeschnittenem Satz verschobenen Feld --
// das erzeugte genau die Ausreisser wie "kein Fix (91920 Sat)" oder eine einzelne 0 m Hoehe im Log.
static bool nmeaChecksumOk(const char *s) {
  const char *star = strchr(s, '*');
  if (!star || star == s || !isxdigit((unsigned char)star[1]) || !isxdigit((unsigned char)star[2])) return false;
  uint8_t cs = 0;
  for (const char *p = s + 1; p < star; p++) cs ^= (uint8_t)*p;
  char hex[3] = { star[1], star[2], 0 };
  return (uint8_t)strtol(hex, nullptr, 16) == cs;
}

static void parseNmea(char *s) {
  if (strlen(s) < 10) return;
  if (!nmeaChecksumOk(s)) return;                   // verstuemmelter Satz (Bitfehler/abgeschnitten): verwerfen
  bool isGga = (strncmp(s + 3, "GGA", 3) == 0);
  bool isRmc = (strncmp(s + 3, "RMC", 3) == 0);
  bool isGsv = (strncmp(s + 3, "GSV", 3) == 0);
  if (!isGga && !isRmc && !isGsv) return;
  char *f[24];                                 // GSV mit 4 Satelliten braucht 4 Kopf- + 4*4 Nutzfelder = 20
  int n = 0;
  char *p = s;
  f[n++] = p;
  while (*p && n < 24) {
    if (*p == ',') { *p = 0; f[n++] = p + 1; }
    p++;
  }
  if (isGsv) {                                 // $GPGSV,Anzahl Saetze,lfd. Satz,sichtbare Satelliten,... -- Feld 3
    if (n >= 4) gsvUpdate(s + 1, atoi(f[3]));  // s+1/s+2 = Talker-ID ("GP"/"GL"/"GA"/"GB"/"GQ"/...), siehe gsvUpdate()
    // Einzelsatelliten: ab Feld 4 in 4er-Gruppen (PRN, Elevation, Azimut, SNR) -- siehe satUpdate() oben.
    // SNR kann leer sein (sichtbar, aber nicht stark genug fuer eine Dekodierung); PRN leer/0 heisst "kein
    // weiterer Satellit in dieser Gruppe" (letzte Zeile einer Konstellation oft nicht ganz voll).
    for (int gi = 4; gi + 3 < n; gi += 4) {
      int prn = atoi(f[gi]);
      if (prn <= 0) continue;
      int elev = f[gi + 1][0] ? atoi(f[gi + 1]) : -1;
      int azim = f[gi + 2][0] ? atoi(f[gi + 2]) : -1;
      int snr  = f[gi + 3][0] ? atoi(f[gi + 3]) : 0;
      satUpdate(s + 1, prn, elev, azim, snr);
    }
    return;
  }
  if (isRmc) {                                 // $GNRMC,hhmmss.ss,A,lat,N/S,lon,E/W,speed(Knoten),course,ddmmyy,...
    if (n >= 10 && f[2][0] == 'A') {
      gps.speedKmh = (float)(atof(f[7]) * 1.852);          // Knoten -> km/h, fuer die grosse Statuszeile auf der Karte
      if (f[8][0]) gps.courseDeg = (float)atof(f[8]);      // Kurs ueber Grund (0-359.9 Grad) -- fuer den
                                                              // komprimierten APRS-Beacon, siehe aprsEncodeCompressed()
      if (strlen(f[1]) >= 6 && strlen(f[9]) >= 6 &&
          (!timeSet || millis() - timeSetMs > 3600000UL)) {                  // beim ersten Fix und dann stuendlich
        int hh = twoDigits(f[1]), mm = twoDigits(f[1] + 2), ss = twoDigits(f[1] + 4);
        int dd = twoDigits(f[9]), mo = twoDigits(f[9] + 2), yy = 2000 + twoDigits(f[9] + 4);
        if (yy >= 2024 && yy < 2100 && mo >= 1 && mo <= 12 && dd >= 1 && dd <= 31 && hh < 24 && mm < 60 && ss < 61) {
          setSystemTimeUtc(yy, mo, dd, hh, mm, ss);
          onTimeSet("GPS");
        }
      }
    } else {
      gps.speedKmh = 0;                                     // kein gueltiger Fix: keine verlaessliche Geschwindigkeit
    }
    return;
  }
  if (n < 10) return;
  gps.sats = atoi(f[7]);
  if (atoi(f[6]) > 0 && f[2][0] && f[4][0]) {
    // Mehrkonstellations-Module senden oft mehrere GGA-Saetze fuer dieselbe Fix-Epoche (z. B. je ein Talker
    // fuer GPS-only und kombiniert GNSS), nur Millisekunden auseinander, mit leicht unterschiedlicher Position
    // (Empfangsrauschen) -- ohne Behandlung wird daraus ein "Geisterpunkt" mit absurder Geschwindigkeit.
    // Der ERSTE Satz einer Epoche durchlaeuft wie gehabt den Sprungfilter (gpsFilterFix) und wird gespeichert/
    // geloggt. Ein WEITERER Satz derselben Epoche durchlaeuft den Sprungfilter NICHT noch einmal (sonst
    // bekaeme er pro Epoche zwei Eingaben und waere leichter auszutricksen) -- wird aber, wenn er mehr
    // Satelliten nutzt oder einen besseren (kleineren) HDOP meldet, als genauere Verfeinerung desselben
    // Augenblicks uebernommen: Anzeige/gps.lat/lon/alt und der zuletzt gespeicherte GPX-Punkt werden dann
    // direkt ueberschrieben, statt einen zweiten Punkt anzuhaengen.
    static char lastGgaTime[8] = "";
    static int lastGgaSats = -1;
    static float lastGgaHdop = -1;
    bool sameEpoch = (strlen(f[1]) >= 6 && strncmp(f[1], lastGgaTime, 6) == 0);
    int sats = atoi(f[7]);
    float hdop = atof(f[8]);
    bool better = sats > lastGgaSats || (sats == lastGgaSats && hdop > 0 && (lastGgaHdop <= 0 || hdop < lastGgaHdop));
    if (sameEpoch && !better) return;                 // weiterer Satz derselben Epoche, aber nicht genauer: verwerfen
    if (strlen(f[1]) >= 6) { strncpy(lastGgaTime, f[1], 6); lastGgaTime[6] = 0; }
    lastGgaSats = sats;
    lastGgaHdop = hdop;
    gps.hdop = hdop;                                   // global verfuegbar fuer Qualitaets-Gates (SondeHub/LoRa-Beacon)

    double rawLat = nmeaToDeg(f[2], f[3][0]);
    double rawLon = nmeaToDeg(f[4], f[5][0]);
    float rawAlt = atof(f[9]);
    if (sameEpoch) {                                  // genauere Verfeinerung derselben Epoche: direkt uebernehmen,
      gps.lat = rawLat;                               // NICHT nochmal durch gpsFilterFix (das hat die Epoche schon
      gps.lon = rawLon;                               // einmal bewertet)
      gps.alt = rawAlt;
      if (gpxCountIch > 0) {                          // letzten gepufferten Punkt dieser Epoche korrigieren statt
        gpxBufIch[gpxCountIch - 1].lat = (float)gps.lat;   // einen zweiten anzuhaengen
        gpxBufIch[gpxCountIch - 1].lon = (float)gps.lon;
        gpxBufIch[gpxCountIch - 1].alt = gps.alt;
      }
    } else {
      double fLat, fLon; float fAlt;
      gpsFilterFix(rawLat, rawLon, rawAlt, fLat, fLon, fAlt);   // Ausreisser (Empfangsfehler) vor Speichern/Anzeigen filtern
      gps.lat = fLat;
      gps.lon = fLon;
      gps.alt = fAlt;
      gps.fix = true;
      gps.lastFix = millis();
      if (gpxEnsureIchId())   // erst ab dem ersten GPS-Zeitstempel aufzeichnen, dann steht der Dateiname fest
        gpxQueue(gpxIch, gpxBufIch, gpxCountIch, gpxIchId, gpxIchName, gps.lat, gps.lon, gps.alt);   // jeder Fix, ungekuerzt (im PSRAM gepuffert)
    }
    if (fabs(gps.lat - homeLat) > 0.0003 || fabs(gps.lon - homeLon) > 0.0005) {   // mehr als ca. 30 m weiter: merken
      homeLat = gps.lat;
      homeLon = gps.lon;
      homeDirty = true;
    }
  } else {
    gps.fix = false;
    gps.hdop = -1;                                     // kein Fix -- HDOP bedeutungslos, Qualitaets-Gates sollen das sehen
  }
}

static void gpsPoll() {
  while (Serial1.available()) {
    char c = Serial1.read();
    gps.bytes++;
    gps.lastByteMs = millis();
    if (c == '$') nmeaLen = 0;
    if (nmeaLen < (int)sizeof(nmeaBuf) - 1) nmeaBuf[nmeaLen++] = c;
    if (c == '\n') {
      nmeaBuf[nmeaLen] = 0;
      parseNmea(nmeaBuf);
      nmeaLen = 0;
    }
  }
}

// Baudrate suchen: erste Rate, bei der '$' (NMEA) ankommt
static void gpsBegin() {
  const uint32_t bauds[3] = {38400, 9600, 115200};
  for (int i = 0; i < 3; i++) {
    Serial1.end();
    Serial1.setRxBufferSize(2048);
    Serial1.begin(bauds[i], SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    uint32_t t0 = millis();
    int dollars = 0;
    while (millis() - t0 < 1500) {
      while (Serial1.available()) if (Serial1.read() == '$') dollars++;
      delay(5);
    }
    if (dollars >= 2) {
      char m[48];
      snprintf(m, sizeof(m), "GPS: NMEA-Daten bei %lu Baud", (unsigned long)bauds[i]);
      logLine(m);
      gps.bytes++;
      gps.lastByteMs = millis();
      return;
    }
  }
  logLine("GPS: keine NMEA-Daten gefunden -> Ersatzposition HOME_LAT/HOME_LON");
  Serial1.end();
  Serial1.setRxBufferSize(2048);
  Serial1.begin(38400, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
}

// Eigene Position: GPS-Fix (nicht aelter als 5 s), sonst die letzte bekannte GPS-Position ("fest"); true = aktueller GPS-Fix
static bool homePosition(double &lat, double &lon) {
  if (gps.fix && millis() - gps.lastFix < 5000) { lat = gps.lat; lon = gps.lon; return true; }
  lat = homeLat;
  lon = homeLon;
  return false;
}

// GPS-Zustand ins Log schreiben (bei Aenderung und alle 20 s -- vorher 60 s, aber HDOP/Tempo dazu gegen das
// Wegfallen der bisherigen "IchFollow"-Zeile (siehe idleMapUpdate(), dort entfernt) sollten auch zwischen zwei
// Zustandswechseln oefter als einmal die Minute im Log stehen) und die letzte Position hoechstens alle 2 Minuten sichern.
static void gpsLogTick() {
  static uint32_t lastLog = 0, lastSave = 0;
  static int prevState = -1;                        // 0 keine NMEA-Daten, 1 kein Fix, 2 Fix
  uint32_t now = millis();
  bool alive = gps.bytes > 0 && now - gps.lastByteMs < 3000;
  bool fixNow = gps.fix && now - gps.lastFix < 5000;
  int st = !alive ? 0 : (fixNow ? 2 : 1);
  if (st != prevState || now - lastLog >= 20000UL) {
    prevState = st;
    lastLog = now;
    char tt[12], m[150];
    timeText(tt, sizeof(tt));
    int visible = gsvVisibleTotal();   // "sichtbar" laut GSV, unabhaengig vom Fix -- siehe gsvVisibleTotal()/GsvEntry oben
    if (st == 2) snprintf(m, sizeof(m), "t%lu %s GPS: Fix, %d Sat (%d sichtbar), HDOP %.1f, %.5f %.5f, %.0f m, %.0f km/h",
                          (unsigned long)(now / 1000), tt, gps.sats, visible, gps.hdop, gps.lat, gps.lon, gps.alt, gps.speedKmh);
    else if (st == 1) snprintf(m, sizeof(m), "t%lu %s GPS: kein Fix (%d sichtbar)", (unsigned long)(now / 1000), tt, visible);
    else snprintf(m, sizeof(m), "t%lu %s GPS: keine NMEA-Daten", (unsigned long)(now / 1000), tt);
    logLine(m);
  }
  if (homeDirty && now - lastSave > 120000UL) {
    lastSave = now;
    homeDirty = false;
    prefs.putDouble("hlat", homeLat);
    prefs.putDouble("hlon", homeLon);
  }
}

static double haversineKm(double la1, double lo1, double la2, double lo2) {
  double p1 = la1 * M_PI / 180.0, p2 = la2 * M_PI / 180.0;
  double dp = p2 - p1, dl = (lo2 - lo1) * M_PI / 180.0;
  double a = sin(dp / 2) * sin(dp / 2) + cos(p1) * cos(p2) * sin(dl / 2) * sin(dl / 2);
  return 6371.0 * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
}

static double bearingDeg(double la1, double lo1, double la2, double lo2) {
  double p1 = la1 * M_PI / 180.0, p2 = la2 * M_PI / 180.0, dl = (lo2 - lo1) * M_PI / 180.0;
  double y = sin(dl) * cos(p2);
  double x = cos(p1) * sin(p2) - sin(p1) * cos(p2) * cos(dl);
  return fmod(atan2(y, x) * 180.0 / M_PI + 360.0, 360.0);
}

// ---------- Kartenansicht ----------
static bool haveSondePos = false;
// Zaehler, der bei jedem "neuen Sondenfund" (erste Position dieser Sitzung ODER Wechsel auf eine andere
// Sonden-ID -- siehe trackSonde()) hochgezaehlt wird. Dient nur der Weboberflaeche (/mstat, siehe webMenu()):
// der Browser merkt sich den zuletzt gesehenen Wert und erkennt an einer Aenderung "da ist etwas Neues",
// ohne dass der Server selbst Zustand pro Browser/Tab vorhalten muss.
static uint32_t sondeFoundSeq = 0;
static double sondeLat = 0, sondeLon = 0, sondeAlt = 0;

// ---------- Hoehentrack (Ansicht gfxView == 1 im Sondenmodus) ----------
// Hoehe ueber der Zeit seit dem ersten Fund der aktuellen Sonde; Puffer wird bei Sondenwechsel neu
// begonnen (siehe trackSonde()). Jedes empfangene Paket wird roh gespeichert, keine Ausduennung und
// kein Zusammenfassen -- stattdessen wird beim Zeichnen (altTrackUpdate()/webAltStat()) immer die
// GESAMTE gespeicherte Kurve (erstes bis letztes Paket) neu auf die Chartbreite skaliert. Ohne neuen
// Empfang aendert sich diese Skalierung nicht, der letzte Wert bleibt also einfach am rechten Rand
// stehen; kommt nach einer Funkpause wieder ein Paket, verbindet die Linie die beiden Punkte direkt
// (kein Fuellen der Luecke). Puffer liegt im PSRAM (siehe setup()), reicht bei RS41-typisch 1 Paket/s
// komfortabel fuer mehrstuendige Fluege; waere er doch einmal voll, werden weitere Pakete verworfen und
// die bisherige Kurve bleibt unangetastet erhalten (niemals rueckwirkend ausgeduennt).
#define ALT_TRACK_MAX 14400
static float *altTrackAlt = nullptr;            // Hoehe [m] je empfangenem Paket
static uint32_t *altTrackMs = nullptr;          // Zeitpunkt (millis(), relativ zum Trackbeginn) je Paket
static int altTrackN = 0;
static uint32_t altTrackStartMs = 0;

static void altTrackReset() {
  altTrackN = 0;
  altTrackStartMs = 0;
}

static void altTrackAdd(double alt) {
  if (!altTrackAlt || !altTrackMs) return;      // Zuteilung fehlgeschlagen (kein PSRAM) -- Ansicht bleibt leer
  uint32_t now = millis();
  if (altTrackN == 0) altTrackStartMs = now;
  if (altTrackN >= ALT_TRACK_MAX) return;       // s.o.: Puffer voll -- vorhandene Kurve bleibt unveraendert
  altTrackMs[altTrackN] = now - altTrackStartMs;
  altTrackAlt[altTrackN] = (float)alt;
  altTrackN++;
}

static float sondeVh = 0, sondeDir = 0, sondeVz = 0;   // Geschwindigkeit: horizontal [m/s], Richtung [Grad], vertikal [m/s]
static bool haveVel = false;
static bool predValid = false;                          // Landevorhersage vorhanden
static double predLat = 0, predLon = 0;                 // vorhergesagter Landepunkt
static uint32_t predEtaS = 0;                           // Restzeit bis zur Landung [s]

// Glaettung nur fuer die Landevorhersage (predictLanding()): die live angezeigte Momentangeschwindigkeit
// (sondeVh/sondeDir/sondeVz) bleibt unveraendert roh, aber ein einzelner verrauschter/pendelnder Fix soll den
// vorhergesagten Landepunkt nicht sofort um die volle Reststrecke verschieben. Zwei Stufen: (1) die ENU-
// Geschwindigkeit, mit der predictLanding() rechnet, wird per Tiefpass geglaettet; (2) der daraus berechnete
// Landepunkt selbst wird nochmal geglaettet, statt bei jedem Update hart zu springen.
#define PRED_SMOOTH_IN_ALPHA  0.25f      // Glaettung der Geschwindigkeit (0..1, kleiner = traeger/ruhiger)
#define PRED_SMOOTH_OUT_ALPHA 0.3f       // Glaettung des Landepunkts selbst
static float predVe = 0, predVn = 0, predVz = 0;        // geglaettete Ost/Nord/Oben-Geschwindigkeit fuer die Vorhersage
static bool predVelInit = false;
static bool predPtInit = false;                         // predLat/predLon schon eingeschwungen?

// ECEF-Geschwindigkeit in lokale Ost/Nord/Oben-Komponenten umrechnen (Position lat/lon in Grad)
static void ecefVelToEnu(double lat, double lon, double vx, double vy, double vz, double &e, double &n, double &u) {
  double la = lat * M_PI / 180.0, lo = lon * M_PI / 180.0;
  double sl = sin(la), cl = cos(la), so = sin(lo), co = cos(lo);
  e = -so * vx + co * vy;
  n = -sl * co * vx - sl * so * vy + cl * vz;
  u = cl * co * vx + cl * so * vy + sl * vz;
}

// Track-Verdichtung (fuer Sondenspur UND eigenen Track): die letzten KEEP_RECENT_KM bleiben IMMER in voller
// Aufloesung erhalten (aktuelle Position/Landung ist am wichtigsten) -- nur der aeltere Teil einer Spur wird bei
// Bedarf ausseduennt (jeder zweite Punkt weg), sobald der Puffer voll wird.
#define KEEP_RECENT_KM 5.0f

// Laeuft vom Ende der Spur rueckwaerts, bis die Summe der Punktabstaende KEEP_RECENT_KM erreicht.
// Rueckgabe: Index, ab dem (bis n-1) die Spur unangetastet bleiben muss.
static int trailRecentFrom(const float *lat, const float *lon, int n, float keepKm) {
  double sum = 0.0;
  int i = n - 1;
  while (i > 0) {
    sum += haversineKm(lat[i - 1], lon[i - 1], lat[i], lon[i]);
    if (sum >= keepKm) break;
    i--;
  }
  return i;
}

// Duennt nur den Teil VOR 'from' aus (jeder zweite Punkt); der Teil ab 'from' (die letzten KEEP_RECENT_KM)
// bleibt vollstaendig erhalten. Rueckgabe: neue Gesamtzahl der Punkte.
static int trailCompressOld(float *lat, float *lon, int n, int from) {
  int w = 0;
  for (int i = 0; i < from; i += 2) { lat[w] = lat[i]; lon[w] = lon[i]; w++; }
  for (int i = from; i < n; i++) { lat[w] = lat[i]; lon[w] = lon[i]; w++; }
  return w;
}

// Spur der Sonde: Punkte werden nur bei mindestens trailMinDist Metern Abstand gespeichert;
// ist der Puffer voll, wird der aeltere Teil ausseduennt (siehe trailCompressOld), die letzten
// KEEP_RECENT_KM bleiben unangetastet. trailMinDist waechst nur noch im Notfall (siehe unten).
#define TRAIL_MAX 800
static float trailLat[TRAIL_MAX], trailLon[TRAIL_MAX];
static int trailN = 0;
static float trailMinDist = 20.0f;
static char trailId[9] = "";

static void trailAdd(double lat, double lon, const char *id) {
  if (id[0] != '?' && strncmp(trailId, id, 8) != 0) {
    bool prevKnown = (trailId[0] != 0 && trailId[0] != '?');
    strncpy(trailId, id, 8);
    trailId[8] = 0;
    if (prevKnown) {                            // wirklich andere Sonde -> neue Spur (unbekannt -> bekannt behaelt die Spur)
      trailN = 0;
      trailMinDist = 20.0f;
    }
  } else if (trailId[0] == 0) {                 // erste Position mit noch unbekannter ID
    strcpy(trailId, "????????");
  }
  if (trailN > 0 && haversineKm(trailLat[trailN - 1], trailLon[trailN - 1], lat, lon) * 1000.0 < trailMinDist) return;
  if (trailN >= TRAIL_MAX) {
    int from = trailRecentFrom(trailLat, trailLon, trailN, KEEP_RECENT_KM);
    int n2 = trailCompressOld(trailLat, trailLon, trailN, from);
    if (n2 >= TRAIL_MAX) {          // Notfall (extrem dicht/verrauscht): ganzen Puffer nachverdichten
      for (int i = 0; i < TRAIL_MAX / 2; i++) { trailLat[i] = trailLat[2 * i]; trailLon[i] = trailLon[2 * i]; }
      n2 = TRAIL_MAX / 2;
      trailMinDist *= 2.0f;
    }
    trailN = n2;
  }
  trailLat[trailN] = (float)lat;
  trailLon[trailN] = (float)lon;
  trailN++;
}
// Eigener Track: GPS-Positionen des T-Deck, gleiches Ausduennen wie bei der Sondenspur
#define OWN_MAX 600
static float ownLat[OWN_MAX], ownLon[OWN_MAX];
static int ownN = 0;
static float ownMinDist = 10.0f;

// ---------- LoRa-APRS: gehoerte Stationen ----------
// Je Station Position + stark verkuerzte Spur (auf APRS_TRAIL_KM Gesamtlaenge gekappt, nicht nur
// ausgeduennt wie beim Sonden-/Eigentrack -- die Sonde ist in diesem Modus nicht relevant und wird
// nirgends gezeichnet, siehe drawAprsStations()/idleMapUpdate()).
// APRS_MAX von 16 auf 32 und APRS_MAX_AGE_MS von 15 auf 30 Minuten angehoben: gehoerte Stationen bleiben
// jetzt deutlich laenger im Puffer, bevor sie ganz herausfallen bzw. (bei vollem Puffer) ueberschrieben
// werden -- der Speicher dafuer ist mit gut 0,5 KB pro Station (vor allem der Fahrstreifen-Trail) auch bei
// doppelter Kapazitaet verschwindend klein, und die Stationsliste/Karte zeigen ohnehin nur so viele
// Eintraege, wie Bildschirmplatz da ist (siehe idleMapUpdate(), "while (... && y < 230)").
#define APRS_MAX 32
#define APRS_TRAIL_MAX 40
#define APRS_TRAIL_KM 5.0f
#define APRS_MAX_AGE_MS (30UL * 60UL * 1000UL)   // Station komplett aus der Liste entfernen, wenn laenger
                                                   // nicht mehr gehoert (siehe aprsPrune()) -- betrifft
                                                   // Stationsliste (Geraet/Web) UND Karte gleichermassen,
                                                   // da beide aus demselben Array lesen.
static char aprsCall[APRS_MAX][10];
static float aprsLat[APRS_MAX], aprsLon[APRS_MAX];
static uint32_t aprsSeenMs[APRS_MAX];
static float aprsTrailLat[APRS_MAX][APRS_TRAIL_MAX], aprsTrailLon[APRS_MAX][APRS_TRAIL_MAX];
static int aprsTrailN[APRS_MAX];
static int aprsN = 0;
// Empfangsart der letzten Meldung je Station -- Grundlage sowohl fuer die Punktfarbe auf der Karte
// (drawAprsStations(), siehe COL_APRS_*) als auch fuer den Fokus "Ich + alle direkt gehoerten Stationen"
// (idleMapUpdate()/focusCycleOrRecenter(): dort zaehlen RECV_DIRECT und RECV_DIGI beide als "direkt gehoert",
// nur RECV_IGATE -- also wirklich ueber das Internet/APRS-IS gelaufen -- gilt dort als nicht direkt).
enum { RECV_DIRECT = 0, RECV_DIGI = 1, RECV_IGATE = 2 };
static uint8_t aprsRecvKind[APRS_MAX];

// Freitext-Kommentar hinter Position+Symbol (Kurs/Tempo, Hoehe "/A=......", oder einfach ein Freitext wie
// der eigene Beacon ihn anhaengt, siehe aprsSendBeacon()) -- bisher von aprsStoreFromInfo() komplett
// verworfen, jetzt je Station gespeichert und in der Stationsliste per Trackballklick als Popup abrufbar
// (aprsListSel/aprsPopupShown, siehe handleInput()/altTrackUpdate()). 120 Byte sind reichlich: der rohe
// Empfangspuffer (aprsPoll(), buf[256]) deckt praktisch jedes realistische APRS-Infofeld schon ab, 120 Byte
// Kommentar allein liegen weit ueber dem, was Stationen in der Praxis an Freitext anhaengen.
#define APRS_COMMENT_LEN 120
static char aprsComment[APRS_MAX][APRS_COMMENT_LEN];
static int aprsListSel = -1;        // Raw-Index (nicht Sortierposition!) der markierten Station in der Liste;
                                     // -1 = keine Auswahl (noch nicht navigiert, oder Station inzwischen
                                     // aus der Liste gefallen -- wird dann beim naechsten Zugriff neu gesetzt)
static bool aprsPopupShown = false; // Zusatzinfo-Popup fuer aprsListSel gerade eingeblendet?
static int aprsListScroll = 0;      // Sortierposition (nicht Raw-Index) der obersten sichtbaren Zeile in der
                                     // Stationsliste -- altTrackUpdate() zieht das Fenster bei jedem Zeichnen so
                                     // nach, dass aprsListSel moeglichst mittig sichtbar bleibt (Bugfix Okt. 2026:
                                     // bisher wurde die Liste nie gescrollt, nur die obersten ca. 19 Zeilen
                                     // gezeichnet -- eine markierte Station, die durch neu hinzukommende juengere
                                     // Stationen nach unten aus dem sichtbaren Bereich rutscht, war dann komplett
                                     // unsichtbar und nur durch viele EV_UP-Schritte wieder erreichbar)

// Rohempfangs-Diagnose (unabhaengig davon, ob sich ein Paket als gueltige APRS-Position decodieren laesst):
// zaehlt JEDES vom Funkmodul mit korrektem CRC abgelieferte Paket, damit sich "kommt ueberhaupt etwas an"
// von "kommt an, aber unser Parser erkennt es nicht" unterscheiden laesst (siehe aprsPoll()).
static uint32_t aprsRxTotal = 0, aprsRxOk = 0;
static uint32_t aprsLastRxMs = 0;
static float aprsLastRssi = 0, aprsLastSnr = 0;

// Neuer Punkt an die Spur einer Station: mind. 20 m Abstand (sonst Haeufung im Stand), danach die
// Gesamtlaenge auf APRS_TRAIL_KM kappen (vom Ende rueckwaerts aufsummiert, der Rest vorne verworfen).
static void aprsTrailAdd(int s, double lat, double lon) {
  float *tl = aprsTrailLat[s], *tn = aprsTrailLon[s];
  int n = aprsTrailN[s];
  if (n > 0 && haversineKm(tl[n - 1], tn[n - 1], lat, lon) * 1000.0 < 20.0) return;
  if (n >= APRS_TRAIL_MAX) { for (int i = 1; i < n; i++) { tl[i - 1] = tl[i]; tn[i - 1] = tn[i]; } n--; }
  tl[n] = (float)lat; tn[n] = (float)lon; n++;
  double sum = 0.0;
  int from = n - 1;
  while (from > 0) {
    sum += haversineKm(tl[from - 1], tn[from - 1], tl[from], tn[from]);
    if (sum >= APRS_TRAIL_KM) break;
    from--;
  }
  if (from > 0) {
    int w = 0;
    for (int i = from; i < n; i++) { tl[w] = tl[i]; tn[w] = tn[i]; w++; }
    n = w;
  }
  aprsTrailN[s] = n;
}

// Station per Rufzeichen finden oder neu anlegen; ist der Puffer voll, wird die Station ersetzt, die am
// laengsten nicht mehr gehoert wurde (kleinstes aprsSeenMs) -- NICHT einfach immer Index 0 (das war ein
// Bug: Index 0 ist nach einer Weile Laufzeit keineswegs mehr "die aelteste Station", sondern nur die, die
// zufaellig als letztes neu angelegt wurde, solange sie seither nicht erneut gehoert wurde; dadurch wirkte
// die Stationsliste bei vollem Puffer so, als wuerde immer nur eine einzelne Zeile ueberschrieben, statt
// die am laengsten inaktive Station zu ersetzen).
// wasNew (optional, nullptr erlaubt): wird auf true gesetzt, wenn die Station NEU angelegt wurde (vorher
// nicht in der Liste), sonst auf false -- fuer den Unterscheidungston "neue Station" (direkt/repeated,
// siehe aprsStoreFromInfo()). Eine Station, die nur nach Verdraengung (voller Puffer) wiederverwendet wird,
// zaehlt hier ebenfalls als "neu", weil sie fuer den Nutzer gerade zum ersten Mal in der Liste auftaucht.
static int aprsFindOrAdd(const char *call, bool *wasNew) {
  for (int i = 0; i < aprsN; i++) if (strncmp(aprsCall[i], call, 9) == 0) { if (wasNew) *wasNew = false; return i; }
  int i;
  if (aprsN < APRS_MAX) {
    i = aprsN++;
  } else {
    i = 0;
    for (int k = 1; k < APRS_MAX; k++) if (aprsSeenMs[k] < aprsSeenMs[i]) i = k;
  }
  strncpy(aprsCall[i], call, 9);
  aprsCall[i][9] = 0;
  aprsTrailN[i] = 0;
  if (wasNew) *wasNew = true;
  return i;
}

// Station an Index i vollstaendig aus der Liste entfernen (nicht nur ueberschreibbar machen wie beim
// Eviction-Fall in aprsFindOrAdd() -- hier rutschen alle nachfolgenden Stationen eine Position nach vorn,
// damit aprsN wieder zur tatsaechlichen Anzahl passt). Nur von aprsPrune() aufgerufen.
static void aprsRemove(int i) {
  for (int k = i; k < aprsN - 1; k++) {
    strncpy(aprsCall[k], aprsCall[k + 1], 9);
    aprsCall[k][9] = 0;
    aprsLat[k] = aprsLat[k + 1];
    aprsLon[k] = aprsLon[k + 1];
    aprsSeenMs[k] = aprsSeenMs[k + 1];
    aprsRecvKind[k] = aprsRecvKind[k + 1];
    aprsTrailN[k] = aprsTrailN[k + 1];
    memcpy(aprsTrailLat[k], aprsTrailLat[k + 1], sizeof(aprsTrailLat[k]));
    memcpy(aprsTrailLon[k], aprsTrailLon[k + 1], sizeof(aprsTrailLon[k]));
    memcpy(aprsComment[k], aprsComment[k + 1], sizeof(aprsComment[k]));
  }
  aprsN--;
  // Markierte Station (Stationsliste, Taste Trackballklick) sitzt auf demselben Raw-Index -- beim Nachruecken
  // der nachfolgenden Stationen muss die Auswahl mitwandern, sonst zeigt sie nach einer Entfernung auf die
  // falsche (verschobene) Station bzw. ins Leere.
  if (aprsListSel == i) { aprsListSel = -1; aprsPopupShown = false; }
  else if (aprsListSel > i) { aprsListSel--; }
}

// Stationen, die laenger als APRS_MAX_AGE_MS (15 Minuten) nicht mehr gehoert wurden, komplett aus der Liste
// entfernen -- nicht erst beim naechsten Ueberschreiben bei vollem Puffer (aprsFindOrAdd()), sondern aktiv:
// so verschwindet eine Station spaetestens 15 Minuten nach ihrer letzten Meldung sowohl aus der Stationsliste
// (Geraet/Web, altTrackUpdate()/webAltStat()) als auch von der Karte (drawAprsStations()), da beide aus
// demselben Array lesen. Rueckwaerts durchlaufen, damit das Nachruecken in aprsRemove() die Laufindizes der
// noch zu pruefenden Stationen nicht verschiebt.
static void aprsPrune() {
  uint32_t now = millis();
  for (int i = aprsN - 1; i >= 0; i--) {
    if (now - aprsSeenMs[i] > APRS_MAX_AGE_MS) aprsRemove(i);
  }
}

// Freitext-Kommentar ab "offset" im rohen Infofeld lesen (hinter Position+Symbol, siehe aprsStoreFromInfo()):
// nicht druckbare Bytes (Bitfehler/abgeschnittener Satz) werden abgeschnitten statt als Muell angezeigt,
// aussen herum wird getrimmt. Leeres Ergebnis (zu kurzes Infofeld oder nur Leerzeichen) heisst "keine
// Zusatzinfo", siehe aprsListClickOrMove()/altTrackUpdate().
static void aprsExtractComment(const char *info, int offset, char *out, size_t outLen) {
  out[0] = 0;
  int len = (int)strlen(info);
  if (offset >= len) return;
  const char *p = info + offset;
  size_t n = 0;
  for (; p[n] && n < outLen - 1; n++) {
    if ((unsigned char)p[n] < 32 || (unsigned char)p[n] >= 127) break;   // Bitfehler/Muell: dort abschneiden
    out[n] = p[n];
  }
  out[n] = 0;
  while (n > 0 && out[n - 1] == ' ') out[--n] = 0;        // rechts trimmen
  size_t s = 0;
  while (out[s] == ' ') s++;                              // links trimmen
  if (s > 0) memmove(out, out + s, n - s + 1);
}

// Ausgewaehlte Station in der APRS-Stationsliste (dieselbe Sortierung "neueste zuerst" wie altTrackUpdate())
// um "dir" Positionen verschieben (+1 runter, -1 hoch, 0 nur gueltig machen ohne zu verschieben). Ohne
// gueltige Auswahl (Einstieg in die Liste, oder die Station ist seither herausgefallen) wird stattdessen
// die oberste (neueste) Station markiert -- "dir" wird in dem Fall ignoriert, der erste Tastendruck landet
// also immer ganz oben, statt eine Position von einer unbekannten Stelle aus zu verschieben.
static void aprsListMoveSel(int dir) {
  if (aprsN == 0) { aprsListSel = -1; return; }
  int idx[APRS_MAX];
  for (int i = 0; i < aprsN; i++) idx[i] = i;
  for (int i = 1; i < aprsN; i++) {
    int v = idx[i], j = i - 1;
    while (j >= 0 && aprsSeenMs[idx[j]] < aprsSeenMs[v]) { idx[j + 1] = idx[j]; j--; }
    idx[j + 1] = v;
  }
  int pos = -1;
  for (int k = 0; k < aprsN; k++) if (idx[k] == aprsListSel) { pos = k; break; }
  if (pos < 0) { aprsListSel = idx[0]; return; }
  pos = constrain(pos + dir, 0, aprsN - 1);
  aprsListSel = idx[pos];
}

// Bis zu maxLines Zeilen von je hoechstens maxChars Zeichen aus "text" schneiden, an Wortgrenzen (nicht
// mitten im Wort), fuer das Zusatzinfo-Popup der Stationsliste (altTrackUpdate()). Rueckgabe: Zeilenzahl.
static int wrapTextLines(const char *text, int maxChars, char lines[][48], int maxLines) {
  int n = 0;
  const char *p = text;
  while (*p && n < maxLines) {
    int len = (int)strlen(p);
    if (len <= maxChars) {
      strncpy(lines[n], p, 47); lines[n][47] = 0;
      n++;
      break;
    }
    int cut = maxChars;
    while (cut > 0 && p[cut] != ' ') cut--;
    if (cut == 0) cut = maxChars;                     // kein Leerzeichen gefunden: hart umbrechen
    int cpy = cut < 47 ? cut : 47;
    strncpy(lines[n], p, cpy); lines[n][cpy] = 0;
    n++;
    p += cut;
    while (*p == ' ') p++;
  }
  return n;
}

// ---------- GPX-Aufzeichnung (eigene Position und Sonde) ----------
// Eigene Position: <Datum_Uhrzeit>.gpx (Zeitpunkt des ersten GPS-Fix nach dem Einschalten, JJJJ-MM-TT_hhmmss,
// vorher wird noch nicht aufgezeichnet). Sonde: <Sondennummer>.gpx, eine eigene Datei je Sonden-ID -- taucht
// je Sonden-ID -- taucht dieselbe Sonde spaeter (auch nach einem Neustart) wieder auf, wird in dieser Datei
// mit einem neuen <trkseg> weitergeschrieben; eine andere Sonde bekommt eine eigene Datei.
// Anders als die Kartenspuren (trailAdd/ownTrailTick, mit Mindestabstand ausgeduennt) wird hier JEDE
// empfangene Position ungekuerzt gespeichert -- reine Aufzeichnung auf der SD-Karte, keine Anzeige.
//
// Punkte werden zunaechst in einem PSRAM-Puffer gesammelt (gpxQueue) und erst seltener wirklich auf die
// SD-Karte geschrieben (gpxDrain): alle GPX_FLUSH_MS automatisch, sofort wenn ein Puffer voll wird, sowie
// vor jedem Schliessen einer Spur (neue Sonde, neues Log). Taste U schreibt beide Puffer sofort weg --
// vor dem Ausschalten druecken, damit nichts verloren geht.
//
// Die Datei bleibt nach jedem geschriebenen Punkt gueltiges XML: neue Zeile wird an der zuvor gemerkten
// Position eingefuegt (seek + write, Modus "r+"), danach folgen sofort wieder die Schluss-Tags. Geflusht
// (physisch auf die Karte geschrieben) wird nur einmal je gpxDrain()-Aufruf, nicht pro Punkt.
// Bei jedem (Wieder-)Start bzw. bei einer neuen Sonde beginnt ein frisches <trkseg> innerhalb desselben <trk>.
static bool gpxOn = true;               // GPX-Tracks mitschreiben (Menue J)
#define GPX_FLUSH_MS 30000              // automatisch auf die SD schreiben, spaetestens alle 30 s
// Baut aus einer Sonden-ID einen dateinamensicheren String (nur Buchstaben/Ziffern); leer oder mit "?" -> "unbekannt"
static void gpxSafeId(const char *id, char *out, size_t n) {
  bool bad = !id || !id[0];
  size_t i = 0;
  for (; id && id[i] && i < 8; i++) if (!isalnum((unsigned char)id[i])) bad = true;
  if (bad) { strncpy(out, "unbekannt", n - 1); out[n - 1] = 0; return; }
  size_t cpy = i < n - 1 ? i : n - 1;
  memcpy(out, id, cpy);
  out[cpy] = 0;
}

// Setzt gpxIchId einmalig auf das aktuelle Datum, sobald die Uhr (per GPS) beim Einschalten erstmals steht.
// Vorher liefert die Funktion false, und es werden noch keine "Ich"-Punkte aufgezeichnet (der Dateiname
// braucht das Datum). Bleibt fuer den Rest der Betriebszeit unveraendert, auch falls es ueber Mitternacht geht.
static bool gpxEnsureIchId() {
  if (gpxIchId[0]) return true;
  if (!timeSet) return false;
  time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);
  snprintf(gpxIchId, sizeof(gpxIchId), "%04d-%02d-%02d_%02d%02d%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
           tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  snprintf(gpxIchName, sizeof(gpxIchName), "%04d-%02d-%02d %02d:%02d:%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
           tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  return true;
}

static void gpxTimeStr(time_t t, uint16_t ms, char *buf, size_t n) {   // UTC, ISO 8601 (GPX-Pflichtformat); leer, wenn t == 0
  if (!t) { buf[0] = 0; return; }
  struct tm tmv;
  gmtime_r(&t, &tmv);
  snprintf(buf, n, "%04d-%02d-%02dT%02d:%02d:%02d.%03uZ", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ms);
}

// Legt die Datei an (falls noetig), sucht das letzte "</trk>" und fuegt dort ein frisches, leeres
// <trkseg> ein. t.insertOffset zeigt danach auf die Stelle direkt vor dem zugehoerigen "</trkseg>".
static bool gpxOpenSegment(GpxTrack &t, const char *idStr, const char *trkName) {
  if (t.f) t.f.close();
  t.ok = false;
  char path[48];
  snprintf(path, sizeof(path), "%s/%s.gpx", GPX_DIR, idStr);
  if (!SD.exists(path)) {
    File nf = SD.open(path, FILE_WRITE);
    if (!nf) return false;
    char skel[192];
    int n = snprintf(skel, sizeof(skel),
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gpx version=\"1.1\" creator=\"T-Deck RS41\" "
      "xmlns=\"http://www.topografix.com/GPX/1/1\">\n<trk><name>%s</name>\n</trk>\n</gpx>\n", trkName);
    nf.write((const uint8_t *)skel, n);
    nf.close();
    touchFileTime(path);   // sonst bliebe die Datei dauerhaft auf dem SD-Anlege-Zeitstempel (1980) stehen, s. touchFileTime()
  }
  t.f = SD.open(path, "r+");
  if (!t.f) return false;
  uint32_t sz = t.f.size();
  uint32_t back = (sz > 300) ? sz - 300 : 0;
  t.f.seek(back);
  char buf[310];
  int rn = t.f.read((uint8_t *)buf, sz - back);
  if (rn <= 0) { t.f.close(); return false; }
  buf[rn] = 0;
  char *m = nullptr, *m2 = buf;               // letztes Vorkommen von "</trk>" im gelesenen Ausschnitt
  while ((m2 = strstr(m2, "</trk>")) != nullptr) { m = m2; m2++; }
  if (!m) { t.f.close(); return false; }
  uint32_t trkEnd = back + (uint32_t)(m - buf);
  static const char seg[] = "<trkseg>\n</trkseg>\n";
  t.f.seek(trkEnd);
  t.f.write((const uint8_t *)seg, sizeof(seg) - 1);
  t.insertOffset = trkEnd + 9;                // Laenge von "<trkseg>\n"
  t.ok = true;
  return true;
}

// Naechster Punkt faengt mit einem neuen <trkseg> an (Geraetestart, neue Sonde). Schreibt vorher alles
// noch Gepufferte weg, damit nichts an der Schnittstelle verloren geht.
static void gpxCloseTrack(GpxTrack &t) {
  if (t.f) t.f.close();
  t.ok = false;
  t.failed = false;
}

// Schreibt EINEN Punkt in die schon offene Datei (seek + write); flusht NICHT selbst -- das uebernimmt
// gpxDrain() einmal fuer den ganzen Stapel. timeStr darf leer/NULL sein (dann kein <time>-Tag).
static void gpxAddPointRaw(GpxTrack &t, const char *idStr, const char *trkName, double lat, double lon, float alt, const char *timeStr) {
  if (t.failed) return;
  if (!t.ok && !gpxOpenSegment(t, idStr, trkName)) { t.failed = true; return; }
  char line[160];
  int n = (timeStr && timeStr[0])
    ? snprintf(line, sizeof(line), "  <trkpt lat=\"%.6f\" lon=\"%.6f\"><ele>%.1f</ele><time>%s</time></trkpt>\n", lat, lon, alt, timeStr)
    : snprintf(line, sizeof(line), "  <trkpt lat=\"%.6f\" lon=\"%.6f\"><ele>%.1f</ele></trkpt>\n", lat, lon, alt);
  if (n <= 0 || !t.f.seek(t.insertOffset)) { t.failed = true; return; }
  t.f.write((const uint8_t *)line, n);
  t.insertOffset += (uint32_t)n;
  static const char tail[] = "</trkseg>\n</trk>\n</gpx>\n";
  t.f.write((const uint8_t *)tail, sizeof(tail) - 1);
}

// Reiht einen Punkt in den PSRAM-Puffer ein (kein SD-Zugriff); ist der Puffer voll, wird er sofort geleert.
static void gpxQueue(GpxTrack &t, GpxPoint *buf, int &count, const char *idStr, const char *trkName, double lat, double lon, float alt) {
  if (!gpxOn || !buf) return;
  if (count >= GPX_BUF_CAP) gpxDrain(t, buf, count, idStr, trkName);   // Puffer voll: sofort wegschreiben, dann weitermachen
  if (count >= GPX_BUF_CAP) return;   // gpxDrain konnte nicht schreiben (z. B. Datei nicht oeffenbar) -> Punkt verwerfen statt haengen bleiben
  buf[count].lat = (float)lat;
  buf[count].lon = (float)lon;
  buf[count].alt = alt;
  if (timeSet) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);                 // Sekunde UND Millisekunde, sonst fallen bei mehreren Punkten/Sekunde
    buf[count].t = tv.tv_sec;                    // (z. B. 2-Hz-GPS) alle auf dieselbe volle Sekunde -- siehe GpxPoint
    buf[count].ms = (uint16_t)(tv.tv_usec / 1000);
  } else {
    buf[count].t = 0;
    buf[count].ms = 0;
  }
  count++;
}

// Schreibt alle gepufferten Punkte einer Spur auf die SD-Karte und flusht einmal am Ende.
static void gpxDrain(GpxTrack &t, GpxPoint *buf, int &count, const char *idStr, const char *trkName) {
  if (count <= 0 || !buf) return;
  for (int i = 0; i < count; i++) {
    char tstr[28];                                // ".mmm" verlaengert den String gegenueber vorher um 4 Zeichen
    gpxTimeStr(buf[i].t, buf[i].ms, tstr, sizeof(tstr));
    gpxAddPointRaw(t, idStr, trkName, buf[i].lat, buf[i].lon, buf[i].alt, tstr);
  }
  if (t.f) t.f.flush();
  count = 0;
}

static void gpxDrainAll() {
  gpxDrain(gpxIch, gpxBufIch, gpxCountIch, gpxIchId, gpxIchName);
  gpxDrain(gpxSonde, gpxBufSonde, gpxCountSonde, gpxSondeId, gpxSondeName);
}

// Periodischer Push (alle GPX_FLUSH_MS) sowie Sofort-Push, sobald ein Puffer voll ist.
static void gpxTick() {
  if (gpxCountIch >= GPX_BUF_CAP) gpxDrain(gpxIch, gpxBufIch, gpxCountIch, gpxIchId, gpxIchName);
  if (gpxCountSonde >= GPX_BUF_CAP) gpxDrain(gpxSonde, gpxBufSonde, gpxCountSonde, gpxSondeId, gpxSondeName);
  if (millis() - gpxLastFlushMs >= GPX_FLUSH_MS) {
    gpxLastFlushMs = millis();
    gpxDrainAll();
  }
}


// ---------- Empfangston (I2S, eingebauter Lautsprecher) ----------
#define BEEP_RATE   16000                     // Abtastrate
#define BEEP_MS     150                       // Tondauer
#define BEEP_HZ     1800                      // Tonhoehe (normaler Empfangs-/Sendeton)
#define BEEP_HZ_NEU_DIREKT   2600              // neue Station DIREKT per Funk gehoert (hoeher)
#define BEEP_HZ_NEU_REPEATED  900              // neue Station ueber Digipeater (Repeater) gehoert (tiefer)
static bool beepReady = false;
static uint32_t beepLastMs = 0;               // letzter Blip im Sekundenmodus (beepSec >= 5)

static void beepInit() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = BEEP_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;
  cfg.tx_desc_auto_clear = true;          // WICHTIG: ohne das wiederholt der DMA den letzten Puffer endlos statt auf Stille zu gehen
  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL) != ESP_OK) { Serial.println("I2S: Installation fehlgeschlagen -> Empfangston bleibt stumm"); return; }
  i2s_pin_config_t pins = {};
  pins.bck_io_num = PIN_I2S_BCK;
  pins.ws_io_num = PIN_I2S_WS;
  pins.data_out_num = PIN_I2S_DOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) { Serial.println("I2S: Pins konnten nicht gesetzt werden -> Empfangston bleibt stumm"); return; }
  beepReady = true;
}

// Kurzer Sinuston mit sanfter Ein-/Ausblendung (kein Knacken), stereo, gleiches Signal auf beiden Kanaelen.
// hz waehlt die Tonhoehe -- so lassen sich mit derselben Klangerzeugung mehrere, am Klang unterscheidbare
// Toene erzeugen (normaler Empfangs-/Sendeton BEEP_HZ, neue Station direkt/repeated siehe aprsStoreFromInfo()).
static void beepPlayHz(int hz) {
  if (!beepReady) return;
  static int16_t buf[2 * (BEEP_RATE * BEEP_MS / 1000)];
  const int n = BEEP_RATE * BEEP_MS / 1000;
  const int ramp = n / 8;
  for (int i = 0; i < n; i++) {
    float env = (i < ramp) ? (float)i / ramp : ((i > n - ramp) ? (float)(n - i) / ramp : 1.0f);
    int16_t v = (int16_t)(env * 24000.0f * sinf(2.0f * (float)M_PI * hz * i / BEEP_RATE));
    buf[2 * i] = v; buf[2 * i + 1] = v;
  }
  size_t written;
  i2s_write(I2S_NUM_0, buf, sizeof(buf), &written, pdMS_TO_TICKS(60));
  i2s_zero_dma_buffer(I2S_NUM_0);           // sofort auf Stille, statt auf den naechsten Unterlauf zu warten
}
static void beepPlay() { beepPlayHz(BEEP_HZ); }     // normaler Empfangs-/Sendeton (bisheriges Verhalten)

static void ownTrailTick() {
  if (!ownTrailOn || !gps.fix || millis() - gps.lastFix > 5000) return;
  if (ownN > 0 && haversineKm(ownLat[ownN - 1], ownLon[ownN - 1], gps.lat, gps.lon) * 1000.0 < ownMinDist) return;
  if (ownN >= OWN_MAX) {
    int from = trailRecentFrom(ownLat, ownLon, ownN, KEEP_RECENT_KM);
    int n2 = trailCompressOld(ownLat, ownLon, ownN, from);
    if (n2 >= OWN_MAX) {            // Notfall (extrem dicht/verrauscht): ganzen Puffer nachverdichten
      for (int i = 0; i < OWN_MAX / 2; i++) { ownLat[i] = ownLat[2 * i]; ownLon[i] = ownLon[2 * i]; }
      n2 = OWN_MAX / 2;
      ownMinDist *= 2.0f;
    }
    ownN = n2;
  }
  ownLat[ownN] = (float)gps.lat;
  ownLon[ownN] = (float)gps.lon;
  ownN++;
}
static uint8_t gfxView = 0;                     // 0 = Karte, 1 = Hoehentrack/Stationen (gilt, wenn mapMode = true)
static bool mapIdleShown = false;               // Die Karte zeigt gerade das Ruhebild (ohne Sonde)
static bool gfxReady() { return gfxView == 0 ? (bgCanvas && frameCanvas) : (frameCanvas != nullptr); }   // Bildpuffer vorhanden?
static uint32_t lastPosOkMs = 0;                // Zeitpunkt der letzten GUELTIGEN Position (Schritt 2/0x7B erfolgreich);
                                                 // bestimmt jetzt die eigentliche Fuellfarbe des Sondenpunkts

// Im Sekundenmodus (beepSec 5..20): waehrend des Empfangs alle beepSec Sekunden ein Blip, unabhaengig von einzelnen Paketen
static void beepTick() {
  if (beepSec < 5 || !lastRxMs || millis() - lastRxMs > 5000) return;
  if (millis() - beepLastMs < (uint32_t)beepSec * 1000UL) return;
  beepLastMs = millis();
  beepPlay();
}
static uint32_t lastMapMs = 0;                  // Dauer des letzten Kartenupdates (0 = keins in diesem Frame)
static double viewZ = -1;                       // Zoom des Hintergrunds (seit FW 3.0 immer eine ganze Stufe); -1 = neu rendern erzwingen
static double viewCx = 0, viewCy = 0;           // Weltpixel der Bildmitte (bei Zoom viewZ)
// Manuelle Ansicht: A/D zoomt, Trackball scrollt. Solange mapManual = false, waehlt die Karte Zoom und Mitte selbst.
static bool mapManual = false;
static double manZoom = MAP_IDLE_Z;             // Zoom im manuellen Modus
static double manLat = 0, manLon = 0;           // Kartenmitte im manuellen Modus
static double shownZ = -1, shownCx = 0, shownCy = 0;   // zuletzt gerenderter Hintergrund (Ausgangspunkt fuer A/D und Trackball)
static double zoomPend = 0;                     // noch nicht angewendete Zoomtasten (mehrere schnelle Tastendruecke = ein Neuzeichnen)
static uint32_t zoomPendMs = 0;
static int panAccX = 0, panAccY = 0;            // Trackball-Impulse seit der letzten Karten-Aktualisierung (rechts/unten positiv)

// Diagnose "Trackball scrollt ruckelig" (Okt. 2026): wird in mapNavTick() beim Ausloesen eines Trackball-Pans
// scharf gestellt und von renderBackground()/blit() wieder geloescht, sobald das dadurch ausgeloeste Neuzeichnen
// fertig ist. Nur WAEHREND dieses Fensters wird geloggt (gezielt auf Serial, nicht SD -- "zu langsam"), damit
// im Log ausschliesslich die fuer einen Pan-Vorgang relevanten Zeiten stehen und nicht bei jedem x-beliebigen
// Bildaufbau (Menue, Idle-Karte, ...) mitgeschrieben wird.
static bool panLogArm = false;
static uint32_t panLogT0 = 0;
static char viewIdSeen[9] = "";                 // Sonden-ID, fuer die die Karte zuletzt auf automatisch gestellt wurde
static uint8_t rFocus = 0;                      // automatische Ansicht (R): 0 Sonde+Ich+Landung, 1 Sonde+Ich, 2 nur Sonde, 3 nur Ich
static uint8_t aprsFocus = 0;                   // automatische Ansicht im LoRa-APRS-Modus (R): 0 nur Ich, 1 Ich+alle
                                                 // Stationen, 2 Ich+alle DIREKT (nicht per Fremdverkehr) gehoerten
                                                 // Stationen -- eigene Zaehlung statt rFocus, da dort keine Sonde existiert
// Einzelstations-Fokus aus der Stationsliste (Taste R bei eingeblendetem Zusatzinfo-Popup, siehe
// handleInput()): Rohindex (nicht Sortierposition) der Station, auf die die Karte zusaetzlich zu "Ich"
// eingezoomt wird -- unabhaengig vom normalen aprsFocus-Zyklus (0/1/2). -1 = kein Einzelstations-Fokus aktiv.
static int aprsSoloIdx = -1;
static bool forceRecenter = false;              // R auf Einzelpunkt (nur Sonde/nur Ich): naechstes Bild neu zentrieren, Zoom unveraendert
static double soloZoom = -1;                    // eigener, getrennter Zoom fuer "nur Sonde"/"nur Ich" (-1 = noch keiner gemerkt);
                                                 // sonst wuerde ein zwischendurch berechneter Mehrpunkt-Zoom (Sonde+Ich) den
                                                 // zuvor fuer den Einzelpunkt gewaehlten Zoom beim Zurueckwechseln ueberschreiben
volatile bool clickReq = false;
volatile uint16_t tbU = 0, tbD = 0, tbL = 0, tbR = 0;   // Trackball-Impulse seit der letzten Abfrage
static int pendingKeyChar = 0;                  // von pollKeyboardRaw() zwischengespeicherte Taste: das Tastatur-IC haelt nur
                                                 // die jeweils letzte Taste, ein langes Kartenrendern wuerde sie sonst verschlucken

void IRAM_ATTR onButton() {                     // Trackball-Klick (GPIO0)
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last > 300) { clickReq = true; last = now; }
}
void IRAM_ATTR isrUp()    { tbU = tbU + 1; }
void IRAM_ATTR isrDown()  { tbD = tbD + 1; }
void IRAM_ATTR isrLeft()  { tbL = tbL + 1; }
void IRAM_ATTR isrRight() { tbR = tbR + 1; }

// ---------- Batterie ----------
// Der Ladestand kommt aus der Spannung an GPIO4. Ein Statussignal des Ladechips ist nicht angeschlossen (nur eine LED),
// darum wird "laedt" geschaetzt: Spannung ab BAT_USB_V (USB/Ladespannung erkannt) oder ueber eine Minute steigend.
static float batV = 0;                 // geglaettete Batteriespannung [V]
static int batPct = -1;                // Ladestand in %, -1 = unbekannt (USB-Spannung liegt an)
static bool batUsb = false;            // USB/Ladespannung erkannt
static bool batChg = false;            // laedt (Annahme)
static bool batTrendChg = false;       // Spannung steigt (ueber mind. 2 Minutenfenster in Folge bestaetigt)
static int batRiseStreak = 0;          // aufeinanderfolgende Minutenfenster mit Spannungsanstieg, siehe batteryTick()
static uint32_t batLastMs = 0, batRefMs = 0;
static float batRefV = 0;

// Die letzten beiden Stuetzstellen (10%@3.69V, 5%@3.61V, 0%@3.27V) waren viel zu grob: 3.61-3.27V sind 340 mV
// fuer nur 5 Prozentpunkte, waehrend der Rest der Kurve ca. 20-60 mV je 5 Punkte braucht. Ein Geraet, das
// irgendwo in diesem Bereich steht (z. B. 3.50V), zeigte dadurch ueber eine Stunde lang unveraendert "3-4%"
// an, obwohl die Spannung spuerbar weiter sank -- sah wie "haengengeblieben" aus. Jetzt bis 0% genauso fein
// gestuft wie der Rest der Kurve.
// Die unteren Stufen (frueher 2%@3.51/1%@3.45/0%@3.27) stammen jetzt aus einem echten Durchlauf bis zum
// tatsaechlichen Abschalten (16.10.2026, Minuten-Log): das Geraet lief ab 3.27 V (altes "0%") noch ueber
// 49 Minuten lang voellig normal weiter (GPS-Fix, LoRa-TX, Display) und ist erst bei ca. 2,51 V wirklich
// ausgegangen -- "0%" stand also fast eine Stunde lang auf dem Display, obwohl noch satt Laufzeit uebrig
// war ("die Anzeige pendelt bei 3-4%, geht aber nicht aus"). Neue Stufen 2%/1%/0% jetzt anhand der in diesem
// Lauf GEMESSENEN Restlaufzeit gesetzt (3.49 V -> noch ca. 283 min, 3.41 V -> ca. 141 min, 3.35 V -> ca.
// 70 min, 2.90 V -> nur noch ca. 8 min bis zum echten Ende) statt einer generischen LiPo-Kurve: "0%" kommt
// dadurch spuerbar spaeter und bedeutet jetzt wirklich "gleich ist Schluss" statt "noch bis zu einer Stunde Zeit".
static int batPercent(float v) {       // typische LiPo-Entladekurve, unten per Messung kalibriert (siehe oben)
  static const float tv[26] = {4.20f, 4.15f, 4.11f, 4.08f, 4.02f, 3.98f, 3.95f, 3.91f, 3.87f, 3.85f, 3.84f,
                               3.82f, 3.80f, 3.79f, 3.77f, 3.75f, 3.73f, 3.71f, 3.69f, 3.65f, 3.61f, 3.57f,
                               3.54f, 3.49f, 3.35f, 2.90f};
  static const int tp[26] = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 8, 6, 4, 3, 2, 1, 0};
  if (v >= tv[0]) return 100;
  if (v <= tv[25]) return 0;
  for (int i = 0; i < 25; i++)
    if (v <= tv[i] && v > tv[i + 1])
      return (int)(tp[i + 1] + (tp[i] - tp[i + 1]) * (v - tv[i + 1]) / (tv[i] - tv[i + 1]) + 0.5f);
  return 0;
}

// Batteriesymbol mit Prozentzahl (oder "USB") und Blitz beim Laden; funktioniert auf Display und Bildpuffer
static void drawBattery(Adafruit_GFX *g, int x, int y) {
  if (batV <= 0) return;
  g->fillRect(x - 2, y - 1, 322 - x, 12, ST77XX_BLACK);
  g->drawRect(x, y, 22, 10, ST77XX_WHITE);
  g->fillRect(x + 22, y + 3, 2, 4, ST77XX_WHITE);
  uint16_t col = (batPct < 0) ? ST77XX_CYAN : (batPct > 50 ? ST77XX_GREEN : (batPct > 20 ? ST77XX_YELLOW : ST77XX_RED));
  int w = (batPct < 0) ? 18 : (batPct * 18 + 50) / 100;
  if (w > 0) g->fillRect(x + 2, y + 2, w, 6, col);
  if (batChg) {                                        // Blitz
    g->drawLine(x + 13, y + 2, x + 9, y + 5, ST77XX_WHITE);
    g->drawLine(x + 9, y + 5, x + 13, y + 5, ST77XX_WHITE);
    g->drawLine(x + 13, y + 5, x + 9, y + 8, ST77XX_WHITE);
  }
  char t[8];
  if (batPct < 0) snprintf(t, sizeof(t), "USB");
  else snprintf(t, sizeof(t), "%d%%", batPct);
  g->setTextSize(1);
  g->setTextColor(ST77XX_WHITE);
  g->setCursor(x + 27, y + 1);
  g->print(t);
}

// GPS-Anzeige: gruen = Fix (Zahl = genutzte Satelliten), gelb = NMEA-Daten aber kein Fix, rot = keine Daten vom GPS
static void drawGps(Adafruit_GFX *g, int x, int y) {
  bool alive = gps.bytes > 0 && millis() - gps.lastByteMs < 3000;
  bool fixNow = gps.fix && millis() - gps.lastFix < 5000;
  uint16_t col = !alive ? ST77XX_RED : (fixNow ? ST77XX_GREEN : ST77XX_YELLOW);
  char t[12];
  if (alive) snprintf(t, sizeof(t), "GPS %d", gps.sats);
  else snprintf(t, sizeof(t), "GPS --");
  g->fillRect(x - 2, y, 322 - x, 9, ST77XX_BLACK);
  g->setTextSize(1);
  g->setTextColor(col);
  g->setCursor(x, y);
  g->print(t);
  if (wifiOn || wifiConnecting) {                      // "W" (cyan): WLAN laeuft, "w" (gelb): verbindet gerade
    g->setTextColor(wifiOn ? ST77XX_CYAN : ST77XX_YELLOW);
    g->setCursor(x + 40, y);
    g->print(wifiOn ? "W" : "w");
  }
}

// Statusblock oben rechts: Batterie und GPS
static void drawStatus(Adafruit_GFX *g, int x, int y) {
  drawBattery(g, x, y);
  drawGps(g, x, y + 12);
}

static void statusRefresh() {                          // Statusblock im Bildpuffer neu zeichnen und aufs Display kopieren
  drawStatus(scr(), 264, 1);
  scrFlush(262, 0, 58, 24);
}

// Alle 3 s messen (16 Werte gemittelt, geglaettet), im Textbild das Symbol aktualisieren
static void batteryTick() {
  uint32_t now = millis();
  if (batLastMs && now - batLastMs < 3000) return;
  batLastMs = now;
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(PIN_BAT_ADC);
  float v = (mv / 16.0f) * BAT_SCALE / 1000.0f + BAT_OFFSET;
  batV = (batV <= 0) ? v : (batV * 0.7f + v * 0.3f);
  batUsb = batV >= BAT_USB_V;
  if (!batRefMs || now - batRefMs > 60000) {           // Trend ueber je eine Minute
    if (batRefMs) {
      float dv = batV - batRefV;
      if (dv >= 0.010f) {
        // Ein einzelnes 1-Minuten-Fenster mit Anstieg ist noch kein verlaessliches Lade-Indiz -- ADC-Rauschen
        // bzw. Temperaturdrift kann das auch bei eigentlich flacher/leicht fallender Spannung kurz vortaeuschen
        // (ein Log zeigte dadurch ueber 15 Minuten am Stueck faelschlich "laedt", obwohl nichts angeschlossen
        // war). Erst beim ZWEITEN Fenster in Folge mit Anstieg gilt die Ladung als bestaetigt.
        batRiseStreak++;
        if (batRiseStreak >= 2) batTrendChg = true;
      } else if (dv <= -0.005f) {
        batRiseStreak = 0;
        batTrendChg = false;
      } else {
        batRiseStreak = 0;        // weder eindeutig steigend noch fallend: Anstiegsserie unterbrochen
      }
    }
    batRefV = batV;
    batRefMs = now;
  }
  batChg = batUsb || batTrendChg;
  batPct = batUsb ? -1 : batPercent(batV);
  if (!mapMode || !gfxReady()) statusRefresh();                     // sonst zeichnet die Ansicht den Statusblock selbst
}

// Batteriezustand jede Minute ins Log schreiben (vorher alle 5 Minuten -- fuer einen Entladetest bis zum
// automatischen Abschalten soll sich der Spannungsverlauf feiner nachvollziehen lassen), unabhaengig davon,
// ob gerade eine Sonde empfangen wird (die Bat..V/..%-Angabe stand bisher nur in der Frame-Zeile pro
// empfangenem Sondenpaket und fehlte damit komplett in Phasen ohne Sondenempfang, z. B. auf der Anfahrt).
static void batteryLogTick() {
  static uint32_t lastLog = 0;
  uint32_t now = millis();
  if (lastLog && now - lastLog < 60000UL) return;
  lastLog = now;
  if (batV <= 0) return;
  char tt[12], m[64];
  timeText(tt, sizeof(tt));
  snprintf(m, sizeof(m), "t%lu %s Bat%.2fV/%d%%%s", (unsigned long)(now / 1000), tt, batV, batPct, batChg ? "+" : "");
  logLine(m);
}

// Hinweis am Bildschirm (Toast) fuer kurze Rueckmeldungen
static char toastText[48] = "";
static uint32_t toastUntil = 0;
static void drawToast(GFXcanvas16 *c) {
  if (!toastUntil || millis() >= toastUntil) return;
  c->fillRect(0, 0, 262, 12, ST77XX_BLACK);
  c->setTextSize(1);
  c->setTextColor(ST77XX_YELLOW);
  c->setCursor(2, 2);
  c->print(toastText);
}

// Einfache Landevorhersage aus Sinkrate, Geschwindigkeit und Richtung (nur beim Sinken):
// Die Sinkrate waechst mit der Hoehe wie 1/sqrt(Luftdichte); Zeit bis zum Boden = Summe ueber 50-m-Schritte,
// die Horizontalgeschwindigkeit (Betrag und Richtung) wird als konstant angenommen (Wind nach Hoehe unbekannt).
static void predictLanding() {
  predValid = false;
  if (!predOn || !haveVel || !haveSondePos) return;
  double vz0 = -(double)predVz;                         // geglaettete Geschwindigkeit statt des einzelnen letzten Fixes
  double h = sondeAlt - landAlt;
  if (vz0 < PRED_MIN_SINK || h < 30.0) return;
  double vh = sqrt((double)predVe * predVe + (double)predVn * predVn);
  double dir = fmod(atan2((double)predVe, (double)predVn) * 180.0 / M_PI + 360.0, 360.0);
  const double H2 = 2.0 * 7000.0;                       // 2 x Skalenhoehe der Atmosphaere [m]
  double T = 0.0, z = sondeAlt;
  while (z > landAlt && T < 36000.0) {
    double step = fmin(50.0, z - landAlt);
    double zm = z - step / 2.0;
    T += step / (vz0 * exp((zm - sondeAlt) / H2));
    z -= step;
  }
  double dist = vh * T;
  double a = dir * M_PI / 180.0;
  double newLat = sondeLat + dist * cos(a) / 110540.0;
  double newLon = sondeLon + dist * sin(a) / (111320.0 * cos(sondeLat * M_PI / 180.0));
  if (predPtInit) {                                     // Landepunkt selbst zusaetzlich daempfen: ein Ausreisser
    predLat += PRED_SMOOTH_OUT_ALPHA * (newLat - predLat);   // verschiebt das Fadenkreuz dann nur teilweise,
    predLon += PRED_SMOOTH_OUT_ALPHA * (newLon - predLon);   // nicht sofort auf die volle Distanz
  } else {
    predLat = newLat; predLon = newLon; predPtInit = true;
  }
  predEtaS = (uint32_t)T;
  predValid = true;
}

// ---------- Online-Landevorhersage (Tawhiri/SondeHub-API) ----------
// Ergaenzt predictLanding() (oben) um eine zweite, auf echten Windvorhersagedaten (GFS) basierende Vorhersage --
// dafuer muss das T-Deck im Heimnetz mit echtem Internetzugang sein (Menue W; der eigene Zugangspunkt hat
// keinen Internetzugang). Dieselbe API (api.v2.sondehub.org/tawhiri, Tawhiri-kompatibel), die auch
// predict.sondehub.org benutzt. Laeuft IMMER (auch im Steigflug) -- zwei Faelle:
//  - Sonde sinkt schon (predVz < 0): wie bisher der Trick "Start = aktuelle Sondenposition, Burst-Hoehe nur
//    knapp darueber", sodass der simulierte Aufstieg sofort endet und nur der (echte, gemessene) Sinkflug
//    zaehlt.
//  - Sonde steigt noch: ECHTE Steigrate (predVz) und die aktuelle Position als Start, dazu eine grobe
//    Hausnummer fuer die noch unbekannte Platz- und spaetere Sinkrate (TAWHIRI_BURST_ALT_DEFAULT_M /
//    TAWHIRI_FALLBACK_VZ0) -- Tawhiri simuliert dann den Rest des Aufstiegs bis dahin und danach den
//    Sinkflug mit echten Winddaten. Das ist nur eine grobe erste Schaetzung, wird aber automatisch genauer,
//    sobald die Sonde wirklich platzt und zu sinken beginnt (dann greift der erste Fall mit echten Werten).
// Takt: alle TAWHIRI_RETRY_MS (5 min), aber alle TAWHIRI_RETRY_FAST_MS (1 min), sobald die Sonde unter
// TAWHIRI_FAST_ALT_M (3000 m) UND dabei tatsaechlich im Sinkflug ist (dann zaehlt jede Minute).
#define TAWHIRI_URL              "https://api.v2.sondehub.org/tawhiri"
#define TAWHIRI_RETRY_MS         300000UL     // normal alle 5 Minuten eine neue Online-Vorhersage versuchen
#define TAWHIRI_RETRY_FAST_MS     60000UL     // unterhalb TAWHIRI_FAST_ALT_M im Sinkflug: alle 1 Minute
#define TAWHIRI_FAST_ALT_M         3000.0     // Hoehenschwelle fuer den schnelleren Takt
#define TAWHIRI_BURST_ALT_DEFAULT_M 30000.0   // Schaetzwert fuer die Platzhoehe, solange die Sonde noch steigt
                                               // (die echte Platzhoehe ist erst nach dem Platzen bekannt) --
                                               // typischer Bereich fuer Wetterballone, nur eine grobe Hausnummer
#define TAWHIRI_FALLBACK_VZ0          5.0     // Schaetzwert fuer die Sinkrate NACH dem (noch nicht erfolgten)
                                               // Platzen [m/s, auf Meereshoehe bezogen, wie von der API erwartet]
                                               // -- auch nur eine Hausnummer, wird durch die echte Rate ersetzt,
                                               // sobald die Sonde wirklich sinkt
#define TAWHIRI_MAX_SONDE_AGE_MS 300000UL     // keine Anfrage mehr, wenn seit > 5 Minuten kein Paket mehr kam

static bool onlinePredValid = false;
static double onlinePredLat = 0, onlinePredLon = 0;    // Landepunkt (letzter Punkt des Abstiegs, siehe jsonLastNumber())
static uint32_t onlinePredMs = 0;             // millis() des letzten erfolgreichen Abrufs
static uint32_t onlinePredNextMs = 0;         // naechster erlaubter Versuch

// Vermutliche Flugstrecke des Abstiegs (Okt. 2026, statt nur einer geraden Linie zum Landepunkt): bis zu
// ONLINE_PRED_PATH_MAX gleichmaessig ueber den Abstieg verteilte Punkte, siehe parseOnlinePredPath().
// onlinePredPathN < 2 heisst "keine Strecke geparst" -- die Aufrufer zeichnen dann ersatzweise nur die
// Gerade zum Landepunkt (onlinePredLat/Lon), wie bisher.
#define ONLINE_PRED_PATH_MAX 48
static float onlinePredPathLat[ONLINE_PRED_PATH_MAX];
static float onlinePredPathLon[ONLINE_PRED_PATH_MAX];
static int onlinePredPathN = 0;

// Letztes Vorkommen von key (z. B. "\"latitude\":") im JSON -- die Antwort listet Aufstieg dann Abstieg,
// je Punkt alphabetisch altitude/datetime/latitude/longitude, daher liefert das letzte Vorkommen von
// "latitude"/"longitude" genau den Landepunkt (letzter Punkt des Abstiegs).
static bool jsonLastNumber(const char *json, const char *key, double &val) {
  const char *p = json, *last = nullptr;
  size_t klen = strlen(key);
  while ((p = strstr(p, key)) != nullptr) { last = p; p += klen; }
  if (!last) return false;
  val = atof(last + klen);
  return true;
}

// Sammelt die vermutliche Flugstrecke des Abstiegs aus der Tawhiri-Antwort: die Antwort listet zwei Stufen
// (Aufstieg, dann Abstieg) mit je einem eigenen "trajectory"-Array -- das LETZTE Vorkommen von "trajectory"
// ist also der Beginn des Abstiegs (siehe jsonLastNumber() oben zum selben Trick). Davon ab werden alle
// "latitude"/"longitude"-Paare eingesammelt (je Punkt in dieser Reihenfolge, siehe jsonLastNumber()); bei mehr
// Punkten als ONLINE_PRED_PATH_MAX wird gleichmaessig ausseduennt (Index 0 und letzter Punkt immer dabei).
// Rueckgabe false: Antwortformat nicht wie erwartet -- Aufrufer zeichnen dann nur die Gerade zum Landepunkt.
static bool parseOnlinePredPath(const char *json) {
  const char *p = json, *descentStart = nullptr;
  size_t klen = strlen("\"trajectory\":");
  while ((p = strstr(p, "\"trajectory\":")) != nullptr) { descentStart = p; p += klen; }
  if (!descentStart) return false;
  int total = 0;                                      // 1. Durchlauf: wie viele Punkte gibt es ueberhaupt?
  for (p = descentStart; (p = strstr(p, "\"latitude\":")) != nullptr; total++) p += 11;
  if (total < 2) return false;
  int stride = (total + ONLINE_PRED_PATH_MAX - 1) / ONLINE_PRED_PATH_MAX;
  if (stride < 1) stride = 1;
  onlinePredPathN = 0;
  p = descentStart;
  int idx = 0;
  while ((p = strstr(p, "\"latitude\":")) != nullptr) {
    const char *latP = p + 11;
    const char *lonP = strstr(latP, "\"longitude\":");
    if (!lonP) break;
    lonP += 12;
    if (((idx % stride) == 0 || idx == total - 1) && onlinePredPathN < ONLINE_PRED_PATH_MAX) {
      onlinePredPathLat[onlinePredPathN] = (float)atof(latP);
      onlinePredPathLon[onlinePredPathN] = (float)atof(lonP);
      onlinePredPathN++;
    }
    p = lonP;
    idx++;
  }
  return onlinePredPathN >= 2;
}

static void onlinePredictLanding() {
  char dt[24];
  {
    time_t t = time(nullptr);
    struct tm tmv;
    gmtime_r(&t, &tmv);
    snprintf(dt, sizeof(dt), "%04d-%02d-%02dT%02d:%02d:%02dZ", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  }
  double ascentRate, burstAlt, seaLevelDescent;
  if (predVz < 0) {                                         // sinkt schon: Trick -- Aufstieg sofort "fertig",
                                                              // echte (gemessene) Sinkrate zaehlt
    double vz0 = -(double)predVz;                            // Sinkrate an der aktuellen Hoehe (wie predictLanding())
    seaLevelDescent = vz0 * exp(-sondeAlt / 14000.0);         // auf Meereshoehe umgerechnet -- so erwartet es die API
    ascentRate = 1.0;
    burstAlt = sondeAlt + 50.0;
  } else {                                                   // steigt noch: echte Steigrate, Rest nur Schaetzung
    ascentRate = fmax(0.5, (double)predVz);                  // API verlangt > 0
    burstAlt = TAWHIRI_BURST_ALT_DEFAULT_M;
    seaLevelDescent = TAWHIRI_FALLBACK_VZ0;
  }
  char url[300];
  snprintf(url, sizeof(url),
           "%s?launch_latitude=%.6f&launch_longitude=%.6f&launch_altitude=%.0f&launch_datetime=%s"
           "&ascent_rate=%.2f&burst_altitude=%.0f&descent_rate=%.2f&profile=standard_profile",
           TAWHIRI_URL, sondeLat, sondeLon, sondeAlt, dt, ascentRate, burstAlt, seaLevelDescent);
  NetworkClientSecure client;
  client.setInsecure();                                    // keine Zertifikatspruefung, wie bei der Fundmeldung
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(client, url)) { logLine("Online-Vorhersage: Verbindung fehlgeschlagen"); return; }
  http.addHeader("User-Agent", "tdeck-rs41");
  int code = http.GET();
  if (code != 200) {
    char m[48];
    snprintf(m, sizeof(m), "Online-Vorhersage: HTTP %d", code);
    logLine(m);
    http.end();
    return;
  }
  String resp = http.getString();
  http.end();
  double la, lo;
  if (resp.indexOf("\"error\"") >= 0 || !jsonLastNumber(resp.c_str(), "\"latitude\":", la) ||
      !jsonLastNumber(resp.c_str(), "\"longitude\":", lo)) {
    logLine("Online-Vorhersage: Antwort nicht auswertbar");
    return;
  }
  onlinePredLat = la;
  onlinePredLon = lo;
  onlinePredMs = millis();
  onlinePredValid = true;
  if (!parseOnlinePredPath(resp.c_str())) onlinePredPathN = 0;   // Rueckfall: nur die Gerade zum Landepunkt
  char m[64];
  snprintf(m, sizeof(m), "Online-Vorhersage: %.5f %.5f (%d Streckenpunkte)", la, lo, onlinePredPathN);
  logLine(m);
}

// Periodischer Hintergrundabruf: nur mit echtem Internet (Heimnetz), solange ueberhaupt eine Sondenposition
// mit Geschwindigkeit vorliegt und kuerzlich empfangen wurde -- NICHT mehr an predictLanding()/predValid
// (also "sinkt gerade") gekoppelt, laeuft also durchgehend, auch im Steigflug (siehe Fallback-Sinkrate oben).
// Takt: alle TAWHIRI_RETRY_MS, aber alle TAWHIRI_RETRY_FAST_MS, sobald die Sonde unter TAWHIRI_FAST_ALT_M
// UND tatsaechlich im Sinkflug ist (predVz < 0) -- dann soll jede Minute zaehlen.
static void onlinePredTick() {
  if (!(wifiOn && wifiMode == 1)) return;
  if (!timeSet || !haveVel || !haveSondePos) return;
  if (!lastRxMs || millis() - lastRxMs > TAWHIRI_MAX_SONDE_AGE_MS) return;
  if ((int32_t)(millis() - onlinePredNextMs) < 0) return;
  bool fastZone = (sondeAlt < TAWHIRI_FAST_ALT_M) && (predVz < 0);
  onlinePredNextMs = millis() + (fastZone ? TAWHIRI_RETRY_FAST_MS : TAWHIRI_RETRY_MS);
  onlinePredictLanding();
}

// Fadenkreuz: nur Kreuz aus zwei 1-Pixel-Linien, Halbe Laenge = CROSS_HALF
static void drawCrosshair(GFXcanvas16 *c, int x, int y, uint16_t col) {
  c->drawLine(x - CROSS_HALF, y, x + CROSS_HALF, y, col);
  c->drawLine(x, y - CROSS_HALF, x, y + CROSS_HALF, col);
}

// Linie mit schwarzem "Lichthof" (je 1 px schwarz auf beiden Seiten, dann die eigentliche Linie in der Mitte):
// bleibt dadurch auf JEDER Kartenfarbe (auch gelb auf hellem Untergrund) gut sichtbar, siehe COL_PRED_ONLINE.
static void drawHaloLine(GFXcanvas16 *c, int x0, int y0, int x1, int y1, uint16_t col) {
  bool flat = abs(x1 - x0) >= abs(y1 - y0);
  int ox = flat ? 0 : 1, oy = flat ? 1 : 0;
  c->drawLine(x0 - ox, y0 - oy, x1 - ox, y1 - oy, ST77XX_BLACK);
  c->drawLine(x0 + ox, y0 + oy, x1 + ox, y1 + oy, ST77XX_BLACK);
  c->drawLine(x0, y0, x1, y1, col);
}

// Fadenkreuz mit schwarzem Lichthof, siehe drawHaloLine() -- die Vorgaenger-Version hat den schwarzen Anteil
// nur in Linienrichtung verlaengert (also fast komplett unter der farbigen Linie versteckt) statt ihn wie bei
// drawHaloLine() QUER zur Linie zu versetzen; dadurch war am Landekreuz praktisch kein Lichthof zu sehen.
// Die Version danach (nur die beiden Seiten je Arm als Linie versetzt) liess an den Spitzen offen -- dort
// blieb der Lichthof optisch "weg", nur an der Kreuzmitte wirkte er umrandet. Jetzt: voller schwarzer Rahmen
// je Arm inkl. Enden (fillRect etwas groesser als der Arm), danach der farbige Arm obendrauf -- wie bei den
// Spuren, die komplett umrandet sind.
static void drawCrosshairHalo(GFXcanvas16 *c, int x, int y, uint16_t col) {
  c->fillRect(x - CROSS_HALF - 1, y - 1, 2 * CROSS_HALF + 3, 3, ST77XX_BLACK);   // waagerechter Arm, rundum schwarz
  c->fillRect(x - 1, y - CROSS_HALF - 1, 3, 2 * CROSS_HALF + 3, ST77XX_BLACK);   // senkrechter Arm, rundum schwarz
  drawCrosshair(c, x, y, col);
}

// Duenner Ladebalken ganz am unteren Bildschirmrand, waehrend ein blockierendes Kacheln-Neuzeichnen laeuft
// (Zoom, Kartenart-Wechsel, grosses Pan-Neuzentrieren -- siehe renderBackground()/bigRefillStart()): wird direkt
// aufs Display geschrieben, nicht in frameCanvas/bgCanvas/bigCanvas, weil die ja gerade erst gefuellt werden.
// Der naechste normale blit() nach Abschluss des Renderns ueberschreibt ihn automatisch mit dem fertigen Bild --
// kein eigenes "wieder wegraeumen" noetig.
static void drawMapRenderProgress(int cur, int total) {
  const int y = 236, h = 4;
  tft.fillRect(0, y, 320, h, ST77XX_BLACK);
  int w = (total > 0) ? constrain((cur * 320) / total, 0, 320) : 0;
  if (w > 0) tft.fillRect(0, y, w, h, ST77XX_RED);
}

// Hintergrund (Kacheln) fuer viewZ / viewCx / viewCy in bgCanvas rendern (immer eine ganze Zoomstufe, seit
// FW 3.0 kein Kommazahl-/Zwischen-Zoom mehr -- siehe Hinweis bei PAN_DIR oben).
static void renderBackground() {
  uint32_t t0 = millis();
  uint16_t *bg = bgCanvas->getBuffer();
  bgCanvas->fillScreen(0x4208);                 // dunkelgrau fuer fehlende Kacheln
  int zi = (int)floor(viewZ + 0.5);
  if (zi < MAP_MIN_Z) zi = MAP_MIN_Z;
  if (zi > MAP_MAX_Z) zi = MAP_MAX_Z;
  double left = viewCx - 160.0, top = viewCy - 120.0;
  long tx0 = (long)floor(left / 256.0), tx1 = (long)floor((left + 319.0) / 256.0);
  long ty0 = (long)floor(top / 256.0),  ty1 = (long)floor((top + 239.0) / 256.0);
  long maxT = (1L << zi) - 1;
  int missing = 0;
  int total = (int)(ty1 - ty0 + 1) * (int)(tx1 - tx0 + 1), done = 0;
  for (long ty = ty0; ty <= ty1; ty++)
    for (long tx = tx0; tx <= tx1; tx++) {
      if (tx < 0 || ty < 0 || tx > maxT || ty > maxT) { done++; continue; }
      tgtBuf = bg; tgtW = 320; tgtH = 240;
      int sx = (int)floor(tx * 256.0 - left + 0.5), sy = (int)floor(ty * 256.0 - top + 0.5);
      if (!drawTileStyled(zi, tx, ty, sx, sy)) missing++;
      drawMapRenderProgress(++done, total);   // Ladebalken am unteren Rand, siehe dort
      pumpDuringRender();          // Kartenrendern kann pro Kachel etliche ms dauern (SD-Zugriff/JPEG):
                                    // Taste zwischenspeichern und Webserver (Log-Download!) bedienen, statt alles zu blockieren
    }
  tgtBuf = nullptr; tgtW = 320; tgtH = 240;
  shownZ = viewZ; shownCx = viewCx; shownCy = viewCy;
  VLOG("Karte: Zoom %.2f (Kacheln z%d) gerendert in %lu ms, %d Kachel(n) fehlen\n", viewZ, zi, (unsigned long)(millis() - t0), missing);
  if (panLogArm) {   // Pan-Diagnose: Dauer des Kachel-Renderns, unabhaengig von LOG_LEVEL/VLOG (siehe panLogArm)
    Serial.printf("PAN render: %lu ms, %d Kachel(n) fehlen\n", (unsigned long)(millis() - t0), missing);
  }
}

// ---------- Grosser Kartenhintergrund mit Rand (schnelles Trackball-Scrollen) ----------
// Idee (Okt. 2026, nach der Diagnose mit den "PAN ..."-Logzeilen): bgCanvas/renderBackground() oben rendern
// IMMER nur genau den Bildschirmausschnitt -- jeder Pan-Schritt, egal wie klein, dekodiert alle sichtbaren
// Kacheln neu (~1,4 s). Stattdessen wird hier ein groesserer Puffer (bigCanvas) mit Rand ringsum gehalten;
// solange der Bildschirmausschnitt innerhalb dieses Randes bleibt, kostet ein Pan-Schritt nur noch einen
// Speicher-Crop (siehe prepareMapBackground(), <5 ms). Der Rand wird dabei kachelweise im Hintergrund
// nachgezogen (eine Kachel pro loop()-Durchlauf, bigRefillTick()), bevor er tatsaechlich ausgeht.

// Liefert true und die Rasterposition (r, c) in bigTileLoaded[][], wenn die Kachel (tx, ty) innerhalb des
// aktuell fuer bigCanvas benutzten Kachelrasters liegt (Ursprung bigTileBaseTx/Ty).
static inline bool bigTileSlot(long tx, long ty, int *r, int *c) {
  long cc = tx - bigTileBaseTx, rr = ty - bigTileBaseTy;
  if (cc < 0 || rr < 0 || cc >= BIG_TILES_MAX || rr >= BIG_TILES_MAX) return false;
  *c = (int)cc; *r = (int)rr;
  return true;
}
static inline void bigMarkTileLoaded(long tx, long ty) {
  int r, c;
  if (bigTileSlot(tx, ty, &r, &c)) bigTileLoaded[r][c] = true;
}

// (Neu) zentriert bigCanvas auf (cx, cy) bei Zoomstufe zi: die gerade sichtbaren Kacheln (Bildschirm um cx/cy)
// werden SOFORT gerendert (blockierend, wie bisher renderBackground() -- das muss weiterhin schnell genug sein,
// damit gleich ein Bild da ist), der Rest des Randes kommt in die Warteschlange fuer bigRefillTick().
static void bigRefillStart(int zi, double cx, double cy) {
  if (!bigCanvas || bigW <= 0) return;
  uint32_t t0 = millis();
  bigZ = (double)zi; bigCx = cx; bigCy = cy; bigStyle = (int)mapStyle;
  memset(bigTileLoaded, 0, sizeof(bigTileLoaded));
  double left = cx - bigW / 2.0, top = cy - bigH / 2.0;
  long tx0 = (long)floor(left / 256.0), tx1 = (long)floor((left + bigW - 1.0) / 256.0);
  long ty0 = (long)floor(top / 256.0),  ty1 = (long)floor((top + bigH - 1.0) / 256.0);
  bigTileBaseTx = tx0; bigTileBaseTy = ty0;
  long maxT = (1L << zi) - 1;
  double sLeft = cx - 160.0, sTop = cy - 120.0;      // der gerade sichtbare Bildschirmausschnitt um (cx, cy)
  long stx0 = (long)floor(sLeft / 256.0), stx1 = (long)floor((sLeft + 319.0) / 256.0);
  long sty0 = (long)floor(sTop / 256.0),  sty1 = (long)floor((sTop + 239.0) / 256.0);
  bigCanvas->fillScreen(0x4208);                      // dunkelgrau fuer (noch) fehlende Kacheln, wie bisher bgCanvas
  bigRefillZi = zi;
  int priCount = 0, missing = 0;
  int priTotal = (int)(sty1 - sty0 + 1) * (int)(stx1 - stx0 + 1);
  for (long ty = sty0; ty <= sty1; ty++)              // 1. sofort: alles, was gerade sichtbar ist
    for (long tx = stx0; tx <= stx1; tx++) {
      if (tx < 0 || ty < 0 || tx > maxT || ty > maxT) { priCount++; drawMapRenderProgress(priCount, priTotal); continue; }
      tgtBuf = bigCanvas->getBuffer(); tgtW = bigW; tgtH = bigH;
      int sx = (int)floor(tx * 256.0 - left + 0.5), sy = (int)floor(ty * 256.0 - top + 0.5);
      if (!drawTileStyled(zi, tx, ty, sx, sy)) missing++;
      bigMarkTileLoaded(tx, ty);
      priCount++;
      drawMapRenderProgress(priCount, priTotal);      // Ladebalken am unteren Rand, siehe dort
      pumpDuringRender();
    }
  bigRefillCount = 0;
  for (long ty = ty0; ty <= ty1 && bigRefillCount < BIG_REFILL_MAX_TILES; ty++)   // 2. der Rand: in die Warteschlange
    for (long tx = tx0; tx <= tx1 && bigRefillCount < BIG_REFILL_MAX_TILES; tx++) {
      if (tx < 0 || ty < 0 || tx > maxT || ty > maxT) continue;
      if (tx >= stx0 && tx <= stx1 && ty >= sty0 && ty <= sty1) continue;   // schon oben erledigt
      bigRefillTx[bigRefillCount] = (int16_t)tx;
      bigRefillTy[bigRefillCount] = (int16_t)ty;
      bigRefillSx[bigRefillCount] = (int16_t)floor(tx * 256.0 - left + 0.5);
      bigRefillSy[bigRefillCount] = (int16_t)floor(ty * 256.0 - top + 0.5);
      bigRefillCount++;
    }
  bigRefillIdx = 0;
  tgtBuf = nullptr; tgtW = 320; tgtH = 240;
  if (panLogArm) {
    Serial.printf("PAN render (big neu zentriert): %lu ms sofort (%d Kachel(n), %d fehlen), %d im Hintergrund vorgemerkt\n",
                  (unsigned long)(millis() - t0), priCount, missing, bigRefillCount);
  }
}

// Laedt hoechstens EINE vorgemerkte Randkachel nach -- wird aus handleInput() bei jedem loop()-Durchlauf
// aufgerufen, ist aber fast immer ein No-op (Warteschlange leer), solange nicht gerade frisch zentriert wurde.
static void bigRefillTick() {
  if (bigRefillIdx >= bigRefillCount || !bigCanvas) return;
  int i = bigRefillIdx++;
  uint32_t t0 = millis();
  tgtBuf = bigCanvas->getBuffer(); tgtW = bigW; tgtH = bigH;
  drawTileStyled(bigRefillZi, bigRefillTx[i], bigRefillTy[i], bigRefillSx[i], bigRefillSy[i]);
  bigMarkTileLoaded(bigRefillTx[i], bigRefillTy[i]);
  tgtBuf = nullptr; tgtW = 320; tgtH = 240;
  VLOG("BigRand: Kachel %d/%d nachgeladen in %lu ms\n", i + 1, bigRefillCount, (unsigned long)(millis() - t0));
}

// true, wenn der Bildschirmausschnitt um (cx, cy) bei Zoomstufe zi komplett innerhalb des aktuell gueltigen
// bigCanvas liegt UND alle dafuer noetigen Kacheln schon geladen sind (Rand reicht also noch aus).
static bool bigTilesReady(int zi, double cx, double cy) {
  if (!bigCanvas || bigW <= 0 || (int)bigZ != zi || bigStyle != (int)mapStyle) return false;
  double left = cx - 160.0, top = cy - 120.0;
  double bigLeft = bigCx - bigW / 2.0, bigTop = bigCy - bigH / 2.0;
  if (left < bigLeft || left + 320.0 > bigLeft + bigW || top < bigTop || top + 240.0 > bigTop + bigH) return false;
  long maxT = (1L << zi) - 1;
  long tx0 = (long)floor(left / 256.0), tx1 = (long)floor((left + 319.0) / 256.0);
  long ty0 = (long)floor(top / 256.0),  ty1 = (long)floor((top + 239.0) / 256.0);
  for (long ty = ty0; ty <= ty1; ty++)
    for (long tx = tx0; tx <= tx1; tx++) {
      if (tx < 0 || ty < 0 || tx > maxT || ty > maxT) continue;
      int r, c;
      if (!bigTileSlot(tx, ty, &r, &c) || !bigTileLoaded[r][c]) return false;
    }
  return true;
}

// true, wenn der Rand auf einer Seite schon so knapp ist (< 128 px = 1/2 Kachel), dass bigCanvas rechtzeitig
// neu zentriert werden sollte, BEVOR bigTilesReady() tatsaechlich fehlschlaegt (vorausschauendes Nachladen).
static bool bigMarginLow(double cx, double cy) {
  if (!bigCanvas || bigW <= 0) return true;
  double bigLeft = bigCx - bigW / 2.0, bigTop = bigCy - bigH / 2.0;
  double mL = (cx - 160.0) - bigLeft, mR = (bigLeft + bigW) - (cx + 160.0);
  double mT = (cy - 120.0) - bigTop,  mB = (bigTop + bigH) - (cy + 120.0);
  return mL < 128.0 || mR < 128.0 || mT < 128.0 || mB < 128.0;
}

// Ersetzt "if (mapViewSetup()) renderBackground()" + die bisherige volle Speicherkopie nach frameCanvas: holt
// den Kartenhintergrund fuer den aktuellen Ausschnitt (viewZ/viewCx/viewCy) direkt in frameCanvas. Ueber
// bigCanvas, wenn moeglich (schneller Crop); sonst (bigCanvas nicht allozierbar, zu wenig PSRAM) wie bisher
// ueber renderBackground()/bgCanvas.
static void prepareMapBackground(bool viewChanged) {
  int zi = (int)floor(viewZ + 0.5);
  if (!bigCanvas || bigW <= 0) {                      // Rueckfall: alter, randloser Weg
    if (viewChanged) renderBackground();              // setzt shownZ/shownCx/shownCy selbst
    memcpy(frameCanvas->getBuffer(), bgCanvas->getBuffer(), 320UL * 240UL * 2UL);
    return;
  }
  // shownZ/shownCx/shownCy sind die "zuletzt gezeichneter Hintergrund"-Werte, auf denen mapNavTick()/A-D/
  // Trackball aufbauen (siehe dort: "noch keine Karte gezeichnet"-Abbruch bei shownZ < 0). Die werden bisher
  // nur von renderBackground() gesetzt -- hier (schneller Crop-Pfad) MUSS das genauso passieren, sonst bleibt
  // shownZ dauerhaft auf dem Startwert -1 stehen und jeder Pan wird von mapNavTick() sofort verworfen, obwohl
  // die Karte laengst zu sehen ist (genauer: der Trackball wird zwar abgefragt, aber nie angewendet).
  shownZ = viewZ; shownCx = viewCx; shownCy = viewCy;
  if (!bigTilesReady(zi, viewCx, viewCy)) {
    bigRefillStart(zi, viewCx, viewCy);               // Rand reicht nicht (mehr) -- jetzt neu zentrieren (kostet wie frueher)
  } else if (bigMarginLow(viewCx, viewCy)) {
    bigRefillStart(zi, viewCx, viewCy);               // Rand wird knapp -- rechtzeitig nachziehen, bevor er ausgeht
  }
  uint32_t ct0 = millis();                            // ab hier: nur noch Speicher-Crop aus bigCanvas, kein Kachel-Dekodieren
  double left = viewCx - 160.0, top = viewCy - 120.0;
  double bigLeft = bigCx - bigW / 2.0, bigTop = bigCy - bigH / 2.0;
  int ox = (int)floor(left - bigLeft + 0.5), oy = (int)floor(top - bigTop + 0.5);
  const uint16_t *src = bigCanvas->getBuffer();
  uint16_t *dst = frameCanvas->getBuffer();
  for (int row = 0; row < 240; row++) {
    int sr = oy + row, sc = ox;
    if (sr < 0 || sr >= bigH || sc < 0 || sc + 320 > bigW) {   // sollte dank bigTilesReady() oben nicht vorkommen
      memset(dst + row * 320, 0, 320 * 2);                     // -- sicherheitshalber trotzdem abfangen statt Muell zu zeigen
      continue;
    }
    memcpy(dst + row * 320, src + (size_t)sr * bigW + sc, 320 * 2);
  }
  if (panLogArm) {
    Serial.printf("PAN render (crop): %lu ms\n", (unsigned long)(millis() - ct0));
  }
}

// Zoomwert, bei dem alle Punkte in ein Rechteck von w x h Pixeln passen (Kommazahl)
static double fitZoom(const double *plat, const double *plon, int np, double w, double h) {
  double x0 = 1e18, x1 = -1e18, y0 = 1e18, y1 = -1e18;
  for (int i = 0; i < np; i++) {
    double px, py;
    latLonToPixel(plat[i], plon[i], 0.0, px, py);
    if (px < x0) x0 = px;
    if (px > x1) x1 = px;
    if (py < y0) y0 = py;
    if (py > y1) y1 = py;
  }
  double z = 99.0;
  if (x1 - x0 > 1e-12) z = fmin(z, log2(w / (x1 - x0)));
  if (y1 - y0 > 1e-12) z = fmin(z, log2(h / (y1 - y0)));
  return z;
}

// Wie fitZoom(), aber der Mittelpunkt liegt FEST auf (homeLat, homeLon) statt auf der Mitte der Bounding-Box --
// fuer den LoRa-Modus, wo "Ich" beim Fokus "Ich+alle/direkt gehoerte Stationen" immer im Bildzentrum bleiben
// soll, auch wenn die Stationen ungleichmaessig um einen herum verteilt sind (siehe mapViewSetup()).
static double fitZoomCentered(double homeLat, double homeLon, const double *plat, const double *plon, int np, double w, double h) {
  double hx0, hy0;
  latLonToPixel(homeLat, homeLon, 0.0, hx0, hy0);
  double maxDx = 0, maxDy = 0;
  for (int i = 0; i < np; i++) {
    double px, py;
    latLonToPixel(plat[i], plon[i], 0.0, px, py);
    double dx = fabs(px - hx0), dy = fabs(py - hy0);
    if (dx > maxDx) maxDx = dx;
    if (dy > maxDy) maxDy = dy;
  }
  double z = 99.0;
  if (maxDx > 1e-12) z = fmin(z, log2((w / 2.0) / maxDx));
  if (maxDy > 1e-12) z = fmin(z, log2((h / 2.0) / maxDy));
  return z;
}

// Zoomwert, der wirklich gezeichnet wird: seit FW 3.0 immer eine ganze Stufe (siehe Hinweis bei PAN_DIR oben)
static double effZoom(double z) {
  if (z < (double)MAP_MIN_Z) z = (double)MAP_MIN_Z;
  if (z > (double)MAP_MAX_Z) z = (double)MAP_MAX_Z;
  z = floor(z + 0.5);
  return z;
}

// Legt Zoom und Bildmitte fest. plat/plon: Punkte, die im Automatikbetrieb im Bild sein muessen (np beliebig,
// z. B. bis zu 1 + APRS_MAX fuer "Ich + alle Stationen" im LoRa-Modus, siehe idleMapUpdate()). centerOnFirst:
// Bildmitte bleibt FEST auf plat[0]/plon[0] (statt auf der Mitte der Bounding-Box aller Punkte) -- fuer den
// LoRa-Modus, wo "Ich" (immer Index 0, siehe idleMapUpdate()) beim Fokus "Ich+alle/direkt gehoerte Stationen"
// im Bildzentrum bleiben soll, egal wie die Stationen um einen herum verteilt sind.
// Rueckgabe true: Hintergrund muss neu gerendert werden (viewZ / viewCx / viewCy sind dann neu gesetzt).
static bool mapViewSetup(const double *plat, const double *plon, int np, bool idle, bool tight = false, bool centerOnFirst = false) {
  double z, cx, cy;
  if (mapManual) {                              // manuell: Zoom und Mitte kommen von A/D und vom Trackball
    z = effZoom(manZoom);
    latLonToPixel(manLat, manLon, z, cx, cy);
    if (z == viewZ && fabs(cx - viewCx) < 0.5 && fabs(cy - viewCy) < 0.5) return false;
    viewZ = z; viewCx = cx; viewCy = cy;
    return true;
  }
  double zt = (viewZ >= (double)MAP_MIN_Z) ? viewZ : (double)MAP_IDLE_Z;   // Einzelpunkt (nur Sonde/nur Ich): bisherigen
                                                                            // Zoom beibehalten, nur beim allerersten Bild MAP_IDLE_Z
  if (np > 1) {                                 // sonst der groesste Zoom, bei dem alle Punkte in 240 x 140 Pixel passen
    zt = centerOnFirst ? fitZoomCentered(plat[0], plon[0], plat, plon, np, 240.0, 140.0)
                        : fitZoom(plat, plon, np, 240.0, 140.0);
    if (zt > (double)MAP_MAX_Z) zt = (double)MAP_MAX_Z;
    if (zt < (double)MAP_MIN_Z) zt = (double)MAP_MIN_Z;
  }
  double zq = floor(zt + 1e-6);                 // seit FW 3.0 immer eine ganze Zoomstufe, nach unten abgerundet: passt sicher
  // Zoom nur aendern, wenn es sein muss (Punkte passen nicht mehr) oder sich deutlich lohnt (0,35 mehr): kein staendiges Neurendern
  bool zOut = (viewZ > zt + 1e-9);              // Punkte passen nicht mehr: herauszoomen
  bool zChange = (viewZ < 0) || zOut || (zq >= viewZ + 0.35);
  z = zChange ? zq : viewZ;
  // Bis zu 1 + APRS_MAX Punkte (Ich + alle LoRa-Stationen, siehe idleMapUpdate()); Sondenmodus braucht nie mehr als 3.
  static const int MVS_MAX_PTS = 1 + APRS_MAX;
  int npc = (np < MVS_MAX_PTS) ? np : MVS_MAX_PTS;
  double px[MVS_MAX_PTS], py[MVS_MAX_PTS];
  double bx0 = 1e18, bx1 = -1e18, by0 = 1e18, by1 = -1e18;
  for (int i = 0; i < npc; i++) {
    latLonToPixel(plat[i], plon[i], z, px[i], py[i]);
    if (px[i] < bx0) bx0 = px[i];
    if (px[i] > bx1) bx1 = px[i];
    if (py[i] < by0) by0 = py[i];
    if (py[i] > by1) by1 = py[i];
  }
  bool rerender = (z != viewZ);
  if (!rerender) {                              // Hintergrund nur neu, wenn ein Punkt zu nah am Rand liegt
    // Naeher als 200 m an der Sonde (tight=true, siehe mapUpdate): kein Toleranzband mehr, jedes Mal exakt neu
    // zentrieren. Das kostet mehr Kachel-Neuzeichnungen, ist aber genau da gewollt, wo es auf jeden Meter ankommt
    // (kurz vor der Landung); weiter weg bleibt die bisherige, sparsame Nachfuehrung am Rand unveraendert.
    if (tight) {
      rerender = true;
    } else {
      double ox = viewCx - 160.0, oy = viewCy - 120.0;
      double xl = idle ? 60 : 14, xr = idle ? 260 : 306, yt = idle ? 50 : 14, yb = idle ? 150 : 192;
      for (int i = 0; i < npc; i++)
        if (px[i] - ox < xl || px[i] - ox > xr || py[i] - oy < yt || py[i] - oy > yb) rerender = true;
    }
  }
  if (rerender) {
    viewZ = z;
    if (centerOnFirst) { viewCx = px[0]; viewCy = py[0]; }    // Ich (Index 0) bleibt im Bildzentrum
    else { viewCx = (bx0 + bx1) / 2.0; viewCy = (by0 + by1) / 2.0; }
  }
  return rerender;
}

// Von der automatischen zur manuellen Ansicht wechseln: aktuellen Ausschnitt als Ausgangspunkt uebernehmen.
// "reason" wird nur beim tatsaechlichen Umschalten (false -> true) geloggt, damit sich spaeter im Log
// nachvollziehen laesst, was die manuelle Ansicht ausgeloest hat (Zoomtaste/-eingabe oder Trackball).
static void enterManual(const char *reason) {
  if (mapManual) return;
  mapManual = true;
  char m[48];
  snprintf(m, sizeof(m), "Karte: manuell (%s)", reason);
  logLine(m);
  if (shownZ > 0) {
    manZoom = shownZ;
    pixelToLatLon(shownCx, shownCy, shownZ, manLat, manLon);
  } else {
    manZoom = (double)MAP_IDLE_Z;
    homePosition(manLat, manLon);
  }
}

// Eigenen Track (mit schwarzem Lichthof, siehe drawHaloLine()) bis zur aktuellen Position (endX, endY) in
// den Bildpuffer zeichnen
static void drawOwnTrail(int endX, int endY) {
  if (!ownTrailOn || ownN < 1) return;
  double ox = viewCx - 160.0, oy = viewCy - 120.0;
  int px0 = 0, py0 = 0;
  for (int i = 0; i < ownN; i++) {
    double tx, ty;
    latLonToPixel(ownLat[i], ownLon[i], viewZ, tx, ty);
    int x = (int)floor(tx - ox), y = (int)floor(ty - oy);
    if (i > 0 && !((px0 < 0 && x < 0) || (px0 > 319 && x > 319) || (py0 < 0 && y < 0) || (py0 > 239 && y > 239)))
      drawHaloLine(frameCanvas, px0, py0, x, y, COL_OWN_TRAIL);
    px0 = x; py0 = y;
  }
  drawHaloLine(frameCanvas, px0, py0, endX, endY, COL_OWN_TRAIL);
}

// Einen Pixelwert um 50% Richtung Schwarz abdunkeln (= jeden Farbkanal halbieren, einzeln maskiert, damit
// beim Schieben keine Bits zwischen R/G/B-Feldern ueberlaufen) -- Grundlage fuer den halbtransparenten
// schwarzen Beschriftungshintergrund in drawAprsStations() (weisse Schrift bleibt so auch auf hellem
// Kartenuntergrund lesbar, ohne ihn komplett zu verdecken).
static uint16_t blend50Black(uint16_t px) {
  uint16_t r = (px >> 11) & 0x1F, g = (px >> 5) & 0x3F, b = px & 0x1F;
  return (uint16_t)(((r >> 1) << 11) | ((g >> 1) << 5) | (b >> 1));
}

// Rechteck mit 50% schwarzem Overlay fuellen (liest dafuer jedes Pixel einzeln aus dem Bildpuffer und
// schreibt es abgedunkelt zurueck, siehe blend50Black()) -- nur fuer kleine Bereiche (Beschriftungen) gedacht.
static void fillRectBlend50Black(GFXcanvas16 *c, int x, int y, int w, int h) {
  for (int yy = y; yy < y + h; yy++) {
    if (yy < 0 || yy > 239) continue;
    for (int xx = x; xx < x + w; xx++) {
      if (xx < 0 || xx > 319) continue;
      c->drawPixel(xx, yy, blend50Black(c->getPixel(xx, yy)));
    }
  }
}

// LoRa-APRS-Stationen auf der Karte: Punkt + Rufzeichen (auf halbtransparentem schwarzem Hintergrund,
// siehe fillRectBlend50Black()) + verkuerzte Spur (siehe aprsTrailAdd()). Punktfarbe nach Empfangsart
// (aprsRecvKind[], siehe COL_APRS_*): blau = direkt per Funk, orange = per Digipeater-Hop weitergereicht,
// rot = als Fremdverkehr ueber ein iGate/APRS-IS hereingekommen. Nur im LoRa-Modus aufgerufen
// (radioMode==1) -- die Sonde wird in diesem Modus nirgends gezeichnet.
static void drawAprsStations() {
  double ox = viewCx - 160.0, oy = viewCy - 120.0;
  for (int s = 0; s < aprsN; s++) {
    if (aprsTrailN[s] > 1) {
      int px0 = 0, py0 = 0;
      for (int i = 0; i < aprsTrailN[s]; i++) {
        double tx, ty;
        latLonToPixel(aprsTrailLat[s][i], aprsTrailLon[s][i], viewZ, tx, ty);
        int x = (int)floor(tx - ox), y = (int)floor(ty - oy);
        if (i > 0) drawHaloLine(frameCanvas, px0, py0, x, y, COL_TRAIL);
        px0 = x; py0 = y;
      }
    }
    double px, py;
    latLonToPixel(aprsLat[s], aprsLon[s], viewZ, px, py);
    int x = (int)floor(px - ox), y = (int)floor(py - oy);
    uint16_t dotCol = (aprsRecvKind[s] == RECV_IGATE) ? COL_APRS_IGATE
                     : (aprsRecvKind[s] == RECV_DIGI) ? COL_APRS_DIGI : COL_APRS_DIRECT;
    frameCanvas->fillCircle(x, y, 5, COL_SONDE_RING);
    frameCanvas->fillCircle(x, y, 4, dotCol);
    drawEdgeArrow(frameCanvas, x, y, dotCol);
    int tw = (int)strlen(aprsCall[s]) * 6;              // Standardfont: 6 px Vorschub je Zeichen bei textSize 1
    fillRectBlend50Black(frameCanvas, x + 5, y - 5, tw + 2, 10);
    frameCanvas->setTextSize(1);
    frameCanvas->setTextColor(ST77XX_WHITE);
    frameCanvas->setCursor(x + 6, y - 4);
    frameCanvas->print(aprsCall[s]);
  }
}

// Pfeil am Bildrand, wenn ein Punkt ausserhalb des sichtbaren Kartenbereichs liegt (zeigt in seine Richtung)
static void drawEdgeArrow(GFXcanvas16 *c, int x, int y, uint16_t col) {
  if (x >= 4 && x <= 315 && y >= 12 && y <= 200) return;
  double dx = x - 160.0, dy = y - 106.0;
  double len = sqrt(dx * dx + dy * dy);
  if (len < 1.0) return;
  double ux = dx / len, uy = dy / len;
  double t = 1e9;
  if (fabs(dx) > 1e-6) t = fmin(t, 148.0 / fabs(dx));
  if (fabs(dy) > 1e-6) t = fmin(t, 86.0 / fabs(dy));
  int px = 160 + (int)(dx * t), py = 106 + (int)(dy * t);
  int tx = px + (int)(ux * 9.0), ty = py + (int)(uy * 9.0);
  int ax = px - (int)(uy * 6.0), ay = py + (int)(ux * 6.0);
  int bx = px + (int)(uy * 6.0), by = py - (int)(ux * 6.0);
  c->fillTriangle(tx, ty, ax, ay, bx, by, col);
  c->drawTriangle(tx, ty, ax, ay, bx, by, ST77XX_BLACK);
}

static void blit(GFXcanvas16 *c) {
  uint32_t bt0 = panLogArm ? millis() : 0;   // Pan-Diagnose: Dauer des SPI-Transfers zum Display, siehe panLogArm
  tft.startWrite();
  tft.setAddrWindow(0, 0, 320, 240);
  tft.writePixels(c->getBuffer(), 320UL * 240UL);
  tft.endWrite();
  if (panLogArm) {
    Serial.printf("PAN blit: %lu ms -- gesamt (Ausloeser bis Bildschirm): %lu ms\n",
                  (unsigned long)(millis() - bt0), (unsigned long)(millis() - panLogT0));
    panLogArm = false;   // dieses Fenster ist fertig; naechster Trackball-Pan stellt neu scharf
  }
}

// Karte mit (lat, lon) in der Mitte anzeigen (Kartentest)
static void showCentered(double lat, double lon, int z) {
  if (!bgCanvas) { Serial.println("Kein Bildpuffer (PSRAM?) -> keine Karte"); return; }
  double px, py;
  latLonToPixel(lat, lon, z, px, py);
  viewZ = z; viewCx = px; viewCy = py;
  renderBackground();
  bgCanvas->fillCircle(160, 120, 6, ST77XX_WHITE);
  bgCanvas->fillCircle(160, 120, 4, ST77XX_RED);
  bgCanvas->fillRect(0, 230, 320, 10, ST77XX_BLACK);
  bgCanvas->setTextColor(ST77XX_WHITE);
  bgCanvas->setCursor(2, 231);
  bgCanvas->print(String("z") + z + "  " + String(lat, 5) + "  " + String(lon, 5));
  blit(bgCanvas);
  viewZ = -1;                                   // fuer die Kartenansicht neu rendern
}

// Schwelle fuer die "grosse Schrift"-Statuszeile auf der Kartenseite: schneller als Gehen/Stehen (z. B. im Auto
// oder zuegig mit dem Rad) -- dann zaehlen nur noch wenige, grosse, aus der Ferne/vom Lenker ablesbare Werte;
// unterhalb bleibt die gewohnte dreizeilige Detailanzeige in Kleinschrift unveraendert.
#define OWN_SPEED_BIG_KMH 8.0f

// Testhilfe: erzwingt die "grosse Schrift"-Statuszeile ohne tatsaechlich schnell zu fahren (Taste N, siehe
// handleInput()). -1 = aus (normales GPS-Tempo gilt), sonst der Testwert in km/h. Bewusst nur im RAM, kein
// Prefs-Eintrag und kein logLine()-Aufruf beim Umschalten -- taucht also nirgends im SD-Log auf.
static float dbgSpeedOverride = -1.0f;
static inline float effSpeedKmh() { return dbgSpeedOverride >= 0.0f ? dbgSpeedOverride : gps.speedKmh; }

// Pfeil fuer Steigen/Sinken neben der Steig-/Sinkrate: nach oben (gruen) bei > 1 m/s, nach unten (orange) bei
// < -1 m/s, sonst ein waagerechter Strich ("eben"). x = linker Rand, yc = vertikale Mitte der Pfeilflaeche.
static void drawVzArrow(Adafruit_GFX *g, int x, int yc, float vz) {
  if (vz > 1.0f) g->fillTriangle(x, yc + 7, x + 14, yc + 7, x + 7, yc - 7, ST77XX_GREEN);
  else if (vz < -1.0f) g->fillTriangle(x, yc - 7, x + 14, yc - 7, x + 7, yc + 7, COL_TRAIL);
  else g->fillRect(x, yc - 1, 14, 2, ST77XX_WHITE);
}

// Grosse Statuszeile ohne Sondenempfang (schneller als OWN_SPEED_BIG_KMH, aber noch keine Sonde diese Sitzung):
// nur die eigene Geschwindigkeit und Hoehe, groesser als die sonst dort stehenden Detailzeilen.
static void drawBigOwnStatus(Adafruit_GFX *g, float speedKmh, float alt) {
  g->fillRect(0, 205, 320, 35, ST77XX_BLACK);
  g->setTextWrap(false);
  g->setTextSize(2);
  g->setTextColor(ST77XX_WHITE);
  char a[20], b[20];
  snprintf(a, sizeof(a), "%.0f km/h", speedKmh);
  snprintf(b, sizeof(b), "%.0f m", alt);
  g->setCursor(2, 213);
  g->print(a);
  g->setCursor(170, 213);
  g->print(b);
  // Welches APRS-Symbol/Fahrzeugkennung der eigene Beacon gerade sendet, war bisher nirgends auf einen
  // Blick zu erkennen (nur verschluesselt als einzelnes Zeichen im rohen Funkpaket, siehe aprsSendBeacon())
  // -- gerade in dieser Grossanzeige (ab OWN_SPEED_BIG_KMH, also fast immer bei Rad/Auto) steht es jetzt klein
  // unten mit dabei.
  if (radioMode == 1 && loraTxOn) {
    g->setTextSize(1);
    g->setTextColor(0x8410);
    g->setCursor(2, 231);
    g->print(aprsSymbolNames[aprsSymbolIdx % NSYMBOL]);
  }
}

// Grosse Statuszeile mit Sonde (schneller als OWN_SPEED_BIG_KMH): Hoehe, Steig-/Sinkrate (mit Pfeil), Geschwindigkeit
// und Entfernung -- jeweils zur letzten bekannten Sondenposition, auch wenn der Empfang gerade aussetzt (genau wie
// die kleine Detailanzeige zeigt dann einfach die letzten bekannten Werte weiter, statt auf "--" zu wechseln).
static void drawBigSondeStatus(Adafruit_GFX *g, double alt, bool haveV, float vz, float vKmh, double distKm) {
  g->fillRect(0, 205, 320, 35, ST77XX_BLACK);
  g->setTextWrap(false);
  g->setTextSize(2);
  char a[20], b[20], c[20], d[20];
  g->setTextColor(ST77XX_WHITE);
  snprintf(a, sizeof(a), "%.0f m", alt);
  g->setCursor(2, 206);
  g->print(a);
  if (haveV) {
    drawVzArrow(g, 140, 214, vz);
    snprintf(b, sizeof(b), "%+.1f m/s", vz);
    g->setTextColor(vz > 1.0f ? ST77XX_GREEN : (vz < -1.0f ? COL_TRAIL : ST77XX_WHITE));
    g->setCursor(158, 206);
    g->print(b);
  }
  g->setTextColor(ST77XX_WHITE);
  if (haveV) snprintf(c, sizeof(c), "%.0f km/h", vKmh);
  else snprintf(c, sizeof(c), "- km/h");
  g->setCursor(2, 222);
  g->print(c);
  if (distKm < 1.0) snprintf(d, sizeof(d), "%.0f m", distKm * 1000.0);
  else snprintf(d, sizeof(d), "%.1f km", distKm);
  g->setCursor(160, 222);
  g->print(d);
}

// Karte ohne Sondenempfang: Umgebung der eigenen Position (aktueller GPS-Fix, sonst letzte bekannte Position).
// Im LoRa-APRS-Modus richtet sich der Kartenausschnitt zusaetzlich nach aprsFocus (Taste R, siehe
// focusCycleOrRecenter()): 0 nur Ich (wie bisher immer), 1 Ich + alle gehoerten Stationen, 2 Ich + nur die
// DIREKT per Funk gehoerten Stationen (ohne Fremdverkehr/"}"-Relais, siehe aprsRecvKind[]/aprsStoreFromInfo()).
static void idleMapUpdate() {
  uint32_t t0 = millis();
  double hLat, hLon;
  bool gpsFix = homePosition(hLat, hLon);
  // Einzelstations-Fokus (aprsSoloIdx, siehe dort) nicht mehr gueltig (Station inzwischen aus dem Puffer
  // gefallen/Position unplausibel) -- leise wieder verlassen, statt auf eine falsche/alte Position zu zoomen.
  if (aprsSoloIdx >= 0 && !(aprsSoloIdx < aprsN && isfinite(aprsLat[aprsSoloIdx]) && isfinite(aprsLon[aprsSoloIdx]))) aprsSoloIdx = -1;
  bool solo = (radioMode == 1 && aprsSoloIdx >= 0);
  bool multi = !solo && (radioMode == 1 && aprsFocus != 0 && aprsN > 0);
  bool viewChanged;
  if (solo) {
    // NUR die Station in der Bildmitte, "Ich" bewusst aussen vor -- wo man selbst ist, weiss man ja (siehe
    // Rueckmeldung). "Ich" taucht trotzdem mit auf, falls gerade im Bildausschnitt bzw. als Randpfeil
    // (drawEdgeArrow() unten, laeuft unveraendert immer mit).
    double sLat = aprsLat[aprsSoloIdx], sLon = aprsLon[aprsSoloIdx];
    viewChanged = mapViewSetup(&sLat, &sLon, 1, true, true);
    soloZoom = viewZ;
  } else if (multi) {
    double plat[1 + APRS_MAX], plon[1 + APRS_MAX];
    int np = 0;
    plat[np] = hLat; plon[np] = hLon; np++;
    for (int i = 0; i < aprsN; i++) {
      if (aprsFocus == 2 && aprsRecvKind[i] == RECV_IGATE) continue;    // Fokus 2: Fremdverkehr-Stationen aussen vor lassen
      if (!isfinite(aprsLat[i]) || !isfinite(aprsLon[i])) continue;     // zweite Sicherung, siehe aprsReadMin5()
      plat[np] = aprsLat[i]; plon[np] = aprsLon[i]; np++;
    }
    viewChanged = mapViewSetup(plat, plon, np, true, false, true);   // centerOnFirst: Ich (Index 0) bleibt im Zentrum
    if (np == 1) soloZoom = viewZ;                // keine (passende) Station gefunden: wie Einzelpunkt behandeln
  } else {
    // "Ich" bleibt bei einem Einzelpunkt (hier: kein Sondenempfang, oder LoRa-Fokus 0) jetzt IMMER exakt auf der
    // Bildmitte festgenagelt (tight=true), statt erst zum Rand laufen zu lassen und dann zu verschieben -- das
    // kostet dank bigCanvas/prepareMapBackground() (Rand ringsum schon vorgerendert) i. d. R. keinen vollen
    // Neuaufbau mehr, sondern nur noch einen schnellen Speicher-Crop.
    viewChanged = mapViewSetup(&hLat, &hLon, 1, true, true);
    soloZoom = viewZ;                             // gilt als "nur Ich"-Zoom, damit R spaeter dorthin zurueckkann
  }
  prepareMapBackground(viewChanged);
  mapIdleShown = true;
  double hx, hy;
  latLonToPixel(hLat, hLon, viewZ, hx, hy);
  int hX = (int)floor(hx - (viewCx - 160.0)), hY = (int)floor(hy - (viewCy - 120.0));
  // Die fruehere "IchFollow"-Diagnosezeile (Position/Pixelkoordinaten alle 5 s, fuer "Ich folgt nicht") ist
  // entfernt -- sie lief unabhaengig vom tatsaechlichen Bewegungs-/Fehlerfall ständig mit und war fuer die
  // GPS-Fehlersuche (Ausreisser, Tunnel/Bruecken) selbst wenig hilfreich. An ihrer Stelle loggen jetzt
  // gpsFilterFix() direkt bei einem Ausreisser/einer Bestaetigung sowie gpsLogTick() mit HDOP alle 20 s.
  drawOwnTrail(hX, hY);
  if (radioMode == 1) drawAprsStations();
  frameCanvas->fillCircle(hX, hY, 6, COL_ICH_RING);
  frameCanvas->fillCircle(hX, hY, 5, COL_ICH_FILL);
  drawEdgeArrow(frameCanvas, hX, hY, COL_ICH_FILL);
  if (effSpeedKmh() > OWN_SPEED_BIG_KMH) {
    drawBigOwnStatus(frameCanvas, effSpeedKmh(), gps.alt);
  } else {
    static const char *const styleShort[NSTYLE] = {"Rad", "Luft", "Topo", "Rel", "TopPl"};
    char l1[64], l2[64], l3[64];
    if (radioMode == 1 && aprsSoloIdx >= 0) {
      snprintf(l1, sizeof(l1), "LoRa-APRS: Fokus %s   z%.1f %s%s", aprsCall[aprsSoloIdx], viewZ, styleShort[mapStyle % NSTYLE], mapManual ? " man" : "");
    }
    else if (radioMode == 1) {
      static const char *const afname[3] = {"", " F:alle", " F:direkt"};
      snprintf(l1, sizeof(l1), "LoRa-APRS: %d Station(en)   z%.1f %s%s%s", aprsN, viewZ, styleShort[mapStyle % NSTYLE], mapManual ? " man" : "", afname[aprsFocus]);
    }
    else snprintf(l1, sizeof(l1), "Keine Sonde empfangen   z%.1f %s%s", viewZ, styleShort[mapStyle % NSTYLE], mapManual ? " man" : "");
    snprintf(l2, sizeof(l2), "Ich: %.5f %.5f  %s", hLat, hLon, gpsFix ? "GPS" : "letzte Position");
    if (radioMode == 1 && loraTxOn) snprintf(l3, sizeof(l3), "LoRa-APRS %.3f MHz | Eigenes Symbol: %s", APRS_FREQ_MHZ, aprsSymbolNames[aprsSymbolIdx % NSYMBOL]);
    else if (radioMode == 1) snprintf(l3, sizeof(l3), "Empfang: LoRa-APRS %.3f MHz", APRS_FREQ_MHZ);
    else if (fixedFreq) snprintf(l3, sizeof(l3), "Empfang: feste Frequenz %.3f MHz", freqMHz);
    else snprintf(l3, sizeof(l3), "Empfang: Suchlauf 400-406 MHz");
    frameCanvas->fillRect(0, 205, 320, 35, ST77XX_BLACK);
    frameCanvas->setTextSize(1);
    frameCanvas->setTextColor(ST77XX_WHITE);
    frameCanvas->setTextWrap(false);    // sonst rutscht eine zu lange Zeile 1 automatisch in Zeile 2 hinein (ueberlagert sie)
    frameCanvas->setCursor(2, 207);
    frameCanvas->print(l1);
    frameCanvas->setCursor(2, 217);
    frameCanvas->print(l2);
    frameCanvas->setCursor(2, 227);
    frameCanvas->print(l3);
  }
  drawToast(frameCanvas);
  drawStatus(frameCanvas, 264, 1);
  blit(frameCanvas);
  lastMapMs = millis() - t0;
}

// Zeichnet die Kartenansicht mit Sonde und "Ich"
static void mapUpdate() {
  if (!bgCanvas || !frameCanvas) return;
  if (!haveSondePos) { idleMapUpdate(); return; }
  if (mapIdleShown) { mapIdleShown = false; viewZ = -1; }        // vom Ruhebild zur Sondenansicht: Hintergrund neu zentrieren
  uint32_t t0 = millis();
  double hLat, hLon;
  bool gpsFix = homePosition(hLat, hLon);

  // Automatik: die ganze Zoomstufe, bei der ich, die Sonde und (falls vorhanden) der/die Landepunkt(e) ins Bild passen.
  // R zykelt rFocus: 0 = alle Punkte, 1 = Sonde+Ich ohne Landung, 2 = nur Sonde, 3 = nur Ich (manuell ohne Wirkung)
  double plat[4], plon[4];
  int fnp = 0;
  plat[fnp] = hLat; plon[fnp] = hLon; fnp++;
  plat[fnp] = sondeLat; plon[fnp] = sondeLon; fnp++;
  if (predValid) { plat[fnp] = predLat; plon[fnp] = predLon; fnp++; }
  if (onlinePredValid) { plat[fnp] = onlinePredLat; plon[fnp] = onlinePredLon; fnp++; }
  double soloLat[1], soloLon[1];
  const double *fplat = plat, *fplon = plon;
  if (rFocus == 1) { fnp = 2; }                     // Sonde+Ich, Landepunkt(e) bewusst aussen vor
  else if (rFocus == 2) { soloLat[0] = sondeLat; soloLon[0] = sondeLon; fplat = soloLat; fplon = soloLon; fnp = 1; }
  else if (rFocus == 3) { soloLat[0] = hLat; soloLon[0] = hLon; fplat = soloLat; fplon = soloLon; fnp = 1; }
  // Einzelpunkt (nur Sonde/nur Ich, fnp==1) wird jetzt IMMER exakt auf die Bildmitte festgenagelt (tight=true),
  // statt erst zum Rand laufen zu lassen und dann zu verschieben -- dank bigCanvas/prepareMapBackground() (Rand
  // ringsum schon vorgerendert) kostet das i. d. R. keinen vollen Neuaufbau mehr, sondern nur einen schnellen
  // Speicher-Crop. Bei mehreren Punkten (fnp>1) bleibt die bisherige Rand-Toleranz, ausser knapp vor der
  // Landung (< 200 m) oder nach einem einmaligen R-Wechsel auf einen Einzelpunkt (forceRecenter).
  bool tightFollow = (fnp == 1) || haversineKm(hLat, hLon, sondeLat, sondeLon) < 0.2 || forceRecenter;
  forceRecenter = false;
  bool viewChanged = mapViewSetup(fplat, fplon, fnp, false, tightFollow);
  prepareMapBackground(viewChanged);
  if (fnp == 1) soloZoom = viewZ;                   // Zoom fuer "nur Sonde"/"nur Ich" merken, damit R spaeter dorthin zurueckkann
  double sx, sy, hx, hy, qx = 0, qy = 0, rx = 0, ry = 0;
  latLonToPixel(sondeLat, sondeLon, viewZ, sx, sy);
  latLonToPixel(hLat, hLon, viewZ, hx, hy);
  if (predValid) latLonToPixel(predLat, predLon, viewZ, qx, qy);
  if (onlinePredValid) latLonToPixel(onlinePredLat, onlinePredLon, viewZ, rx, ry);
  double ox = viewCx - 160.0, oy = viewCy - 120.0;
  int sX = (int)floor(sx - ox), sY = (int)floor(sy - oy), hX = (int)floor(hx - ox), hY = (int)floor(hy - oy);
  drawOwnTrail(hX, hY);                                     // eigener Track unter der Sondenspur
  // Spur der Sonde (orange, mit schwarzem Lichthof, siehe drawHaloLine()) bis zum aktuellen Punkt
  if (trailN > 0) {
    int px0 = 0, py0 = 0;
    for (int i = 0; i < trailN; i++) {
      double tx, ty;
      latLonToPixel(trailLat[i], trailLon[i], viewZ, tx, ty);
      int x = (int)floor(tx - ox), y = (int)floor(ty - oy);
      if (i > 0 && !((px0 < 0 && x < 0) || (px0 > 319 && x > 319) || (py0 < 0 && y < 0) || (py0 > 239 && y > 239)))
        drawHaloLine(frameCanvas, px0, py0, x, y, COL_TRAIL);
      px0 = x; py0 = y;
    }
    drawHaloLine(frameCanvas, px0, py0, sX, sY, COL_TRAIL);
  }
  // Vorhersage: blaue Verlaengerung der Spur bis zum Landepunkt
  int qX = 0, qY = 0;
  if (predValid) {
    qX = (int)floor(qx - ox);
    qY = (int)floor(qy - oy);
    drawHaloLine(frameCanvas, sX, sY, qX, qY, COL_PRED);
  }
  // Online-Vorhersage (Tawhiri/SondeHub-API, echte Windvorhersage): vermutliche Flugstrecke des Abstiegs statt
  // nur einer Geraden (sofern geparst, siehe onlinePredPathN/parseOnlinePredPath()), mit schwarzem Lichthof,
  // damit die Linie auf jeder Kartenfarbe gut zu sehen ist (vorher schlecht sichtbares Gelb auf hellem Grund).
  int rX = 0, rY = 0;
  if (onlinePredValid) {
    rX = (int)floor(rx - ox);
    rY = (int)floor(ry - oy);
    if (onlinePredPathN >= 2) {
      int px = sX, py = sY;
      for (int i = 0; i < onlinePredPathN; i++) {
        double tx, ty;
        latLonToPixel(onlinePredPathLat[i], onlinePredPathLon[i], viewZ, tx, ty);
        int x = (int)floor(tx - ox), y = (int)floor(ty - oy);
        drawHaloLine(frameCanvas, px, py, x, y, COL_PRED_ONLINE);
        px = x; py = y;
      }
    } else {
      drawHaloLine(frameCanvas, sX, sY, rX, rY, COL_PRED_ONLINE);
    }
  }
  // Sonden-Punkt: Fuellfarbe nach Alter der letzten GUELTIGEN Position (gruen/gelb/rot) -- ein blosses Signal
  // (Schritt 1 ohne Schritt 2/Position) faerbt NICHT mehr gruen, siehe lastPosOkMs. Blinkt aber kurz mit einem
  // gruenen Ring, wenn gerade ein Frame reinkam, damit man sieht "da ist was", auch wenn es (noch) nicht reichte.
  uint32_t ageS = lastRxMs ? (millis() - lastRxMs) : 999999UL;
  uint32_t age = lastPosOkMs ? (millis() - lastPosOkMs) / 1000 : 999;
  uint16_t sCol = (age < 4) ? COL_SONDE_OK : (age < 20) ? COL_SONDE_LATE : COL_SONDE_LOST;
  uint16_t sRing = (ageS < SONDE_SIGNAL_FLASH_MS && sCol != COL_SONDE_OK) ? COL_SONDE_RING_SIG : COL_SONDE_RING;
  drawHaloLine(frameCanvas, hX, hY, sX, sY, COL_LINE);      // Verbindungslinie ich -> Sonde
  frameCanvas->fillCircle(hX, hY, 6, COL_ICH_RING);         // ich: Ring
  frameCanvas->fillCircle(hX, hY, 5, COL_ICH_FILL);         // ich: Fuellung
  frameCanvas->fillCircle(sX, sY, 6, sRing);                // Sonde: Ring (kurz gruen bei blossem Signal)
  frameCanvas->fillCircle(sX, sY, 5, sCol);                 // Sonde: Farbe nach Alter der letzten gueltigen Position
  if (predValid) drawCrosshairHalo(frameCanvas, qX, qY, COL_PRED);   // vorhergesagter Landepunkt (lokal)
  if (onlinePredValid) drawCrosshairHalo(frameCanvas, rX, rY, COL_PRED_ONLINE);   // vorhergesagter Landepunkt (online)
  drawEdgeArrow(frameCanvas, hX, hY, COL_ICH_FILL);         // liegt ein Punkt ausserhalb des Bildes: Pfeil am Rand
  drawEdgeArrow(frameCanvas, sX, sY, sCol);

  double dist = haversineKm(hLat, hLon, sondeLat, sondeLon);
  double brg = bearingDeg(hLat, hLon, sondeLat, sondeLon);
  char d[16];
  if (dist < 1.0) snprintf(d, sizeof(d), "%.0f m", dist * 1000.0);
  else snprintf(d, sizeof(d), "%.1f km", dist);
  if (effSpeedKmh() > OWN_SPEED_BIG_KMH) {
    drawBigSondeStatus(frameCanvas, sondeAlt, haveVel, sondeVz, sondeVh * 3.6f, dist);
  } else {
    char l1[64], l2[64];
    static const char *const styleShort[NSTYLE] = {"Rad", "Luft", "Topo", "Rel", "TopPl"};
    const char *mm = mapManual ? " man" : (rFocus == 1 ? " S+I" : (rFocus == 2 ? " Sonde" : (rFocus == 3 ? " Ich" : "")));
    if (age < 4) snprintf(l1, sizeof(l1), "%s Nr %d  %.0f dBm  z%.1f %s%s", sondeId, sondeFnr, sondeRssi, viewZ, styleShort[mapStyle % NSTYLE], mm);
    else snprintf(l1, sizeof(l1), "%s Nr %d  KEIN EMPFANG seit %lu s  z%.1f%s", sondeId, sondeFnr, (unsigned long)age, viewZ, mm);
    snprintf(l2, sizeof(l2), "%s  %03.0f Grad  Hoehe %.0f m  %s%s", d, brg, sondeAlt, gpsFix ? "GPS" : "fest",
             onlinePredValid ? "  +Online" : "");   // gelbes Fadenkreuz: zusaetzliche Landevorhersage per Internet
    char l3[64];
    const char *ph = sondeVz > 1.0f ? "steigt" : (sondeVz < -1.0f ? "sinkt" : "eben");
    if (haveVel && predValid) snprintf(l3, sizeof(l3), "%.0f km/h %03.0f Grad %+.1f m/s Landung %lu:%02lu", sondeVh * 3.6f, sondeDir,
                                       sondeVz, (unsigned long)(predEtaS / 60), (unsigned long)(predEtaS % 60));
    else if (haveVel) snprintf(l3, sizeof(l3), "%.0f km/h %03.0f Grad  %+.1f m/s %s", sondeVh * 3.6f, sondeDir, sondeVz, ph);
    else snprintf(l3, sizeof(l3), "Geschwindigkeit: -");
    frameCanvas->fillRect(0, 205, 320, 35, ST77XX_BLACK);
    frameCanvas->setTextSize(1);
    frameCanvas->setTextColor(ST77XX_WHITE);
    frameCanvas->setTextWrap(false);    // sonst rutscht eine zu lange Zeile 1 automatisch in Zeile 2 hinein (ueberlagert sie)
    frameCanvas->setCursor(2, 207);
    frameCanvas->print(l1);
    frameCanvas->setCursor(2, 217);
    frameCanvas->print(l2);
    frameCanvas->setCursor(2, 227);
    frameCanvas->print(l3);
  }
  drawToast(frameCanvas);
  drawStatus(frameCanvas, 264, 1);
  blit(frameCanvas);
  lastMapMs = millis() - t0;
  VLOG("Karte aktualisiert in %lu ms (%s, %03.0f Grad)\n", (unsigned long)lastMapMs, d, brg);
}

// Hoehentrack/Stationen-Ansicht (gfxView == 1): im Sondenmodus ein Hoehendiagramm (Hoehe ueber der Zeit
// seit dem Fund der aktuellen Sonde, siehe altTrackAdd()), im LoRa-Modus stattdessen die Liste der
// gehoerten APRS-Stationen mit Entfernung und "zuletzt gehoert". Dieselbe Weboberflaeche-Logik (gleiches
// Aktualisierungsintervall wie das Live-Bild) spiegelt das auf der Menueseite, siehe webAltPanel().
static void altTrackUpdate() {
  if (!frameCanvas) return;
  frameCanvas->fillScreen(ST77XX_BLACK);
  frameCanvas->setTextSize(1);
  if (radioMode == 1) {                              // LoRa-APRS: Stationsliste statt Hoehentrack
    frameCanvas->setTextColor(ST77XX_CYAN);
    frameCanvas->setCursor(4, 4);
    frameCanvas->print("LoRa-APRS-Stationen");
    frameCanvas->setTextColor(COL_RADAR_TEXT);              // Rohempfangs-Diagnose: zeigt auch an, wenn ueberhaupt
    frameCanvas->setCursor(4, 14);                           // Pakete ankommen, aber keine Station daraus wird
    if (aprsRxTotal == 0) {
      frameCanvas->print("Pakete: 0 (noch kein Empfang)");
    } else {
      char rx[64];
      uint32_t ageS = (millis() - aprsLastRxMs) / 1000;
      snprintf(rx, sizeof(rx), "Pakete: %lu (%lu ok) letzt. vor %lus %.0fdBm", (unsigned long)aprsRxTotal,
               (unsigned long)aprsRxOk, (unsigned long)ageS, aprsLastRssi);
      frameCanvas->print(rx);
    }
    if (aprsN == 0) {
      frameCanvas->setTextColor(COL_RADAR_TEXT);
      frameCanvas->setCursor(4, 28);
      frameCanvas->print("Noch keine Station gehoert");
    } else {
      double hLat, hLon;
      homePosition(hLat, hLon);
      // Anzeigereihenfolge nach zuletzt gehoert sortieren (neueste zuerst), statt nach Array-Index --
      // sonst wirkt die Liste bei vollem Puffer so, als wuerde sich nichts bewegen/nach unten verschieben
      // (siehe auch den Eviction-Fix in aprsFindOrAdd()). Einfache Einfuegesortierung, bei max. APRS_MAX
      // (32) Eintraegen voellig ausreichend schnell.
      int idx[APRS_MAX];
      for (int i = 0; i < aprsN; i++) idx[i] = i;
      for (int i = 1; i < aprsN; i++) {
        int v = idx[i], j = i - 1;
        while (j >= 0 && aprsSeenMs[idx[j]] < aprsSeenMs[v]) { idx[j + 1] = idx[j]; j--; }
        idx[j + 1] = v;
      }
      // Scroll-Fenster so nachziehen, dass die markierte Station moeglichst mittig sichtbar bleibt (siehe
      // aprsListScroll oben) -- vorher wurde immer nur ab der neuesten Station gezeichnet, eine durch
      // neuere Stationen nach unten verdraengte Auswahl war dann schlicht unsichtbar.
      const int listTop = 26, rowH = 11, listBottom = 230;
      const int rowsVisible = (listBottom - listTop) / rowH + 1;
      if (aprsN <= rowsVisible) {
        aprsListScroll = 0;
      } else {
        int selPos = -1;
        for (int k = 0; k < aprsN; k++) if (idx[k] == aprsListSel) { selPos = k; break; }
        if (selPos >= 0) aprsListScroll = constrain(selPos - rowsVisible / 2, 0, aprsN - rowsVisible);
        else aprsListScroll = constrain(aprsListScroll, 0, aprsN - rowsVisible);
      }
      if (aprsN > rowsVisible) {                          // Positionsanzeige, nur wenn ueberhaupt gescrollt wird
        char posb[16];
        snprintf(posb, sizeof(posb), "%d-%d/%d", aprsListScroll + 1, min(aprsListScroll + rowsVisible, aprsN), aprsN);
        frameCanvas->setTextColor(COL_RADAR_TEXT);
        frameCanvas->setCursor(260, 4);
        frameCanvas->print(posb);
      }
      int y = listTop;
      for (int k = aprsListScroll; k < aprsN && y < listBottom; k++) {
        int i = idx[k];
        uint32_t ageS = (millis() - aprsSeenMs[i]) / 1000;
        char ageb[16];
        if (ageS < 3600) snprintf(ageb, sizeof(ageb), "%lu:%02lu", (unsigned long)(ageS / 60), (unsigned long)(ageS % 60));
        else snprintf(ageb, sizeof(ageb), "%luh", (unsigned long)(ageS / 3600));
        double d = haversineKm(hLat, hLon, aprsLat[i], aprsLon[i]);
        char ln[64];
        snprintf(ln, sizeof(ln), "%-9s %6.1f km  vor %s", aprsCall[i], d, ageb);
        bool sel = (i == aprsListSel);                // per Trackball markierte Station (siehe aprsListMoveSel())
        if (sel) frameCanvas->fillRect(0, y - 2, 320, 11, COL_RADAR_RING);
        // Herkunft des Pakets vorangestellt, wie auf der Karte farblich unterschieden (aprsRecvKind[], siehe
        // COL_APRS_*): D = direkt per Funk gehoert (blau), R = ueber einen Digipeater-Hop weitergereicht
        // (orange), I = Fremdverkehr ueber ein iGate/APRS-IS relayt (rot). Eigene Farbe bleibt auch in der
        // markierten (hervorgehobenen) Zeile erhalten, nur der Rest der Zeile wechselt dort auf Schwarz.
        const char *kindTag = (aprsRecvKind[i] == RECV_IGATE) ? "I" : (aprsRecvKind[i] == RECV_DIGI) ? "R" : "D";
        uint16_t kindCol = (aprsRecvKind[i] == RECV_IGATE) ? COL_APRS_IGATE
                         : (aprsRecvKind[i] == RECV_DIGI) ? COL_APRS_DIGI : COL_APRS_DIRECT;
        frameCanvas->setTextColor(kindCol);
        frameCanvas->setCursor(4, y);
        frameCanvas->print(kindTag);
        frameCanvas->setTextColor(sel ? ST77XX_BLACK : (ageS < 300 ? ST77XX_WHITE : COL_RADAR_TEXT));
        frameCanvas->setCursor(12, y);
        frameCanvas->print(ln);
        if (sel && aprsComment[i][0]) {                 // Hinweis, dass ein Trackballklick mehr zeigt
          frameCanvas->setCursor(220, y);
          frameCanvas->print("(Klick: Info)");
        }
        y += 11;
      }
    }
    // Zusatzinfo-Popup (Trackballklick auf die markierte Station, siehe handleInput()) -- deckt einen Teil
    // der Liste ab, ein weiterer Klick blendet es wieder aus. Nur zeichnen, wenn die Auswahl noch gueltig
    // ist (Station nicht inzwischen aus der Liste gefallen, siehe aprsRemove()) und einen Kommentar hat.
    if (aprsPopupShown && aprsListSel >= 0 && aprsListSel < aprsN && aprsComment[aprsListSel][0]) {
      const int bx = 16, by = 40, bw = 288, bh = 170;
      frameCanvas->fillRect(bx, by, bw, bh, ST77XX_BLACK);
      frameCanvas->drawRect(bx, by, bw, bh, ST77XX_CYAN);
      frameCanvas->drawRect(bx + 1, by + 1, bw - 2, bh - 2, ST77XX_CYAN);
      frameCanvas->setTextSize(2);
      frameCanvas->setTextColor(ST77XX_CYAN);
      frameCanvas->setCursor(bx + 8, by + 8);
      frameCanvas->print(aprsCall[aprsListSel]);
      frameCanvas->drawFastHLine(bx + 6, by + 28, bw - 12, COL_RADAR_RING);
      frameCanvas->setTextSize(1);
      frameCanvas->setTextColor(ST77XX_WHITE);
      char lines[12][48];
      int nLines = wrapTextLines(aprsComment[aprsListSel], 44, lines, 12);
      int ly = by + 36;
      for (int li = 0; li < nLines && ly < by + bh - 10; li++) {
        frameCanvas->setCursor(bx + 8, ly);
        frameCanvas->print(lines[li]);
        ly += 11;
      }
      frameCanvas->setTextColor(COL_RADAR_TEXT);
      frameCanvas->setCursor(bx + 8, by + bh - 10);
      frameCanvas->print("Klick: schliessen");
    }
  } else {                                           // Sondenmodus: Hoehe ueber der Zeit
    frameCanvas->setTextColor(ST77XX_CYAN);
    frameCanvas->setCursor(4, 4);
    frameCanvas->print("Hoehentrack");
    if (!altTrackAlt) {
      frameCanvas->setTextColor(COL_RADAR_TEXT);
      frameCanvas->setCursor(4, 24);
      frameCanvas->print("Nicht verfuegbar (kein PSRAM)");
    } else if (altTrackN < 2) {
      frameCanvas->setTextColor(COL_RADAR_TEXT);
      frameCanvas->setCursor(4, 24);
      frameCanvas->print(haveSondePos ? "Sammle Daten..." : "Keine Sonde empfangen");
    } else {
      const int gx0 = 44, gx1 = 312, gy0 = 24, gy1 = 206;   // Diagrammflaeche
      float aMin = altTrackAlt[0], aMax = altTrackAlt[0];
      for (int i = 1; i < altTrackN; i++) {
        if (altTrackAlt[i] < aMin) aMin = altTrackAlt[i];
        if (altTrackAlt[i] > aMax) aMax = altTrackAlt[i];
      }
      if (aMax - aMin < 10.0f) aMax = aMin + 10.0f;   // Mindestspannweite nur oben -- unten bleibt bewusst
                                                        // exakt die tiefste empfangene Hoehe (s. Flaechenfuellung)
      uint32_t tMin = altTrackMs[0], tMax = altTrackMs[altTrackN - 1];
      if (tMax <= tMin) tMax = tMin + 1;
      frameCanvas->drawRect(gx0, gy0, gx1 - gx0, gy1 - gy0, COL_RADAR_RING);
      char b[16];
      frameCanvas->setTextColor(ST77XX_WHITE);
      snprintf(b, sizeof(b), "%.0f", aMax);
      frameCanvas->setCursor(2, gy0);
      frameCanvas->print(b);
      snprintf(b, sizeof(b), "%.0f", aMin);
      frameCanvas->setCursor(2, gy1 - 7);
      frameCanvas->print(b);
      // Pro Pixelspalte interpolieren: altTrackMs ist monoton steigend, ein Zeiger "j" wandert dabei nur
      // vorwaerts (O(N) insgesamt). Das skaliert IMMER die gesamte gespeicherte Kurve (erstes bis letztes
      // Paket) neu auf die Chartbreite -- ohne neuen Empfang bleibt tMax und damit das Bild unveraendert,
      // der letzte Wert steht also einfach am rechten Rand. Eine Funkpause wird dabei ganz natuerlich mit
      // einer geraden Linie zwischen den Punkten davor/danach verbunden (lineare Interpolation). Zugleich
      // wird je Spalte die Flaeche bis zur tiefsten Hoehe (aMin, Zeile gy1) gefuellt.
      int j = 0, prevY = gy1;
      for (int cx = gx0; cx <= gx1; cx++) {
        uint32_t t = tMin + (uint32_t)((double)(tMax - tMin) * (cx - gx0) / (gx1 - gx0));
        while (j < altTrackN - 2 && altTrackMs[j + 1] < t) j++;
        uint32_t t0 = altTrackMs[j], t1 = altTrackMs[j + 1];
        float frac = (t1 > t0) ? (float)(t - t0) / (float)(t1 - t0) : 0.0f;
        if (frac < 0.0f) frac = 0.0f; else if (frac > 1.0f) frac = 1.0f;
        float a = altTrackAlt[j] + (altTrackAlt[j + 1] - altTrackAlt[j]) * frac;
        int y = gy1 - (int)((float)(gy1 - gy0) * (a - aMin) / (aMax - aMin));
        if (y < gy0) y = gy0; else if (y > gy1) y = gy1;
        frameCanvas->drawLine(cx, y, cx, gy1, COL_ALT_FILL);          // Flaeche bis zur tiefsten Hoehe (Farbe wie auf der Website)
        if (cx > gx0) {
          frameCanvas->drawLine(cx - 1, prevY, cx, y, ST77XX_YELLOW);
          frameCanvas->drawLine(cx - 1, prevY + 1, cx, y + 1, ST77XX_YELLOW);
        }
        prevY = y;
      }
      frameCanvas->fillCircle(gx1, prevY, 3, ST77XX_RED);      // letzter empfangener Wert, am rechten Rand
      frameCanvas->setTextSize(2);
      frameCanvas->setTextColor(ST77XX_WHITE);
      snprintf(b, sizeof(b), "%.0f m", sondeAlt);
      frameCanvas->setCursor(gx0 + 4, gy1 + 6);
      frameCanvas->print(b);
      frameCanvas->setTextSize(1);
      frameCanvas->setTextColor(COL_RADAR_TEXT);
      uint32_t tSpanS = (tMax - tMin) / 1000;
      snprintf(b, sizeof(b), "%lu:%02lu min", (unsigned long)(tSpanS / 60), (unsigned long)(tSpanS % 60));
      frameCanvas->setCursor(220, gy1 + 10);
      frameCanvas->print(b);
    }
  }
  drawToast(frameCanvas);
  drawStatus(frameCanvas, 264, 1);
  blit(frameCanvas);
}

// ---------- GPS-Radar-Diagnoseseite (gfxView == 2) ----------
// Polarplot der einzelnen Satelliten (Azimut/Elevation, aus $--GSV -- siehe satTable[]/satUpdate() oben):
// Mitte = Zenit (90 Grad Elevation), Ringe alle 30 Grad bis zum Rand (0 Grad, Horizont), Norden oben, im
// Uhrzeigersinn (wie ein Kompass). Farbe nach Konstellation (Talker-ID); ein gefuellter Punkt wurde mit
// brauchbarem SNR tatsaechlich empfangen, ein nur umrandeter Punkt ist laut Almanach sichtbar, aber (noch) zu
// schwach fuer eine Dekodierung (Mehrwegeempfang/Abschattung). Rechts daneben eine Textliste aller frischen
// Eintraege (PRN, Elevation, Azimut, SNR), weil sich nicht jeder Punkt im Plot selbst sauber beschriften laesst.
static uint16_t satTalkerColor(const char *talker) {
  if (strncmp(talker, "GP", 2) == 0) return ST77XX_GREEN;                                  // GPS
  if (strncmp(talker, "GL", 2) == 0) return ST77XX_ORANGE;                                 // GLONASS
  if (strncmp(talker, "GA", 2) == 0) return ST77XX_CYAN;                                   // Galileo
  if (strncmp(talker, "GB", 2) == 0 || strncmp(talker, "BD", 2) == 0) return ST77XX_MAGENTA;  // BeiDou
  if (strncmp(talker, "GQ", 2) == 0) return ST77XX_YELLOW;                                 // QZSS
  if (strncmp(talker, "GI", 2) == 0) return ST77XX_RED;                                    // NavIC
  return ST77XX_WHITE;
}

static void satRadarUpdate() {
  if (!frameCanvas) return;
  // gfxUpdate() wird (wie bei jeder Ansicht) aus loop() heraus voellig ungedrosselt aufgerufen -- bei Karte/
  // Hoehentrack faellt das nicht auf, weil sich deren Inhalt zwischen zwei Aufrufen kaum aendert. Hier aber
  // trudeln laufend neue $--GSV-Saetze ein (satUpdate() oben), und jeder Aufruf hat nur einen Teil des
  // aktuellen Zyklus verarbeitet -- ein Vollbildneuaufbau bei JEDEM loop()-Durchlauf zeigte daher staendig
  // einen anderen Zwischenstand (mal diese, mal jene Konstellation gerade aktualisiert) und wirkte wie zwei
  // sich gegenseitig ueberlagernde, flackernde Ansichten. Deshalb hoechstens alle 500 ms wirklich neu zeichnen.
  static uint32_t lastDraw = 0;
  uint32_t nowMs = millis();
  if (lastDraw != 0 && nowMs - lastDraw < 500) return;
  lastDraw = nowMs;
  frameCanvas->fillScreen(ST77XX_BLACK);
  frameCanvas->setTextSize(1);
  frameCanvas->setTextColor(ST77XX_CYAN);
  frameCanvas->setCursor(4, 4);
  frameCanvas->print("GPS-Radar");
  frameCanvas->setTextColor(COL_RADAR_TEXT);
  char hdr[48];
  if (gps.hdop > 0) snprintf(hdr, sizeof(hdr), "Fix %s %dSat HDOP%.1f sicht%d", gps.fix ? "ja" : "nein", gps.sats, gps.hdop, satFreshCount());
  else snprintf(hdr, sizeof(hdr), "Fix %s %dSat HDOP-- sicht%d", gps.fix ? "ja" : "nein", gps.sats, satFreshCount());
  frameCanvas->setCursor(4, 14);
  frameCanvas->print(hdr);
  frameCanvas->setCursor(198, 14);
  frameCanvas->print("PRN Elv Az SNR");

  const int cx = 95, cy = 138, maxR = 86;                 // Polarplot: Mitte = Zenit, Rand = Horizont
  frameCanvas->drawCircle(cx, cy, maxR, COL_RADAR_RING);          // 0 Grad Elevation (Horizont)
  frameCanvas->drawCircle(cx, cy, maxR * 2 / 3, COL_RADAR_RING);  // 30 Grad
  frameCanvas->drawCircle(cx, cy, maxR / 3, COL_RADAR_RING);      // 60 Grad
  frameCanvas->drawFastHLine(cx - maxR, cy, 2 * maxR, COL_RADAR_RING);
  frameCanvas->drawFastVLine(cx, cy - maxR, 2 * maxR, COL_RADAR_RING);
  frameCanvas->setCursor(cx - 3, cy - maxR - 10); frameCanvas->print("N");
  frameCanvas->setCursor(cx - 3, cy + maxR + 2);  frameCanvas->print("S");
  frameCanvas->setCursor(cx + maxR + 3, cy - 3);  frameCanvas->print("O");
  frameCanvas->setCursor(cx - maxR - 9, cy - 3);  frameCanvas->print("W");

  // Frische Eintraege erst sammeln und stabil nach Konstellation/PRN sortieren (statt in roher Tabellen-
  // Reihenfolge) -- sonst springt ein Satellit in der Liste bei jedem Redraw an eine andere Zeile, sobald
  // satUpdate() oben ihn wegen Tabellenengpass auf einen anderen Platz verdraengt/neu einsortiert hat.
  int idx[SAT_MAX], nIdx = 0;
  uint32_t now = millis();
  for (int i = 0; i < SAT_MAX; i++) if (satTable[i].ms && now - satTable[i].ms < SAT_TIMEOUT_MS) idx[nIdx++] = i;
  for (int i = 1; i < nIdx; i++) {
    int v = idx[i], j = i - 1;
    auto before = [&](int a, int b) {                 // a vor b: erst Talker, dann PRN
      int c = strncmp(satTable[a].talker, satTable[b].talker, 2);
      return c < 0 || (c == 0 && satTable[a].prn < satTable[b].prn);
    };
    while (j >= 0 && before(v, idx[j])) { idx[j + 1] = idx[j]; j--; }
    idx[j + 1] = v;
  }
  int listX = 198, listY = 26, shown = 0, fresh = nIdx;
  for (int k = 0; k < nIdx; k++) {
    SatInfo &e = satTable[idx[k]];
    uint16_t col = satTalkerColor(e.talker);
    if (e.elev >= 0 && e.azim >= 0) {                    // Position bekannt: als Punkt im Polarplot zeichnen
      float r = maxR * (90.0f - (float)e.elev) / 90.0f;
      if (r < 0) r = 0;
      float az = (float)e.azim * (float)M_PI / 180.0f;
      int px = cx + (int)(r * sinf(az));
      int py = cy - (int)(r * cosf(az));
      if (e.snr > 0) frameCanvas->fillCircle(px, py, 3, col);
      else frameCanvas->drawCircle(px, py, 3, col);
    }
    if (listY < 230 && shown < 20) {
      char ln[20];
      snprintf(ln, sizeof(ln), "%.2s%02d %3d %3d %3d", e.talker, e.prn, e.elev, e.azim, e.snr);
      frameCanvas->setTextColor(col);
      frameCanvas->setCursor(listX, listY);
      frameCanvas->print(ln);
      listY += 10;
      shown++;
    }
  }
  frameCanvas->setTextColor(COL_RADAR_TEXT);
  frameCanvas->setCursor(listX, listY);
  if (fresh == 0) frameCanvas->print("Keine GSV-Daten");
  else if (fresh > shown) { char more[20]; snprintf(more, sizeof(more), "+%d weitere", fresh - shown); frameCanvas->print(more); }

  drawToast(frameCanvas);
  drawStatus(frameCanvas, 264, 1);
  blit(frameCanvas);
}

// Aktive grafische Ansicht (Karte oder Hoehentrack/Stationen) neu zeichnen
static void gfxUpdate() {
  if (gfxView == 0) mapUpdate();
  else if (gfxView == 1) altTrackUpdate();
  else satRadarUpdate();
}

// ---------- Bedienung: Tastatur, Trackball, Konfigurationsmenue ----------
// Ereigniscodes: Tastaturzeichen (ASCII) oder Trackball-Ereignisse
enum { EV_NONE = 0, EV_UP = 0x101, EV_DOWN = 0x102, EV_LEFT = 0x103, EV_RIGHT = 0x104, EV_CLICK = 0x105 };

// ---------- Tastenlicht ----------
// Die Tastenbeleuchtung steuert der Tastatur-Controller selbst (Alt+B schaltet sie ebenfalls um); der Sketch schickt ihm nur
// den 2-Byte-Befehl 0x01 + Helligkeit. Es gibt keinen Helligkeitssensor: "dunkel" wird aus Sonnenstand, Uhrzeit (GPS) und Position berechnet.
// Zwei grundsaetzlich verschiedene Betriebsarten (Menue T):
//  - Modus 1 "Taste": Licht an bei Tastendruck, nach KB_ON_MS Inaktivitaet wieder aus (kbTouch()/kbTick()). Der
//    ERSTE Tastendruck, der das Licht aus dem Aus heraus anschaltet, schaltet NUR das Licht an und wird selbst
//    verworfen -- man sieht die Tastatur zu diesem Zeitpunkt ja noch nicht. Erst der naechste Tastendruck
//    (Tastatur jetzt beleuchtet) loest die eigentliche Funktion aus. Siehe pollInput()/pollKeyboardRaw().
//  - Modus 2 "Taste+dunkel": tagsueber (isDark()==false) verhaelt es sich GENAUSO wie Modus 1 (siehe oben) --
//    Licht an bei Tastendruck, erster "blinder" Druck verworfen, nach KB_ON_MS wieder aus. Nachts (isDark()==true)
//    bleibt es dagegen durchgehend an (gedimmt auf KB_LEVEL_DIM, spart Akku/blendet nicht), auch ganz ohne
//    Tastendruck, und geht erst aus, wenn es wieder hell wird. Ein Tastendruck hellt dann nur kurz auf volle
//    Helligkeit auf (KB_BRIGHT_MS) und wird selbst IMMER ausgefuehrt (kein Verwerfen) -- die Tastatur ist ja
//    nachts schon sichtbar beleuchtet, bevor ueberhaupt eine Taste gedrueckt wird.
static double sunElevationDeg(double lat, double lon, double tUnix) {        // Sonnenhoehe ueber dem Horizont [Grad], einfache NOAA-Naeherung
  double jd = tUnix / 86400.0 + 2440587.5;
  double n = jd - 2451545.0;
  double L = fmod(280.460 + 0.9856474 * n, 360.0);
  double g = fmod(357.528 + 0.9856003 * n, 360.0) * M_PI / 180.0;
  double lam = (L + 1.915 * sin(g) + 0.020 * sin(2.0 * g)) * M_PI / 180.0;
  double eps = (23.439 - 0.0000004 * n) * M_PI / 180.0;
  double alpha = atan2(cos(eps) * sin(lam), cos(lam));
  double delta = asin(sin(eps) * sin(lam));
  double gmst = fmod(280.46061837 + 360.98564736629 * n, 360.0);
  double H = (gmst + lon) * M_PI / 180.0 - alpha;
  double la = lat * M_PI / 180.0;
  return asin(sin(la) * sin(delta) + cos(la) * cos(delta) * cos(H)) * 180.0 / M_PI;
}

static bool isDark() {                                                        // ohne gesetzte Uhr gilt es als dunkel
  static uint32_t lastCalc = 0;
  static bool dark = true;
  static bool everCalculated = false;    // true erst NACH der ersten echten Berechnung (mit gesetzter Uhrzeit) -- der
                                          // Default "dark=true" oben ist nur ein Platzhalter bis dahin, keine echte Messung
  uint32_t now = millis();
  if (lastCalc && now - lastCalc < 30000UL) return dark;
  lastCalc = now;
  if (!timeSet) { dark = true; return dark; }
  double la, lo;
  homePosition(la, lo);
  double elev = sunElevationDeg(la, lo, (double)time(nullptr));
  bool newDark = elev < KB_DARK_ELEV;
  // Mitloggen -- NICHT nur bei einem Wechsel, sondern auch bei der allerersten echten Berechnung: wenn es beim
  // Einschalten schon dunkel ist, muss das Licht sofort greifen, und genau dieser (haeufigste) Fall waere mit einer
  // reinen Wechsel-Erkennung nie protokolliert worden (newDark==dark==true, der Platzhalter-Default, saehe wie "kein
  // Wechsel" aus).
  if (!everCalculated || newDark != dark) {
    char tt[12], m[96];
    timeText(tt, sizeof(tt));
    snprintf(m, sizeof(m), "Tastenlicht: %s %s (%s, Sonnenhoehe %.1f Grad, Pos %.4f/%.4f)",
             everCalculated ? "jetzt" : "beim Start", newDark ? "dunkel" : "hell", tt, elev, la, lo);
    logLine(m);
  }
  everCalculated = true;
  dark = newDark;
  return dark;
}

// Rueckgabe true: Wert ist jetzt (oder war schon) gesetzt. Rueckgabe false: I2C-Fehler, am Tastenlicht hat sich
// NICHTS geaendert -- die Aufrufer duerfen dann kbLit NICHT auf true setzen, sonst denkt der Rest des Codes
// dauerhaft "Licht ist an" und versucht es nie wieder, obwohl es in Wirklichkeit dunkel geblieben ist (genau der
// Bug vom Okt. 2026: erster Tastendruck wird wie gewollt verworfen, aber das Licht geht nie wirklich an, weil ein
// einziger fehlgeschlagener I2C-Transfer kbLit schon auf true gesetzt hatte).
static bool kbSetRaw(uint8_t v) {
  static uint8_t cur = 255;                 // 255 = Sentinel, erzwingt beim allerersten Aufruf ein Schreiben
  if (v == cur) return true;                // unveraendert -- keinen unnoetigen I2C-Transfer schicken (Modus 2 ruft
                                             // das jetzt laufend aus kbTick() auf)
  Wire.beginTransmission(KB_ADDR);
  Wire.write((uint8_t)KB_CMD_BRIGHT);
  Wire.write(v);
  uint8_t err = Wire.endTransmission();     // 0 = ok, alles andere ein I2C-Fehler (z. B. NACK/Bus belegt); mitloggen,
                                             // um ein "Licht bleibt trotz dark=true aus" von einem echten
                                             // Uebertragungsfehler unterscheiden zu koennen
  if (err != 0) {
    char m[64];
    snprintf(m, sizeof(m), "Tastenlicht: I2C-Fehler beim Setzen auf %u (Code %u)", v, err);
    logLine(m);
    return false;                           // bei Fehler cur NICHT aktualisieren -- naechster Versuch soll erneut senden
  }
  cur = v;
  return true;
}

static void kbOff() {
  kbSetRaw(0);
  kbLit = false;
}

// Bei jedem Tastendruck: Licht an/heller. Rueckgabe true, wenn das Licht durch GENAU DIESEN Tastendruck gerade
// erst angeschaltet wurde (vorher komplett aus) -- die Aufrufer nutzen das, um einen ersten "blinden" Tastendruck
// (Taste wird gedrueckt, aber man kann die Tastatur noch nicht sehen) nur zum Anschalten zu verwenden und NICHT
// gleich auch noch als Funktionstaste auszufuehren.
//  - Modus 1 "Taste": Licht war aus -> an und erster Druck wird verworfen (wie bisher); Licht war schon an ->
//    Druck wird ganz normal ausgefuehrt, nur der KB_ON_MS-Timer wird verlaengert.
//  - Modus 2 "Taste+dunkel" tagsueber: identisch zu Modus 1 (siehe kbTouchLikeMode1() unten).
//  - Modus 2 "Taste+dunkel" nachts: das Licht ist ohnehin schon an (gedimmt, siehe kbTick()), man sieht die
//    Tastatur also immer schon, BEVOR man eine Taste drueckt -- deshalb wird hier nie etwas verworfen. Ein
//    Tastendruck hellt nur kurz auf volle Helligkeit auf (KB_BRIGHT_MS), kbTick() dimmt danach wieder zurueck.
static bool kbTouchLikeMode1(int modeForLog) {    // Modus-1-Verhalten, auch von Modus 2 tagsueber genutzt
  bool justLit = !kbLit;
  if (!kbLit) {
    if (kbSetRaw(KB_LEVEL)) {
      kbLit = true;
      char m[56];
      snprintf(m, sizeof(m), "Tastenlicht: an (Modus %d, Helligkeit %d)", modeForLog, KB_LEVEL);
      logLine(m);
    }
  }
  kbOffAt = millis() + KB_ON_MS;
  return justLit;
}

static bool kbTouch() {
  if (kbMode == 2) {
    if (!isDark()) return kbTouchLikeMode1(kbMode);   // tagsueber: wie Modus 1
    if (kbLit) {                                  // nachts ist das Licht ohnehin schon an (gedimmt) --
      kbSetRaw(KB_LEVEL);                          // ein Tastendruck hellt nur kurz auf
      kbBrightUntil = millis() + KB_BRIGHT_MS;
    }
    return false;                                 // nachts wird ein Tastendruck nie verworfen
  }
  if (kbMode != 1) return false;                  // Modus 0 (aus) reagiert nicht auf Tastendruck
  return kbTouchLikeMode1(kbMode);
}

// Haelt das Tastenlicht aktuell -- wird laufend (nicht nur bei Tastendruck) aus pollInput()/pollKeyboardRaw()
// aufgerufen, damit alle Modi ohne Verzoegerung reagieren:
//  - Modus 1 ("Taste"): schaltet KB_ON_MS nach dem letzten Tastendruck wieder aus (unveraendert).
//  - Modus 2 ("Taste+dunkel") tagsueber: identisch zu Modus 1 -- schaltet KB_ON_MS nach dem letzten Tastendruck
//    (gesetzt in kbTouchLikeMode1()) wieder aus.
//  - Modus 2 ("Taste+dunkel") nachts: folgt NICHT dem Tastendruck, sondern direkt der Helligkeit draussen --
//    bleibt durchgehend an (gedimmt auf KB_LEVEL_DIM), solange es dunkel ist, auch ohne jeden Tastendruck, und
//    geht erst aus, wenn es wieder hell wird (dann uebernimmt ab sofort wieder die Modus-1-Logik oben). Ein
//    Tastendruck (kbTouch()) hellt kurz auf; nach KB_BRIGHT_MS ohne weiteren Druck dimmt kbTick() hier
//    automatisch wieder zurueck.
static void kbTick() {
  if (kbMode == 2) {
    if (isDark()) {
      if (!kbLit) {
        if (kbSetRaw(KB_LEVEL_DIM)) {
          kbLit = true;
          kbBrightUntil = 0;
          logLine("Tastenlicht: an (Modus 2, gedimmt, dauerhaft bis wieder hell)");
        }
      } else if (kbBrightUntil && (int32_t)(millis() - kbBrightUntil) >= 0) {
        kbSetRaw(KB_LEVEL_DIM);                   // Tastendruck ist her, zurueck auf die gedimmte Grundhelligkeit
        kbBrightUntil = 0;
      }
      return;
    }
    // tagsueber: wie Modus 1 -- bei Uebergang von Dunkel auf Hell ist kbOffAt noch nicht (sinnvoll) gesetzt,
    // das gedimmte Licht geht dann wie bisher sofort aus; danach steuert jeder Tastendruck ganz normal ueber
    // kbTouchLikeMode1()/kbOffAt.
    if (kbLit && (int32_t)(millis() - kbOffAt) >= 0) kbOff();
    return;
  }
  if (kbLit && (int32_t)(millis() - kbOffAt) >= 0) kbOff();
}

// Nur das Tastatur-IC roh abfragen (kein Menue, kein gfxUpdate!) und die Taste zwischenspeichern: wird aus dem
// Kartenrendern (renderBackground, kann pro Aufruf lange dauern) aufgerufen, damit dabei keine Taste verloren geht
// -- das IC merkt sich nur die jeweils letzte Taste, keine Warteschlange.
static void pollKeyboardRaw() {
  kbTick();
  if (pendingKeyChar) return;                     // schon eine Taste zwischengespeichert: nicht ueberschreiben
  if (Wire.requestFrom((uint8_t)KB_ADDR, (uint8_t)1) == 1) {
    int c = Wire.read();
    if (c > 0 && !kbTouch()) pendingKeyChar = c;    // kbTouch()==true: dieser Druck hat gerade erst das Licht angeschaltet
  }                                                  // -- Taste verwerfen (verwerfen statt zwischenspeichern), nicht ausfuehren
}

static void inputBegin() {
  Wire.begin(PIN_KB_SDA, PIN_KB_SCL);
  kbSetRaw(0);
  // Nach einem WLAN/OTA-Upload loest esp_restart() (von ArduinoOTA nach dem Schreiben des Updates selbst
  // aufgerufen) auf diesem Chip einen RTC_SW_CPU_RST aus -- das resettet NUR den CPU-Kern, nicht aber die
  // GPIO-/IO-Mux-Peripherieregister (anders als ein serieller Upload, der ueber den EN-Pin einen echten
  // Hardware-Reset ausloest). Die alte, noch laufende Firmware hatte die Trackball-Pins zu diesem Zeitpunkt
  // bereits per pinMode()+attachInterrupt() konfiguriert (Pullup + Flanken-Interrupt aktiv); diese rohe
  // Hardwarekonfiguration ueberlebt den CPU-Reset unveraendert, und pinMode() allein setzt eine noch aktive
  // Interrupt-Freigabe nicht zurueck -- das fuehrte zu einem Haenger direkt im ersten pinMode()-Aufruf danach.
  // Fix: jeden Trackball-Pin vor der eigentlichen Konfiguration per gpio_reset_pin() hart auf den
  // Auslieferungszustand zuruecksetzen (IO-Mux-Funktion, Pull-Widerstaende UND Interrupt-Freigabe).
  gpio_reset_pin((gpio_num_t)PIN_TB_CLICK);
  gpio_reset_pin((gpio_num_t)PIN_TB_UP);
  gpio_reset_pin((gpio_num_t)PIN_TB_DOWN);
  gpio_reset_pin((gpio_num_t)PIN_TB_LEFT);
  gpio_reset_pin((gpio_num_t)PIN_TB_RIGHT);
  pinMode(PIN_TB_CLICK, INPUT_PULLUP);
  pinMode(PIN_TB_UP, INPUT_PULLUP);
  pinMode(PIN_TB_DOWN, INPUT_PULLUP);
  pinMode(PIN_TB_LEFT, INPUT_PULLUP);
  pinMode(PIN_TB_RIGHT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TB_CLICK), onButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_TB_UP), isrUp, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_TB_DOWN), isrDown, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_TB_LEFT), isrLeft, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_TB_RIGHT), isrRight, FALLING);
}

// Liefert hoechstens ein Ereignis pro Aufruf (Abfrage alle 25 ms): Klick, Trackball-Richtung oder Taste
static int pollInput() {
  static uint32_t last = 0;
  static int aU = 0, aD = 0, aL = 0, aR = 0;
  if (pendingKeyChar) {                                  // von pollKeyboardRaw() waehrend eines langen Kartenrenderns
    int c = pendingKeyChar; pendingKeyChar = 0;           // gemerkte Taste: sofort liefern, nicht auf das 25-ms-Raster warten
    Serial.printf("Eingabe: Taste 0x%02X (zwischengespeichert)\n", c);
    return c;
  }
  uint32_t now = millis();
  if (now - last < 25) return EV_NONE;
  uint32_t gap = last ? (now - last) : 0;   // Pan-Diagnose: Abstand zur letzten Abfrage -- sollte ~25 ms sein;
  last = now;                               // deutlich mehr heisst, der Hauptloop (und damit pollInput()) war
                                             // zwischendurch blockiert, z. B. in renderBackground()/blit()
  kbTick();
  if (clickReq) { clickReq = false; Serial.println("Eingabe: Trackball-Klick"); return EV_CLICK; }
  int u = tbU, d = tbD, l = tbL, r = tbR;
  tbU = 0;
  tbD = 0;
  tbL = 0;
  tbR = 0;
  if (tbLockMode >= 1) { u = d = l = r = 0; }            // Trackball-Sperre (Stufe 1+): Impulse verwerfen
  if (u || d || l || r) {   // nur loggen, wenn der Trackball sich gerade bewegt -- sonst flutet das Log
    Serial.printf("PAN Impuls: u%d d%d l%d r%d, Abfrageluecke %lu ms\n", u, d, l, r, (unsigned long)gap);
  }
  panAccX = constrain(panAccX + r - l, -2000, 2000);     // fuer das Scrollen der Karte (siehe mapNavTick)
  panAccY = constrain(panAccY + d - u, -2000, 2000);
  aU = u ? aU + u : 0;
  aD = d ? aD + d : 0;
  aL = l ? aL + l : 0;
  aR = r ? aR + r : 0;
  if (aU >= TB_PULSES) { aU = 0; Serial.println("Eingabe: Trackball hoch"); return EV_UP; }
  if (aD >= TB_PULSES) { aD = 0; Serial.println("Eingabe: Trackball runter"); return EV_DOWN; }
  if (aL >= TB_PULSES) { aL = 0; Serial.println("Eingabe: Trackball links"); return EV_LEFT; }
  if (aR >= TB_PULSES) { aR = 0; Serial.println("Eingabe: Trackball rechts"); return EV_RIGHT; }
  if (Wire.requestFrom((uint8_t)KB_ADDR, (uint8_t)1) == 1) {
    int c = Wire.read();
    if (c > 0) {
      if (kbTouch()) {                              // dieser Druck hat gerade erst das Licht angeschaltet: nicht
        Serial.printf("Eingabe: Taste 0x%02X nur Beleuchtung an (Taste verworfen)\n", c);   // ausfuehren, man hat
      } else {                                       // die Tastatur ja noch gar nicht gesehen, bevor sie leuchtete
        Serial.printf("Eingabe: Taste 0x%02X\n", c);
        return c;
      }
    }
  }
  return EV_NONE;
}

// ---- Einstellungen speichern/laden
static void applyBrightness() {
  int duty = 20 + ((int)brightPct - 10) * 235 / 90;     // 10 % -> 20/255, 100 % -> 255/255: das Display geht nie ganz aus
  analogWrite(PIN_TFT_BL, constrain(duty, 20, 255));
}

static void loadSettings() {
  prefs.begin("rs41", false);
  fixedFreq = prefs.getBool("fixed", false);
  freqMHz = prefs.getFloat("freq", 405.300f);
  predOn = prefs.getBool("pred", true);
  landAlt = prefs.getDouble("land", LAND_ALT_DEFAULT);
  panPx = (uint8_t)constrain((int)prefs.getUChar("panpx", 5), 1, 30);
  tbLockMode = (uint8_t)(prefs.getUChar("tblockm", 0) % 3);   // neuer 3-stufiger Schluessel (tblockm) statt des alten
                                                               // bool-Schluessels "tblock" -- Umstieg genuegt (0 = aus)
  ownTrailOn = prefs.getBool("owntr", true);
  beepSec = (uint8_t)constrain((int)prefs.getUChar("beepsec", 0), 0, 20);
  brightPct = constrain((int)prefs.getUChar("brpct", 100), 10, 100);
  kbMode = prefs.getUChar("kbmode", 2) % 3;
  gpxOn = prefs.getBool("gpxon", true);
  sdLogOn = prefs.getBool("sdlog", SD_LOG);
  sondehubMode = (uint8_t)(prefs.getUChar("shub", 0) % 2);   // nur noch 0 aus / 1 Chase-Car, s. sondehubMode-Deklaration
  rxBw = prefs.getFloat("bw", RX_BW_DEFAULT);
  fixOn = prefs.getBool("fix", true);
  mapStyle = prefs.getUChar("mstyle4", 0) % NSTYLE;
  shotCount = prefs.getUInt("shotn", 0);
  {
    String ss = prefs.getString("ssid", "");
    String pw = prefs.getString("wpass", "");
    strncpy(wifiSsid, ss.c_str(), sizeof(wifiSsid) - 1);
    wifiSsid[sizeof(wifiSsid) - 1] = 0;
    strncpy(wifiPass, pw.c_str(), sizeof(wifiPass) - 1);
    wifiPass[sizeof(wifiPass) - 1] = 0;
  }
  {
    String rb = prefs.getString("recby", "");
    strncpy(gpxRecoveredBy, rb.c_str(), sizeof(gpxRecoveredBy) - 1);
    gpxRecoveredBy[sizeof(gpxRecoveredBy) - 1] = 0;
  }
  savedView = prefs.getUChar("view", 0);
  if (savedView > 3) savedView = 3;            // 0 Text, 1 Karte, 2 Hoehentrack/Stationen, 3 GPS-Radar (neu,
                                                // siehe satRadarUpdate()); ungueltige/aeltere Werte -> Radar
  homeLat = prefs.getDouble("hlat", HOME_LAT);
  homeLon = prefs.getDouble("hlon", HOME_LON);
  radioMode = (uint8_t)(prefs.getUChar("rmode", 0) % 2);   // Menue I: 0 Sondenempfang / 1 LoRa-APRS
  if (radioMode == 1 && savedView == 0) savedView = 1;     // LoRa-Modus: Textansicht (Frequenzscan) gibt's nicht -> auf Karte ausweichen
  txPowerDbm = (int8_t)constrain((int)prefs.getChar("txpwr", 10), -9, 22);   // Menue U (LoRa-Beacon-Sendeleistung)
  loraTxWasOn = prefs.getBool("loratx", false);            // siehe confirmLoraTxResume() in setup()
}

static void saveSettings() {
  prefs.putBool("fixed", fixedFreq);
  prefs.putFloat("freq", freqMHz);
  prefs.putBool("pred", predOn);
  prefs.putDouble("land", landAlt);
  prefs.putUChar("panpx", panPx);
  prefs.putUChar("tblockm", tbLockMode);
  prefs.putBool("owntr", ownTrailOn);
  prefs.putUChar("beepsec", beepSec);
  prefs.putUChar("brpct", brightPct);
  prefs.putUChar("kbmode", kbMode);
  prefs.putBool("gpxon", gpxOn);
  prefs.putBool("sdlog", sdLogOn);
  prefs.putUChar("shub", sondehubMode);
  prefs.putFloat("bw", rxBw);
  prefs.putBool("fix", fixOn);
  prefs.putUChar("mstyle4", mapStyle);
  prefs.putUChar("rmode", radioMode);
  prefs.putChar("txpwr", txPowerDbm);
  prefs.putBool("loratx", loraTxOn);                       // siehe confirmLoraTxResume() in setup()
}

// ---- Ansichten: Text -> Karte -> Hoehentrack/Stationen -> GPS-Radar -> Text
// Im LoRa-Modus gibt es keine sinnvolle Textansicht (der Frequenzscan ist Sonde-spezifisch) -- dort wird
// sie komplett uebersprungen und nur zwischen Karte, Stationsliste und GPS-Radar umgeschaltet.
static void advanceView(int dir) {               // nur den Zustand aendern, nicht zeichnen
  if (radioMode == 1) {
    int v = mapMode ? (gfxView + 1) % 3 : 0;      // aus Text (sollte im LoRa-Modus nicht vorkommen) -> Karte
    mapMode = true;
    gfxView = v;
    return;
  }
  int v = !mapMode ? 0 : gfxView + 1;             // 0 Text, 1 Karte, 2 Hoehentrack/Stationen, 3 GPS-Radar
  v = (v + (dir > 0 ? 1 : 3)) % 4;
  mapMode = (v != 0);
  gfxView = mapMode ? (v - 1) : 0;
}

static void showView() {                          // Bildschirm passend zur aktuellen Ansicht neu aufbauen
  {                                               // Ansicht merken (nur schreiben, wenn sie sich geaendert hat)
    uint8_t v = !mapMode ? 0 : gfxView + 1;
    if (prefs.getUChar("view", 255) != v) prefs.putUChar("view", v);
  }
  scrClear();
  if (mapMode) {
    if (gfxReady()) {                             // auch ohne Sonde: Karte um die eigene Position
      viewZ = -1;
      mapIdleShown = false;
      panAccX = 0; panAccY = 0; zoomPend = 0;      // beim Menue angesammelte Trackball-Impulse verwerfen
      // rFocus bewusst NICHT zuruecksetzen: Menue oeffnen/schliessen soll den gewaehlten Fokus (Sonde/Ich/...)
      // nicht verwerfen -- auf neue Sonde/erste Position wird er ohnehin an anderer Stelle neu auf 0 gesetzt.
      gfxUpdate();
    } else {
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(10, 110);
      tft.print("Ansicht nicht verfuegbar (kein PSRAM)");
    }
  } else {
    line(0, TITLE_TEXT, ST77XX_CYAN);
    statusRefresh();
  }
}

static void cycleView() {
  advanceView(1);
  showView();
}

// ---- Menuepunkte (der Buchstabe waehlt nur den Punkt)
// MI_ZSTEP ("Zoomschritt", Taste Y) im Okt. 2026 entfernt: seit FW 3.0 wird ohnehin immer um genau eine ganze
// Zoomstufe gezoomt (effZoom() rundet sowieso), die Schrittweite war also nur noch totes Menue. Taste H (vorher
// MI_BRIGHT/Helligkeit) zeigt seitdem direkt die Hilfe in der Kartenansicht (siehe showHelpScreen()); Helligkeit
// ist dafuer auf die frei gewordene Taste Y gewandert.
enum { MI_SEARCH, MI_FREQ, MI_BW, MI_FIX, MI_ID, MI_PRED, MI_LAND, MI_ZOOM, MI_PAN, MI_TBLOCK, MI_OWN, MI_BEEP, MI_MAP, MI_BRIGHT, MI_KB, MI_GPX, MI_SDLOG, MI_WIFI, MI_NET, MI_SHUB, MI_TXPWR, MI_SHUTDOWN, MI_COUNT };
static const char menuKey[MI_COUNT] = {'s', 'f', 'b', 'k', 'i', 'v', 'l', 'z', 'g', 'r', 'o', 'x', 'm', 'y', 't', 'j', 'p', 'w', 'e', 'a', 'u', 'q'};
static const char *const menuName[MI_COUNT] = {"Suche", "Frequenz", "Bandbreite", "Fehlerkorr.", "Funkmodus", "Vorhersage", "Landehoehe", "Zoom", "Scroll-Empf.", "Trackball-Sperre", "Eigener Track", "Ton", "Kartenart", "Helligkeit", "Tastatur", "GPX", "SD-Log", "WLAN", "Netz", "SondeHub-Upload", "Sendeleistung", "Herunterfahren"};

// Anzeigename eines Menuepunkts -- wie menuName[], aber MI_SHUB heisst im LoRa-Modus "LoRa Upload" statt
// "SondeHub-Upload" (siehe menuValue()/menuStep() MI_SHUB: derselbe Menuepunkt ist dort je nach radioMode
// umgewidmet). Ueberall verwenden, wo der Menuename angezeigt/geloggt wird (Geraete-Menue, Weboberflaeche,
// Logzeilen) -- menuName[] selbst bleibt die reine Lookup-Tabelle.
static const char *menuDisplayName(int i) { return (i == MI_SHUB && radioMode == 1) ? "LoRa Upload" : menuName[i]; }
// Taste A fuer SondeHub-Upload: auf der Kartenseite liegt A schon auf dem Zoom (A/D links, O/P rechts, siehe
// handleInput()) -- dort oeffnet A also weiter den Zoom, nicht dieses Menue; ausserhalb der Karte (Hoehentrack/
// Stationen, Ruhebild) oder ueber Enter+Auswahl ist es ganz normal erreichbar. Gleiches Prinzip wie schon bei R/M.
static const float bwList[7] = {9.7f, 11.7f, 14.6f, 19.5f, 23.4f, 29.3f, 39.0f};   // gueltige RX-Bandbreiten des SX1262 [kHz]

static char editBuf[12];                          // getippter Zahlenwert
static int editLen = 0;
static bool editing = false;
static char menuNote[56] = "";                    // kurze Meldung im Menue (z. B. Screenshot)
static uint32_t menuNoteUntil = 0;
static bool shutdownConfirm = false;               // "Herunterfahren": Sicherheitsabfrage
static uint32_t shutdownConfirmMs = 0;

static bool menuIsNumeric(int i) { return i == MI_FREQ || i == MI_LAND || i == MI_ZOOM || i == MI_PAN || i == MI_BEEP || i == MI_BRIGHT || i == MI_TXPWR; }

// Beginnt beim Einschalten ein frisches Log fuer diese Sitzung: ohne Platz-Rotation, einfach unter der
// ersten noch freien Platzhalter-Nummer rs41_log_01.txt .. _<LOG_FALLBACK_MAX>.txt hochgezaehlt (nichts wird
// dabei ueberschrieben). Nur im unrealistischen Fall, dass wirklich alle Nummern bis LOG_FALLBACK_MAX belegt
// sind, faengt es wieder bei 01 an (und ueberschreibt dann die aelteste). Mehr als ein paar Platzhalter sollten
// ohnehin nie entstehen, weil onTimeSet() die Datei sofort umbenennt, sobald GPS/NTP eine Uhrzeit liefert.
static void startLog() {
  logActivePath[0] = 0;
  if (!sdReady) return;
  char pth[48];
  int num = 1;
  for (; num <= LOG_FALLBACK_MAX; num++) {
    logFallbackPathFor(pth, sizeof(pth), num);
    if (!SD.exists(pth)) break;
  }
  if (num > LOG_FALLBACK_MAX) { num = 1; logFallbackPathFor(pth, sizeof(pth), num); }
  if (SD.exists(pth)) SD.remove(pth);
  File f = SD.open(pth, FILE_WRITE);
  if (f) {
    char t[40], m[80];
    if (timeSet) dateTimeText(t, sizeof(t));
    else snprintf(t, sizeof(t), "Laufzeit %lu s", (unsigned long)(millis() / 1000));
    snprintf(m, sizeof(m), "### Start (%s)", t);
    f.println(m);
    f.close();
  }
  strncpy(logActivePath, pth, sizeof(logActivePath) - 1);
  Serial.printf("Log: %s\n", pth);
  gpxDrainAll();                          // Gepuffertes noch in die alten Dateien schreiben ...
  gpxCloseTrack(gpxIch);                  // ... naechster "Ich"-Punkt beginnt ein neues <trkseg> in der heutigen Datei
  gpxCloseTrack(gpxSonde);
}

// ---------- Screenshot (Taste C) und WLAN-Abruf ----------
// EXIF-Block (TIFF-Struktur, little endian): ImageDescription, Software, DateTime und GPS-Angaben.
// out beginnt mit dem TIFF-Header (ohne APP1-Segmentkopf). Rueckgabe: Laenge in Bytes, 0 bei Fehler.
static int buildExif(uint8_t *out, int cap, double lat, double lon, double alt, const char *desc) {
  if (cap < 700) return 0;
  memset(out, 0, cap);
  auto p16 = [&](int o, uint16_t v) { out[o] = v & 255; out[o + 1] = v >> 8; };
  auto p32 = [&](int o, uint32_t v) {
    out[o] = v & 255; out[o + 1] = (v >> 8) & 255; out[o + 2] = (v >> 16) & 255; out[o + 3] = (v >> 24) & 255;
  };
  out[0] = 'I'; out[1] = 'I'; p16(2, 42); p32(4, 8);
  struct tm loc, utc;
  bool tm = timeSet;
  if (tm) { time_t t = time(nullptr); localtime_r(&t, &loc); gmtime_r(&t, &utc); }
  const int n0 = tm ? 4 : 3;
  int dataPtr = 8 + 2 + n0 * 12 + 4;
  auto putData = [&](const void *src, int len) -> int {
    int off = dataPtr;
    if (off + len + 1 > cap) return 0;
    memcpy(out + off, src, len);
    dataPtr += len + (len & 1);
    return off;
  };
  auto entry = [&](int &pos, uint16_t tag, uint16_t type, uint32_t count, uint32_t val) {
    p16(pos, tag); p16(pos + 2, type); p32(pos + 4, count); p32(pos + 8, val); pos += 12;
  };
  int e = 8;
  p16(e, n0);
  e += 2;
  char dbuf[204];
  int dlen = (int)strlen(desc);
  if (dlen > 200) dlen = 200;
  memcpy(dbuf, desc, dlen);
  dbuf[dlen] = 0;
  entry(e, 0x010E, 2, dlen + 1, putData(dbuf, dlen + 1));            // ImageDescription
  entry(e, 0x0131, 2, 12, putData("T-Deck RS41", 12));               // Software
  if (tm) {
    char dt[64];
    snprintf(dt, sizeof(dt), "%04d:%02d:%02d %02d:%02d:%02d", loc.tm_year + 1900, loc.tm_mon + 1, loc.tm_mday, loc.tm_hour, loc.tm_min, loc.tm_sec);
    entry(e, 0x0132, 2, 20, putData(dt, 20));                         // DateTime (Ortszeit)
  }
  int gpsPtr = e;
  entry(e, 0x8825, 4, 1, 0);                                          // GPSInfo (Offset wird unten eingetragen)
  p32(e, 0);                                                          // kein weiteres IFD

  const int nG = tm ? 10 : 8;
  int gpsOff = dataPtr;
  dataPtr += 2 + nG * 12 + 4;
  p32(gpsPtr + 8, gpsOff);
  int g = gpsOff;
  p16(g, nG);
  g += 2;
  auto degRat = [&](double v, uint32_t r[6]) {                         // Grad, Minuten, Sekunden als Bruchzahlen
    double a = fabs(v);
    uint32_t d = (uint32_t)a;
    double mm = (a - d) * 60.0;
    uint32_t m = (uint32_t)mm;
    double sec = (mm - m) * 60.0;
    r[0] = d; r[1] = 1; r[2] = m; r[3] = 1; r[4] = (uint32_t)(sec * 10000.0 + 0.5); r[5] = 10000;
  };
  { uint8_t v[4] = {2, 3, 0, 0}; uint32_t val; memcpy(&val, v, 4); entry(g, 0x0000, 1, 4, val); }                 // GPSVersionID
  { uint8_t v[4] = {(uint8_t)(lat >= 0 ? 'N' : 'S'), 0, 0, 0}; uint32_t val; memcpy(&val, v, 4); entry(g, 0x0001, 2, 2, val); }
  { uint32_t r[6]; degRat(lat, r); entry(g, 0x0002, 5, 3, putData(r, 24)); }                                        // GPSLatitude
  { uint8_t v[4] = {(uint8_t)(lon >= 0 ? 'E' : 'W'), 0, 0, 0}; uint32_t val; memcpy(&val, v, 4); entry(g, 0x0003, 2, 2, val); }
  { uint32_t r[6]; degRat(lon, r); entry(g, 0x0004, 5, 3, putData(r, 24)); }                                        // GPSLongitude
  entry(g, 0x0005, 1, 1, alt < 0 ? 1 : 0);                                                                           // GPSAltitudeRef
  { uint32_t r[2] = {(uint32_t)(fabs(alt) * 100.0 + 0.5), 100}; entry(g, 0x0006, 5, 1, putData(r, 8)); }          // GPSAltitude
  if (tm) { uint32_t r[6] = {(uint32_t)utc.tm_hour, 1, (uint32_t)utc.tm_min, 1, (uint32_t)utc.tm_sec, 1}; entry(g, 0x0007, 5, 3, putData(r, 24)); }
  entry(g, 0x0012, 2, 7, putData("WGS-84", 7));                                                                     // GPSMapDatum
  if (tm) {
    char ds[32];
    snprintf(ds, sizeof(ds), "%04d:%02d:%02d", utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday);
    entry(g, 0x001D, 2, 11, putData(ds, 11));                                                                       // GPSDateStamp (UTC)
  }
  p32(g, 0);
  return dataPtr;
}

static JPEGENC jpgEnc;
static JPEGENCODE jpgEc;

// Kodiert den aktuellen Bildschirminhalt (frameCanvas) als JPEG in einen vom Aufrufer bereitgestellten
// Speicherpuffer (mind. 'cap' Byte) -- ohne EXIF, ohne SD-Karte. Genutzt sowohl von takeScreenshot()
// (die danach noch EXIF anfuegt und auf die SD schreibt) als auch vom Live-Bild der Weboberflaeche
// (webLive(), direkt an den Browser). Rueckgabe: Groesse in Byte, 0 bei Fehler.
static int encodeFrameJpeg(uint8_t *out, size_t cap) {
  if (!frameCanvas) return 0;
  int rc = jpgEnc.open(out, (int)cap);
  if (rc == JPEGE_SUCCESS) rc = jpgEnc.encodeBegin(&jpgEc, 320, 240, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_420, JPEGE_Q_HIGH);
  uint16_t *buf = frameCanvas->getBuffer();
  for (int y = 0; y < 240 && rc == JPEGE_SUCCESS; y += 16)                // 4:2:0 = MCUs mit 16 x 16 Pixeln
    for (int x = 0; x < 320 && rc == JPEGE_SUCCESS; x += 16)
      rc = jpgEnc.addMCU(&jpgEc, (uint8_t *)(buf + y * 320 + x), 320 * 2);
  int size = jpgEnc.close();
  if (rc != JPEGE_SUCCESS || size < 100 || out[0] != 0xFF || out[1] != 0xD8) return 0;
  return size;
}

// Speichert den aktuellen Bildschirm als JPEG mit EXIF (Koordinaten der Sonde, sonst des T-Deck) unter /shots.
static bool takeScreenshot(bool inMenu, char *msg, size_t msgLen) {
  if (!frameCanvas) { snprintf(msg, msgLen, "kein Bildpuffer"); return false; }
  if (!sdReady) { snprintf(msg, msgLen, "keine SD-Karte"); return false; }
  double hl, ho;
  bool gf = homePosition(hl, ho);
  double lat = hl, lon = ho, alt = 0;
  char desc[200];
  if (haveSondePos) {                              // Koordinaten der Sonde; die des T-Deck stehen in der Beschreibung
    lat = sondeLat; lon = sondeLon; alt = sondeAlt;
    snprintf(desc, sizeof(desc), "Sonde %s Nr %d: %.5f %.5f %.0f m | T-Deck: %.5f %.5f (%s)", sondeId, sondeFnr, sondeLat, sondeLon,
             sondeAlt, hl, ho, gf ? "GPS" : "letzte Position");
  } else {                                         // noch keine Sonde: Koordinaten des T-Deck
    snprintf(desc, sizeof(desc), "T-Deck: %.5f %.5f (%s), keine Sonde empfangen", hl, ho, gf ? "GPS" : "letzte Position");
  }
  const size_t cap = 96 * 1024;
  uint8_t *out = (uint8_t *)ps_malloc(cap);
  if (!out) out = (uint8_t *)malloc(cap);
  if (!out) { snprintf(msg, msgLen, "kein Speicher"); return false; }
  int size = encodeFrameJpeg(out, cap);
  if (size <= 0) {
    free(out);
    snprintf(msg, msgLen, "JPEG-Fehler");
    return false;
  }
  uint8_t exif[700];
  int en = buildExif(exif, sizeof(exif), lat, lon, alt, desc);
  const char *vn = inMenu ? "menue" : (!mapMode ? "text" : (gfxView == 0 ? "karte" : (gfxView == 1 ? "hoehentrack" : "gps-radar")));
  char name[64], path[96];
  if (timeSet) {
    time_t t = time(nullptr);
    struct tm lt;
    localtime_r(&t, &lt);
    snprintf(name, sizeof(name), "shot_%04d%02d%02d_%02d%02d%02d_%s.jpg", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, vn);
  } else {
    snprintf(name, sizeof(name), "shot_%05lu_%s.jpg", (unsigned long)shotCount, vn);
  }
  shotCount++;
  prefs.putUInt("shotn", shotCount);
  if (!SD.exists(SHOT_DIR)) SD.mkdir(SHOT_DIR);
  snprintf(path, sizeof(path), "%s/%s", SHOT_DIR, name);
  File f = SD.open(path, FILE_WRITE);
  if (!f) { free(out); snprintf(msg, msgLen, "Datei nicht schreibbar"); return false; }
  f.write(out, 2);                                  // SOI
  if (en > 0) {                                     // APP1 (EXIF) direkt nach SOI
    uint8_t hdr[10] = {0xFF, 0xE1, (uint8_t)((en + 8) >> 8), (uint8_t)((en + 8) & 255), 'E', 'x', 'i', 'f', 0, 0};
    f.write(hdr, 10);
    f.write(exif, en);
  }
  f.write(out + 2, size - 2);                       // Rest des JPEG
  f.close();
  touchFileTime(path);   // sonst bliebe der Screenshot dauerhaft auf dem SD-Anlege-Zeitstempel (1980) stehen
  free(out);
  snprintf(msg, msgLen, "%s", name);
  char lg[140];
  snprintf(lg, sizeof(lg), "Screenshot: %s (%d kB), Koordinaten: %s", path, (size + 512) / 1024, haveSondePos ? "Sonde" : "T-Deck");
  logLine(lg);
  return true;
}

// ---- WLAN-Zugangspunkt mit kleiner Webseite: Screenshots und Logs abrufen
static WebServer server(80);

// Wird aus renderBackground() heraus zwischen den Kacheln aufgerufen (kann pro Kachel etliche ms dauern):
// Webserver bedienen (sonst haengt z. B. ein Log-Download waehrend des Kartenrenderns) und Tastatur roh
// abfragen (siehe pollKeyboardRaw). Bewusst KEIN handleInput()/gfxUpdate() hier -- das wuerde rekursiv
// wieder ins Kartenrendern hineinlaufen koennen (Stapelueberlauf).
static void pumpDuringRender() {
  if (wifiOn) { server.handleClient(); ArduinoOTA.handle(); }
  pollKeyboardRaw();
}

// Debug: protokolliert JEDEN gesehenen Verzeichniseintrag (auch nicht passende), damit sich im Log
// nachvollziehen laesst, was die SD-Bibliothek tatsächlich zurueckliefert (vorübergehend fuer die
// Fehlersuche "Web-Auflistung zeigt nach dem Loeschen zu wenige/keine Dateien an").
#define LISTDIR_DEBUG 0        // war fuer eine einmalige Fehlersuche an -- jeder Eintrag schreibt sonst per logLine()
                                // einen eigenen SD-Open/Write/Close-Zyklus, was listDir() (u.a. Taste Q) stark ausbremst

// onProgress (optional): wird waehrend eines langsamen Scans (viele Dateien, z. B. Taste Q) alle paar Eintraege
// mit der bisherigen Anzahl gesehener Eintraege aufgerufen, damit der Aufrufer einen Fortschrittsbalken zeichnen
// kann -- die uebrigen Aufrufer (Web-Oberflaeche) lassen das Argument einfach weg.
static void listDir(const char *dir, const char *prefix, const char *suffix, std::vector<FileInfo> &v, void (*onProgress)(int) = nullptr) {
  File d = SD.open(dir);
  if (!d || !d.isDirectory()) {
#if LISTDIR_DEBUG
    char m[80];
    snprintf(m, sizeof(m), "WLAN: listDir(%s): Verzeichnis nicht oeffenbar (d=%d)", dir, (int)(bool)d);
    logLine(m);
#endif
    if (d) d.close();
    return;
  }
#if LISTDIR_DEBUG
  int seen = 0, dirs = 0, matched = 0;
#endif
  int n = 0;
  uint32_t lastProgMs = 0;
  File e = d.openNextFile();
  while (e && v.size() < 400) {
    n++;
    if (onProgress && (millis() - lastProgMs >= 120)) { lastProgMs = millis(); onProgress(n); }
#if LISTDIR_DEBUG
    seen++;
#endif
    if (!e.isDirectory()) {
      const char *nm = strrchr(e.name(), '/');
      nm = nm ? nm + 1 : e.name();
      String sn(nm);
      bool ok = sn.startsWith(prefix) && sn.endsWith(suffix);
      if (ok) {
        v.push_back({sn, (uint32_t)e.size(), e.getLastWrite()});
#if LISTDIR_DEBUG
        matched++;
#endif
      }
#if LISTDIR_DEBUG
      char m[96];
      snprintf(m, sizeof(m), "WLAN: listDir(%s) Eintrag: '%s' (%u B) -> %s", dir, sn.c_str(), (unsigned)e.size(), ok ? "match" : "kein Match");
      logLine(m);
#endif
    }
#if LISTDIR_DEBUG
    else dirs++;
#endif
    e.close();
    e = d.openNextFile();
  }
  d.close();
  if (onProgress) onProgress(n);                      // Endstand, damit der Balken sichtbar fertig wird
  std::sort(v.begin(), v.end(), [](const FileInfo &a, const FileInfo &b) { return a.name > b.name; });  // neueste zuerst
#if LISTDIR_DEBUG
  char m2[96];
  snprintf(m2, sizeof(m2), "WLAN: listDir(%s): %d Eintraege gesehen (%d Ordner), %d passend", dir, seen, dirs, matched);
  logLine(m2);
#endif
}

static bool safeName(const String &n, const char *prefix, const char *suffix) {
  if (n.length() < 5 || n.length() > 63) return false;
  if (n.indexOf('/') >= 0 || n.indexOf('\\') >= 0 || n.indexOf("..") >= 0) return false;
  return n.startsWith(prefix) && n.endsWith(suffix);
}

// cls: CSS-Klasse der Checkboxen dieser Tabelle (eine je Verzeichnis), damit "Alle auswaehlen" nur
// innerhalb dieses Verzeichnisses greift statt ueber alle drei Tabellen hinweg.
static String fileRows(const std::vector<FileInfo> &v, const char *route, bool withDelete, const char *cls = "") {
  // Leeres Verzeichnis: NICHT einfach "" zurueckgeben -- server.sendContent("") erzeugt bei
  // Chunked Transfer Encoding einen 0-Byte-Chunk, und das ist im HTTP-Protokoll das Ende-Signal.
  // Der Browser haelt die Antwort dann fuer fertig und verwirft alles, was danach noch gesendet
  // wird (GPX-Tracks, Logs, Button) -- deshalb baute sich die Seite nach dem Leeren von /shots
  // nicht mehr vollstaendig auf. Stattdessen immer eine (nicht-leere) Platzhalterzeile liefern.
  if (v.empty()) return String("<tr><td colspan=") + (withDelete ? "5" : "3") + ">(keine Dateien)</td></tr>";
  String r;
  for (const FileInfo &fi : v) {
    char dt[24] = "";
    if (fi.t > 100000) {
      struct tm lt;
      localtime_r(&fi.t, &lt);
      strftime(dt, sizeof(dt), "%d.%m.%Y %H:%M", &lt);
    }
    r += "<tr>";
    if (withDelete) r += "<td><input type=checkbox class=\"" + String(cls) + "\" name=del value=\"" + fi.name + "\"></td>";
    r += "<td><a href=\"";
    r += route;
    r += "?n=" + fi.name + "\">" + fi.name + "</a></td><td>" + String((unsigned long)((fi.size + 512) / 1024)) + " kB</td><td>" + dt + "</td>";
    if (withDelete) r += "<td><a href=\"/d?n=" + fi.name + "\" onclick=\"return confirm('Löschen?')\">l&ouml;schen</a></td>";
    r += "</tr>";
  }
  return r;
}

// ---- Bulk-Download eines ganzen Ordners als ZIP (unkomprimiert "stored", per Data Descriptor gestreamt --
// jede Datei wird nur einmal gelesen, die Pruefsumme entsteht dabei nebenbei, nichts wird im RAM gepuffert)
static uint32_t crc32Update(uint32_t crc, const uint8_t *buf, size_t len) {
  static uint32_t table[256];
  static bool ready = false;
  if (!ready) {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t c = i;
      for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
      table[i] = c;
    }
    ready = true;
  }
  crc = ~crc;
  for (size_t i = 0; i < len; i++) crc = table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
  return ~crc;
}

static void zipLE16(NetworkClient &c, uint16_t v) { uint8_t b[2] = {(uint8_t)v, (uint8_t)(v >> 8)}; c.write(b, 2); }
static void zipLE32(NetworkClient &c, uint32_t v) { uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)}; c.write(b, 4); }

static void zipEntriesFromDir(const char *dir, std::vector<FileInfo> &files, std::vector<ZipEntry> &out) {
  for (auto &fi : files) out.push_back({String(dir) + "/" + fi.name, fi.name, fi.size, fi.t});
}

// Wandelt einen Unix-Zeitstempel in die im ZIP-Format verlangten gepackten DOS-Datum/Zeit-Felder um (lokale
// Zeit, wie es die meisten ZIP-Tools erwarten). t == 0 (Datei-Zeitstempel unbekannt, z. B. SD-Datei nie per
// touchFileTime() nachgetragen) oder vor 2020 (eindeutig noch nicht die echte Uhrzeit) faellt auf den alten
// Platzhalter 1980-01-01 00:00 zurueck -- besser als ein falsches "echtes" Datum zu erfinden.
static void dosDateTime(time_t t, uint16_t &dTime, uint16_t &dDate) {
  if (t < 1577836800) { dTime = 0; dDate = 0x21; return; }   // < 2020-01-01: kein verlaesslicher Zeitstempel
  struct tm tmv;
  localtime_r(&t, &tmv);
  dTime = (uint16_t)((tmv.tm_hour << 11) | (tmv.tm_min << 5) | (tmv.tm_sec / 2));
  int y = tmv.tm_year + 1900 - 1980;
  if (y < 0) y = 0;
  dDate = (uint16_t)((y << 9) | ((tmv.tm_mon + 1) << 5) | tmv.tm_mday);
}

// Sendet die uebergebenen Dateien als eine ZIP-Datei. Groesse steht vorher exakt fest (stored, keine
// Kompression), dadurch normaler Content-Length-Download statt Chunked -- funktioniert in jedem Browser.
static void webZipSend(std::vector<ZipEntry> &files, const char *zipName) {
  if (files.empty()) { server.send(404, "text/plain; charset=utf-8", "keine Dateien ausgewählt"); return; }
  uint32_t total = 0;
  for (auto &fi : files) total += 30 + fi.arc.length() + fi.size + 16;          // lokale Header + Daten + Data Descriptor
  uint32_t cdStart = total;
  for (auto &fi : files) total += 46 + fi.arc.length();                        // zentrales Verzeichnis
  total += 22;                                                                  // End of Central Directory

  NetworkClient client = server.client();
  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + zipName + "\"");
  server.setContentLength(total);
  server.send(200, "application/zip", "");

  std::vector<uint32_t> offsets(files.size()), crcs(files.size());
  uint32_t pos = 0;
  uint8_t buf[512];
  for (size_t i = 0; i < files.size(); i++) {
    ZipEntry &fi = files[i];
    offsets[i] = pos;
    zipLE32(client, 0x04034b50);
    zipLE16(client, 20);                    // benoetigte Version
    zipLE16(client, 0x0008);                // Flag: Data Descriptor folgt nach den Dateidaten
    zipLE16(client, 0);                     // Methode: stored (keine Kompression)
    { uint16_t dT, dD; dosDateTime(fi.t, dT, dD); zipLE16(client, dT); zipLE16(client, dD); }   // echtes Datum/Zeit der SD-Datei
    zipLE32(client, 0); zipLE32(client, 0); zipLE32(client, 0);   // CRC/Groessen folgen im Data Descriptor
    zipLE16(client, fi.arc.length());
    zipLE16(client, 0);
    client.write((const uint8_t *)fi.arc.c_str(), fi.arc.length());
    pos += 30 + fi.arc.length();

    File f = SD.open(fi.path);
    uint32_t crc = 0, n = 0;
    while (f && f.available()) {
      int got = f.read(buf, sizeof(buf));
      if (got <= 0) break;
      crc = crc32Update(crc, buf, got);
      client.write(buf, got);
      n += got;
    }
    if (f) f.close();
    crcs[i] = crc;
    pos += n;

    zipLE32(client, 0x08074b50);            // Data-Descriptor-Signatur
    zipLE32(client, crc);
    zipLE32(client, n);
    zipLE32(client, n);
    pos += 16;
  }
  for (size_t i = 0; i < files.size(); i++) {
    ZipEntry &fi = files[i];
    zipLE32(client, 0x02014b50);
    zipLE16(client, 20); zipLE16(client, 20);
    zipLE16(client, 0x0008);
    zipLE16(client, 0);
    { uint16_t dT, dD; dosDateTime(fi.t, dT, dD); zipLE16(client, dT); zipLE16(client, dD); }   // echtes Datum/Zeit der SD-Datei
    zipLE32(client, crcs[i]);
    zipLE32(client, fi.size);
    zipLE32(client, fi.size);
    zipLE16(client, fi.arc.length());
    zipLE16(client, 0); zipLE16(client, 0);
    zipLE16(client, 0);
    zipLE16(client, 0);
    zipLE32(client, 0);
    zipLE32(client, offsets[i]);
    client.write((const uint8_t *)fi.arc.c_str(), fi.arc.length());
  }
  zipLE32(client, 0x06054b50);
  zipLE16(client, 0); zipLE16(client, 0);
  zipLE16(client, files.size()); zipLE16(client, files.size());
  zipLE32(client, total - cdStart - 22);
  zipLE32(client, cdStart);
  zipLE16(client, 0);
  char m[64];
  snprintf(m, sizeof(m), "WLAN: ZIP gesendet: %s (%u Datei(en), %u Byte)", zipName, (unsigned)files.size(), (unsigned)total);
  logLine(m);
}

static void webZipShots() { std::vector<FileInfo> v; listDir(SHOT_DIR, "shot_", ".jpg", v); std::vector<ZipEntry> e; zipEntriesFromDir(SHOT_DIR, v, e); webZipSend(e, "screenshots.zip"); }
static void webZipGpx()   { std::vector<FileInfo> v; listDir(GPX_DIR, "", ".gpx", v);       std::vector<ZipEntry> e; zipEntriesFromDir(GPX_DIR, v, e);   webZipSend(e, "gpx.zip"); }
static void webZipLogs()  { std::vector<FileInfo> v; listDir(LOG_DIR, "rs41_log_", ".txt", v); std::vector<ZipEntry> e; zipEntriesFromDir(LOG_DIR, v, e); webZipSend(e, "logs.zip"); }

// Baut aus den per Checkbox markierten Dateien (ein "del"-Feld je Name, wie bei webDeleteMulti) eine ZIP --
// kann Dateien aus allen drei Ordnern gleichzeitig enthalten. Dateiname: aktuelles Datum_Uhrzeit.zip (steht
// die Uhr noch nicht, Platzhalter mit der Laufzeit in ms).
static void webZipSelected() {
  std::vector<ZipEntry> files;
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) != "del") continue;
    String n = server.arg(i);
    const char *dir = nullptr;
    if (safeName(n, "shot_", ".jpg")) dir = SHOT_DIR;
    else if (safeName(n, "", ".gpx")) dir = GPX_DIR;
    else if (safeName(n, "rs41_log_", ".txt")) dir = LOG_DIR;
    else continue;
    String path = String(dir) + "/" + n;
    File f = SD.open(path);
    if (!f) continue;
    uint32_t sz = f.size();
    time_t ft = f.getLastWrite();
    f.close();
    files.push_back({path, n, sz, ft});
  }
  char zipName[32];
  if (timeSet) {
    time_t t = time(nullptr);
    struct tm tmv;
    localtime_r(&t, &tmv);
    snprintf(zipName, sizeof(zipName), "%04d%02d%02d_%02d%02d%02d.zip", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  } else {
    snprintf(zipName, sizeof(zipName), "auswahl_%lu.zip", (unsigned long)millis());
  }
  webZipSend(files, zipName);
}

static void webIndex() {
  char lm[64];
  snprintf(lm, sizeof(lm), "WLAN: Anfrage GET / (Startseite) von %s", server.client().remoteIP().toString().c_str());
  logLine(lm);
  std::vector<FileInfo> shots, logs, gpx;
  listDir(SHOT_DIR, "shot_", ".jpg", shots);
  listDir(LOG_DIR, "rs41_log_", ".txt", logs);
  listDir(GPX_DIR, "", ".gpx", gpx);
  std::sort(shots.begin(), shots.end(), [](const FileInfo &a, const FileInfo &b) { return a.name > b.name; });
  std::sort(logs.begin(), logs.end(), [](const FileInfo &a, const FileInfo &b) { return a.name < b.name; });
  std::sort(gpx.begin(), gpx.end(), [](const FileInfo &a, const FileInfo &b) { return a.name > b.name; });
  uint64_t totalKB = SD.totalBytes() / 1024ULL;
  uint64_t usedKB = SD.usedBytes() / 1024ULL;
  uint64_t freeKB = totalKB > usedKB ? totalKB - usedKB : 0;
  char spc[64];
  snprintf(spc, sizeof(spc), "Frei: %llu MB von %llu MB", (unsigned long long)(freeKB / 1024ULL), (unsigned long long)(totalKB / 1024ULL));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent("<!doctype html><html><head><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\">"
                     "<link rel=icon href=\"/favicon.ico\"><link rel=apple-touch-icon href=\"/apple-touch-icon.png\">"
                     "<title>T-Deck RS41</title><style>body{font-family:sans-serif;margin:12px auto;max-width:760px;"
                     "background:#000;color:#fff}"
                     "table{border-collapse:collapse;width:100%}a{color:#6cf}"
                     "td{padding:4px 10px;border-bottom:1px solid #444}"
                     "input[type=checkbox]{width:22px;height:22px;vertical-align:middle}"
                     "label{display:inline-block;margin:6px 0 2px}"
                     "button{margin:10px 0;padding:6px 14px}"
                     "@media (max-width:600px){table,tbody,tr,td{display:block;width:100%}"
                     "tr{margin-bottom:10px;border-bottom:2px solid #555}td{border-bottom:none}}"
                     "</style>"
                     // Die folgenden JS-Strings (alert/confirm) und der Seitentitel oben erscheinen NICHT als
                     // HTML-Text, sondern als JS-String bzw. in <title> -- dort wirken HTML-Entities (&uuml; usw.)
                     // nicht, deshalb hier echte UTF-8-Umlaute statt der sonst im Sketch ueblichen ASCII-Schreibweise.
                     "<script>function selAll(el,cls){var cs=document.getElementsByClassName(cls);"
                     "for(var i=0;i<cs.length;i++)cs[i].checked=el.checked;}"
                     "function countSel(){return document.querySelectorAll('input[name=del]:checked').length;}"
                     "function confirmZip(){if(countSel()===0){alert('Keine Datei ausgewählt.');return false;}return true;}"
                     "function confirmDel(){var n=countSel();if(n===0){alert('Keine Datei ausgewählt.');return false;}"
                     "return confirm(n+' Datei(en) löschen?');}</script>"
                     "</head><body>");
  server.sendContent(String("<p>SD-Karte -- ") + spc + "</p>");
  server.sendContent("<p><a href=\"/menu\">Men&uuml;-Fernsteuerung &amp; Live-Bild</a></p>");
  server.sendContent("<form method=POST action=\"/dm\">"
                     "<h2>Screenshots <a href=\"/zf\">(alle als ZIP)</a></h2>"
                     "<label><input type=checkbox onclick=\"selAll(this,'c_shots')\"> Alle ausw&auml;hlen</label><table>");
  server.sendContent(fileRows(shots, "/f", true, "c_shots"));
  server.sendContent("</table><h2>GPX-Tracks <a href=\"/zg\">(alle als ZIP)</a></h2>"
                     "<label><input type=checkbox onclick=\"selAll(this,'c_gpx')\"> Alle ausw&auml;hlen</label><table>");
  server.sendContent(fileRows(gpx, "/g", true, "c_gpx"));
  server.sendContent("</table><h2>Logs <a href=\"/zl\">(alle als ZIP)</a></h2>"
                     "<label><input type=checkbox onclick=\"selAll(this,'c_logs')\"> Alle ausw&auml;hlen</label><table>");
  server.sendContent(fileRows(logs, "/l", true, "c_logs"));
  server.sendContent("</table>"
                     "<button type=submit formaction=\"/zm\" onclick=\"return confirmZip()\">Ausgew&auml;hlte herunterladen (ZIP)</button> "
                     "<button type=submit formaction=\"/dm\" onclick=\"return confirmDel()\">Ausgew&auml;hlte l&ouml;schen</button>"
                     "</form>");
  server.sendContent(String("<p style=\"color:#666;font-size:12px;margin-top:20px\">T-Deck RS41 v") + FW_VERSION + "</p></body></html>");
  server.sendContent("");
}

static void webSend(const String &path, const char *type, const String &name) {
  File f = SD.open(path);
  if (!f) { server.send(404, "text/plain", "nicht gefunden"); return; }
  server.sendHeader("Content-Disposition", "inline; filename=\"" + name + "\"");
  server.streamFile(f, type);
  f.close();
}

static void webShot() {
  String n = server.arg("n");
  if (!safeName(n, "shot_", ".jpg")) { server.send(400, "text/plain; charset=utf-8", "ungültiger Name"); return; }
  webSend(String(SHOT_DIR) + "/" + n, "image/jpeg", n);
}

static void webGpx() {
  String n = server.arg("n");
  if (!safeName(n, "", ".gpx")) { server.send(400, "text/plain; charset=utf-8", "ungültiger Name"); return; }
  webSend(String(GPX_DIR) + "/" + n, "application/gpx+xml", n);
}

static void webLog() {
  String n = server.arg("n");
  if (!safeName(n, "rs41_log_", ".txt")) { server.send(400, "text/plain; charset=utf-8", "ungültiger Name"); return; }
  webSend(String(LOG_DIR) + "/" + n, "text/plain; charset=utf-8", n);
}

// Loescht genau eine Datei (Screenshot, GPX-Track oder Log), sofern der Name zu einem der drei
// erlaubten Muster passt; wird von der Einzel- (webDelete) wie der Mehrfachloeschung (webDeleteMulti) genutzt.
static bool deleteOneFile(const String &n) {
  if (safeName(n, "shot_", ".jpg")) {
    return SD.remove(String(SHOT_DIR) + "/" + n);
  }
  if (safeName(n, "", ".gpx")) {
    // Wird gerade genau diese Datei als offener Track fortgeschrieben? Handle vorher sauber schliessen,
    // sonst legt der naechste Punkt beim erneuten Oeffnen (gpxOpenSegment) automatisch wieder eine frische an.
    String base = n.substring(0, n.length() - 4);   // ohne ".gpx"
    if (base == String(gpxIchId)) gpxCloseTrack(gpxIch);
    if (base == String(gpxSondeId)) gpxCloseTrack(gpxSonde);
    return SD.remove(String(GPX_DIR) + "/" + n);
  }
  if (safeName(n, "rs41_log_", ".txt")) {
    return SD.remove(String(LOG_DIR) + "/" + n);
  }
  return false;
}

// Die ESP32-SD-Bibliothek markiert einen geloeschten Verzeichniseintrag nur als frei, raeumt ihn aber
// nicht sofort auf -- openNextFile() bricht die Auflistung an so einem Eintrag danach vorzeitig ab,
// obwohl (unveraendert!) weitere Dateien folgen. Deshalb nach jeder Loeschung die Karte sauber neu
// einhaengen, damit die Verzeichnis-Auflistung wieder vollstaendig ist. Erst noch offene GPX-Tracks
// sichern/schliessen, sonst werden ihre Handles durch das Aushaengen ungueltig.
static void sdRemountAfterDelete() {
  gpxDrainAll();
  gpxCloseTrack(gpxIch);
  gpxCloseTrack(gpxSonde);
  SD.end();
  delay(50);
  sdReady = SD.begin(PIN_SD_CS, SPI, 4000000, "/sd", SD_MAX_OPEN_FILES);
}

static void webDelete() {
  String n = server.arg("n");
  bool ok = deleteOneFile(n);
  if (ok) {
    char m[104];
    snprintf(m, sizeof(m), "WLAN: Datei geloescht: %s (von %s)", n.c_str(), server.client().remoteIP().toString().c_str());
    logLine(m);
    sdRemountAfterDelete();
  }
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

// Loescht mehrere per Checkbox ausgewaehlte Dateien auf einmal (POST, ein "del"-Feld je angehaktem Namen).
static void webDeleteMulti() {
  int n = 0;
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) != "del") continue;
    if (deleteOneFile(server.arg(i))) n++;
  }
  if (n > 0) {
    char m[72];
    snprintf(m, sizeof(m), "WLAN: %d Datei(en) geloescht (von %s)", n, server.client().remoteIP().toString().c_str());
    logLine(m);
    sdRemountAfterDelete();
  }
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

// ---------- Favicon / Apple-Touch-Icon (kleiner Wetterballon, eingebettet als PNG/ICO) ----------
// Browser (und iOS bei "Zum Home-Bildschirm") fragen diese Pfade automatisch ab; ohne Antwort
// gibt es nur 404-Log-Muell. Bilder mit Pillow erzeugt (Ballon + Sonde, T-Deck-Kartenfarben).
static const uint8_t FAVICON_ICO[] PROGMEM = {
  0x00,0x00,0x01,0x00,0x01,0x00,0x10,0x10,0x00,0x00,0x00,0x00,0x20,0x00,0x94,0x03,
  0x00,0x00,0x16,0x00,0x00,0x00,0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,
  0x00,0x0d,0x49,0x48,0x44,0x52,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x08,0x06,
  0x00,0x00,0x00,0x1f,0xf3,0xff,0x61,0x00,0x00,0x03,0x5b,0x49,0x44,0x41,0x54,0x78,
  0x9c,0x45,0x91,0x5b,0x68,0x1c,0x65,0x1c,0xc5,0xcf,0xff,0x9b,0xc9,0x6e,0x76,0x66,
  0xdb,0x9d,0x6c,0x2e,0x62,0x33,0xd9,0x18,0xa4,0x56,0x44,0x82,0x4a,0x1e,0xaa,0xa0,
  0x60,0x94,0x6a,0x09,0x85,0xb4,0x36,0xad,0x3e,0x4a,0xc1,0x1b,0x86,0x8a,0xda,0x6a,
  0x29,0x82,0x60,0x5b,0xa3,0x08,0x82,0xd8,0x22,0x58,0x51,0x51,0x90,0xf6,0x41,0xbc,
  0x60,0x5a,0xc4,0xaa,0xa4,0x0a,0x25,0x4a,0x6d,0x93,0xb6,0x58,0xc5,0x6e,0x9b,0x90,
  0x4b,0x77,0xb6,0xcd,0x66,0x67,0x36,0x3b,0xd9,0x99,0xf9,0x8e,0x0f,0x8d,0x78,0xe0,
  0xc0,0x79,0xf9,0xc1,0xe1,0x1c,0x01,0x00,0x7b,0xf5,0x9a,0xed,0x54,0xc6,0x1b,0x22,
  0x58,0x2b,0x89,0x0e,0x02,0x33,0xfd,0x01,0x76,0xbf,0x72,0x69,0xe3,0xe0,0x23,0x6d,
  0x62,0x9a,0x1c,0x1d,0x3d,0x71,0x15,0x6f,0x8e,0x74,0xda,0x75,0xff,0x39,0x88,0xb4,
  0x93,0xf8,0x5b,0x74,0xf2,0x5a,0xad,0x3a,0x7b,0x04,0x99,0x5c,0xe7,0x90,0xd5,0xd2,
  0x45,0xbb,0xa5,0x40,0x3b,0x5f,0xf8,0x0b,0x4f,0xbe,0xbc,0x79,0x7f,0x3d,0xfc,0xf4,
  0x4c,0x50,0x0e,0x4b,0x35,0x8f,0x73,0xd5,0xab,0x3c,0xed,0x97,0xab,0xaf,0x2f,0x35,
  0x3e,0xc4,0x9e,0x77,0x1e,0xca,0xb6,0xf7,0xfc,0x66,0x3b,0x05,0x5a,0x2d,0x5d,0xcc,
  0xe4,0x3a,0x87,0x60,0x39,0xee,0xb4,0xe5,0x74,0x31,0xdb,0xda,0x3d,0x8f,0xe1,0x7d,
  0xeb,0xdf,0xf6,0xfd,0xaf,0x7d,0xdf,0xfb,0x87,0xd5,0xd2,0x09,0xff,0xda,0xd4,0x62,
  0xb5,0x7c,0x39,0x09,0xbd,0x22,0xc9,0x80,0x23,0xe5,0xeb,0xc7,0xf0,0xf1,0xf1,0xdb,
  0xb3,0x1d,0xb7,0xfe,0x69,0x3b,0x5d,0xb4,0x1c,0x77,0x0a,0x96,0xe3,0xea,0x6c,0xbe,
  0x40,0xd5,0xdd,0xfb,0xcc,0x40,0xc8,0x17,0x2e,0xcc,0x5d,0xa9,0x90,0xdc,0x45,0x72,
  0x57,0x2d,0xb8,0xfe,0x7d,0xad,0x3a,0xc7,0x92,0x77,0x39,0x0e,0x4a,0xc5,0x86,0xb7,
  0x5c,0x61,0xbf,0x1f,0x3f,0x6f,0xdc,0xd3,0xbf,0x39,0x9b,0x2f,0xd0,0x72,0x5c,0x2a,
  0x00,0x50,0x86,0xe1,0xe9,0xe1,0x3d,0x3f,0x6e,0xf0,0xa6,0xb7,0xae,0x5d,0xd5,0x6c,
  0x07,0x41,0xa5,0x3f,0xac,0x57,0xef,0x8d,0x1a,0x4b,0x77,0xc7,0x51,0x04,0x93,0x50,
  0x75,0x88,0x6a,0x63,0xa4,0x37,0x95,0xa7,0x9f,0x48,0x3e,0xfa,0x76,0x5c,0xa5,0x52,
  0x17,0x41,0xc0,0x14,0x42,0x98,0x4e,0x4d,0xe0,0xa5,0xc7,0x1b,0x37,0x4d,0x15,0x0b,
  0x26,0x9a,0xcc,0x24,0xae,0x3d,0x8a,0x44,0x20,0x20,0x12,0xad,0x81,0x1b,0x51,0x74,
  0x14,0xa9,0x9b,0x45,0x17,0x70,0x97,0x65,0x20,0x6b,0x8f,0x4b,0x3d,0x5c,0x67,0x02,
  0x80,0x34,0xa5,0x7c,0x00,0x51,0x4c,0x34,0x40,0x32,0x8e,0x93,0x24,0xa1,0x36,0x00,
  0x40,0x44,0x04,0x2b,0x12,0x08,0x22,0xc8,0x32,0x80,0x44,0x52,0x69,0x1f,0x00,0x14,
  0x00,0x50,0x27,0xed,0x00,0xea,0x13,0x31,0xcf,0x33,0xdd,0x2c,0xa0,0x86,0xac,0xe8,
  0x7f,0x98,0x1a,0xe9,0x34,0x26,0x63,0x9e,0x07,0x10,0xe8,0x38,0xee,0x00,0x00,0x45,
  0x01,0x92,0x5a,0xed,0x4e,0xfb,0xc0,0x21,0xf7,0xb0,0xaf,0x3e,0x39,0x55,0x0d,0xd9,
  0x6a,0x1a,0x2a,0xd2,0x3a,0x21,0x00,0x02,0x88,0xb4,0x4e,0x5a,0x4d,0x43,0x9d,0xf2,
  0x43,0x1e,0x2e,0x45,0x9f,0xa7,0xc7,0xce,0xe6,0x93,0xca,0x62,0x1f,0x01,0xc0,0x72,
  0xdc,0xd8,0xce,0xb9,0xb4,0xfb,0x1e,0x7c,0x0b,0xe4,0xaa,0x8d,0xbf,0x4f,0x8e,0x9c,
  0x0b,0xca,0x64,0xe8,0xb1,0xea,0x15,0xb9,0xe8,0x15,0xc9,0xd0,0xe3,0xb9,0xa0,0xcc,
  0x81,0xd3,0x17,0xf6,0x81,0xcc,0xd9,0x0f,0x6f,0xd9,0x69,0xe5,0x5c,0x5a,0x8e,0x9b,
  0xc0,0x72,0x5c,0x6d,0x39,0x6e,0x92,0x71,0xba,0xfc,0xcc,0x8e,0x9d,0x5b,0x40,0x3a,
  0xb7,0xfd,0x72,0xe6,0xd9,0x43,0xf3,0xf3,0x3f,0x4f,0x07,0x95,0xea,0x6c,0xb0,0x50,
  0x3d,0x38,0x5f,0xfa,0x69,0xdd,0xc9,0x3f,0x9e,0xc6,0x1c,0x7b,0xd2,0x7b,0x0f,0x6c,
  0xc8,0xe4,0xbb,0x67,0xff,0xe3,0xc4,0x72,0x5c,0x0d,0x00,0x20,0x84,0x86,0x2a,0x1b,
  0xf7,0x3f,0xf0,0x6a,0xf0,0xd5,0x67,0x5f,0xe2,0x87,0xf1,0xf6,0xd1,0xe5,0xc9,0xf7,
  0xd3,0x26,0x52,0x5b,0xeb,0xad,0xc3,0x0b,0x83,0x83,0x33,0xe9,0x6f,0xc6,0x5e,0xc4,
  0xc1,0xf7,0x9e,0x32,0x4f,0x8e,0xb5,0xb3,0xb9,0x99,0x20,0x45,0x2c,0xc7,0xe5,0xca,
  0x4e,0x04,0x20,0x0a,0x40,0x2d,0x9b,0xff,0x6e,0x53,0xaf,0x31,0xf7,0xc5,0x7d,0x97,
  0x76,0xa4,0x9a,0x95,0xec,0xfe,0xb5,0xe3,0xe8,0xbb,0x57,0x7a,0x8c,0xcc,0xfa,0xde,
  0xc7,0x74,0xc9,0x83,0x71,0xfc,0x18,0xd1,0xd4,0x24,0x20,0x61,0x02,0x9c,0x01,0x54,
  0x27,0x40,0x0d,0x40,0x41,0x20,0x46,0x65,0x61,0xe0,0xe2,0xd9,0x10,0xd3,0x7d,0x0e,
  0x32,0x4d,0x29,0x8c,0x4f,0x78,0xdb,0xd4,0xb5,0x18,0xb8,0xe3,0x16,0xa0,0x52,0x21,
  0x92,0x44,0x23,0x95,0x32,0x40,0x3d,0x23,0x99,0x5c,0xe7,0x90,0x28,0x75,0xf4,0x46,
  0x07,0x72,0xe5,0xfa,0x24,0x8a,0x22,0x69,0xcb,0x3b,0xca,0xb6,0x6d,0x14,0xa7,0x66,
  0x74,0xda,0x50,0xd4,0x4e,0x8b,0x21,0xf5,0x25,0xa2,0xd1,0x10,0x28,0x05,0x6a,0xbd,
  0x4d,0x00,0xa0,0x79,0xf5,0x9a,0xed,0x4a,0xa9,0xfd,0x00,0x7a,0x00,0xc8,0x8a,0xa1,
  0x09,0x80,0x84,0x52,0x02,0x88,0x00,0x71,0x4c,0x28,0x45,0x28,0x29,0xea,0x44,0xef,
  0x0d,0xab,0xb3,0x47,0xfe,0x05,0x5d,0x7b,0xba,0xda,0x50,0xb1,0x0e,0x00,0x00,0x00,
  0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82,
};
static const size_t FAVICON_ICO_LEN = 938;

static const uint8_t APPLE_ICON_PNG[] PROGMEM = {
  0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
  0x00,0x00,0x00,0xb4,0x00,0x00,0x00,0xb4,0x08,0x03,0x00,0x00,0x00,0x0a,0x13,0xf6,
  0x00,0x00,0x00,0x00,0x60,0x50,0x4c,0x54,0x45,0x0a,0x10,0x1c,0xee,0xeb,0xe0,0x00,
  0xb5,0xc6,0x00,0x00,0x01,0x04,0xe0,0xef,0xfd,0xfe,0xfd,0x00,0xa4,0xb4,0xfe,0x91,
  0x1e,0x00,0xea,0xf4,0x08,0x1e,0x2a,0x01,0xcc,0xdb,0x03,0xd8,0xe7,0xa1,0xe5,0xe3,
  0x06,0x55,0x62,0x6a,0x35,0x03,0x5d,0xe0,0xe5,0x8d,0x48,0x03,0x09,0x0f,0x1b,0x03,
  0x98,0xa6,0x07,0x39,0x46,0x06,0x54,0x61,0xd2,0x75,0x14,0xa5,0xe2,0xe7,0x6e,0x41,
  0x0b,0x05,0x77,0x85,0x02,0xb7,0xc6,0x01,0x7d,0x94,0x03,0x76,0x84,0x5f,0x5a,0x2d,
  0x9a,0x9c,0x9f,0x66,0x4f,0x1d,0x5a,0xd5,0xe2,0xcf,0xd9,0x66,0x7c,0x00,0x00,0x00,
  0x20,0x74,0x52,0x4e,0x53,0xfe,0xfe,0x2d,0x02,0xf8,0x8d,0x29,0xfe,0x23,0x9e,0x5c,
  0x9f,0xfd,0x57,0xfc,0xfc,0xfe,0x5b,0xff,0x75,0xfe,0xfd,0x5e,0xc9,0xfe,0xfe,0x27,
  0x42,0x79,0xff,0x92,0x38,0x68,0x74,0xa1,0xf7,0x00,0x00,0x08,0xb5,0x49,0x44,0x41,
  0x54,0x78,0xda,0xed,0x9d,0x7d,0x77,0xea,0x28,0x10,0xc6,0x49,0x1a,0xb9,0x21,0x31,
  0xc6,0xa8,0xed,0xb6,0xf6,0xee,0xfa,0xfd,0xbf,0xe5,0x62,0x24,0xd1,0xc0,0x00,0x33,
  0x04,0xc2,0xed,0xd9,0x9d,0xbf,0x3c,0xa7,0x56,0x7f,0x7d,0xfa,0xcc,0xf0,0x22,0x0e,
  0xec,0xcd,0x13,0x83,0x10,0x0c,0x19,0x42,0x74,0x1d,0x2f,0x81,0xe0,0x5d,0x47,0x79,
  0x95,0xc1,0xc7,0xc4,0xdc,0xc0,0x8c,0x00,0xcc,0x6b,0x19,0x25,0x18,0xf7,0x9f,0xf0,
  0xee,0x4c,0x78,0xb9,0x21,0x0c,0x7a,0x60,0xf8,0xb7,0x50,0xc0,0x9c,0x97,0x96,0xe0,
  0xf2,0x47,0xf7,0x67,0x74,0x78,0x6e,0x36,0x90,0xa1,0x09,0xa2,0x74,0x0f,0x5e,0x2b,
  0xb0,0x06,0x4e,0xe0,0x16,0x24,0x68,0xbc,0x01,0xbb,0xd2,0xa9,0x30,0x00,0x4e,0xe2,
  0x16,0x68,0x68,0x41,0x11,0x99,0x40,0x3c,0x81,0xcb,0xbf,0xf3,0xba,0x0a,0x9b,0x85,
  0x7b,0x59,0x48,0xd1,0x00,0x62,0x3e,0x86,0xf9,0x78,0xf1,0x14,0xf9,0x9b,0xe7,0x70,
  0x6f,0xb3,0x50,0x99,0x25,0x72,0xc9,0xa1,0xff,0x7e,0xd3,0xb4,0x6d,0x5b,0xa9,0x90,
  0x0f,0x9b,0x06,0x72,0xbc,0x94,0xbb,0x3c,0x87,0x8a,0xad,0x43,0xaf,0x40,0xbe,0x03,
  0xcf,0xb4,0xcb,0x90,0xe4,0x06,0xf7,0x1d,0x1b,0xab,0x90,0x1b,0x1a,0xed,0x65,0x1d,
  0x99,0xf3,0x27,0x70,0xdf,0xef,0xe7,0xe8,0xfb,0x27,0x38,0x80,0xcd,0x45,0x08,0x35,
  0x0b,0xb0,0xb3,0x4c,0x3f,0xe3,0xed,0x1b,0xc5,0xbb,0xdf,0x17,0x66,0xec,0x15,0x79,
  0xab,0xe7,0x80,0xf4,0x76,0x17,0x60,0x6c,0x46,0x66,0x96,0xce,0x58,0xbe,0xf5,0x2c,
  0x32,0xc4,0xfb,0x04,0x87,0xe5,0xe6,0x58,0x8f,0x0c,0x30,0xf4,0x80,0x94,0x59,0x73,
  0x86,0x42,0xee,0x0b,0x6f,0xec,0x21,0x6c,0xe9,0x91,0x8e,0x4a,0xcd,0x68,0x7e,0xbe,
  0xcb,0xac,0x19,0xa3,0xf5,0x8a,0xac,0xf9,0x44,0x62,0x07,0x89,0x0d,0x41,0xa3,0x98,
  0x35,0x37,0x73,0xde,0x8e,0x46,0x2e,0xd0,0xf1,0xc0,0x2e,0x75,0xb1,0xcf,0x24,0x6a,
  0x46,0x61,0xee,0x74,0x37,0x37,0x04,0x95,0x17,0x26,0x31,0xc4,0xee,0x28,0xd4,0x8c,
  0x30,0xa6,0x74,0x90,0xcc,0x44,0xe4,0x09,0x5b,0x17,0xbb,0xe6,0x84,0x51,0x86,0xe1,
  0x93,0x50,0xb3,0x33,0xaf,0x71,0xe9,0x07,0x45,0x6f,0x88,0x2d,0x8b,0x9f,0x40,0x27,
  0x23,0xc3,0x9a,0xc3,0xb0,0x73,0x13,0x26,0xf3,0x53,0xec,0x46,0x37,0xb6,0xc0,0x1a,
  0x84,0x21,0xcd,0x21,0xb4,0x4a,0x37,0x5a,0xa3,0x58,0x11,0x77,0x8b,0x68,0x43,0x14,
  0x82,0x5a,0xbc,0x40,0x07,0x31,0xf7,0xc5,0xaa,0xe8,0x0d,0x6a,0x8e,0xa0,0x7e,0x42,
  0x0b,0x3f,0xb3,0xf6,0xf2,0xeb,0x99,0x47,0x8b,0xb4,0x25,0x95,0x5a,0xcc,0xd0,0x08,
  0x9d,0x97,0xd1,0xae,0xb0,0xf3,0xd2,0xd8,0x64,0xea,0x09,0xda,0xfb,0x44,0xc0,0x1b,
  0x11,0x98,0x21,0x6a,0xbf,0xaf,0x85,0x82,0xa6,0xd6,0xba,0x58,0xcc,0x0f,0x87,0xe8,
  0xd9,0x88,0x90,0x9a,0xf9,0x6b,0xf4,0x77,0x32,0x66,0x80,0xda,0x3f,0xca,0x0c,0x23,
  0x34,0x71,0x1c,0x6c,0x22,0x32,0x83,0xd4,0x9d,0x57,0x6a,0x2f,0xb4,0x30,0x99,0xfb,
  0xa2,0x88,0x4a,0xad,0x8f,0x8d,0x67,0x3f,0xf4,0x40,0x2a,0x76,0xa5,0xc1,0xbc,0x9b,
  0x22,0xbc,0x5e,0xeb,0xd4,0xc2,0xe3,0x0f,0xe6,0xa9,0x1d,0xfa,0xba,0xaa,0x5d,0x8e,
  0x83,0xbb,0x65,0x84,0x8e,0x8d,0x25,0x25,0x19,0x85,0x84,0x5e,0x61,0xe8,0x9d,0x19,
  0x61,0xd4,0x34,0x5b,0x7b,0xa0,0x75,0x73,0x2c,0x0c,0xbd,0x83,0x23,0x8a,0xad,0x85,
  0x1b,0xda,0x69,0x69,0x7d,0x67,0xe3,0x75,0x92,0xb4,0xdb,0x45,0xa3,0xee,0xcd,0x91,
  0xd1,0x69,0x6a,0x26,0x68,0xe6,0xf0,0x33,0x87,0x50,0xd3,0x0c,0x22,0x5c,0xd0,0xae,
  0xca,0xb1,0xdb,0xc5,0xa4,0xa6,0x19,0x44,0xb8,0x87,0x6f,0x7b,0xe5,0x88,0x0b,0x5d,
  0x98,0xc3,0x79,0xc9,0x82,0x42,0x38,0x2a,0xc7,0x6e,0x97,0x5c,0xea,0x73,0x10,0x74,
  0x59,0x06,0x0a,0x1d,0x96,0x8b,0xc6,0x8a,0x20,0x84,0xf9,0xec,0x2a,0xd1,0xd1,0xa1,
  0x01,0xa9,0xbb,0x6d,0x85,0xce,0x26,0xb5,0x26,0x74,0x59,0x13,0x84,0xce,0x26,0xb5,
  0x2e,0xf4,0x6b,0x8d,0x4e,0x01,0x5d,0x54,0xeb,0xa5,0x76,0xcf,0xee,0x52,0x40,0xef,
  0xcd,0x61,0x91,0x5a,0x40,0xf4,0x1a,0xdd,0x38,0x67,0x77,0xbb,0x38,0xb3,0xbd,0x66,
  0x5d,0xad,0x06,0xf6,0x0c,0xfa,0xd4,0xd0,0x7d,0xc8,0x3e,0x88,0x63,0xd6,0x71,0x77,
  0xc7,0xbe,0x40,0x53,0x07,0xaf,0x61,0xb4,0xad,0x8a,0xfa,0x9b,0x04,0xad,0x0b,0xdd,
  0xe8,0x7b,0x60,0x09,0xa0,0x0b,0x60,0x07,0x58,0x44,0x74,0x47,0xec,0x59,0x9e,0xd5,
  0x1f,0x94,0x54,0xfc,0xd6,0xb7,0x94,0x80,0x15,0x78,0x74,0x66,0xb3,0x7e,0x48,0xec,
  0x98,0xee,0x88,0xba,0x72,0x89,0xe3,0x0f,0x81,0x82,0x2e,0x62,0x33,0x43,0xe3,0x4b,
  0x17,0x5c,0x3b,0x00,0x4b,0x5b,0xb0,0xd7,0xee,0xfe,0x1a,0x9b,0x64,0x78,0x7f,0x18,
  0x87,0x7a,0x1c,0x9b,0x4a,0xd1,0x88,0x41,0x53,0xe3,0xfd,0x61,0x0c,0xe1,0x65,0xd4,
  0x9d,0x30,0xb7,0xa9,0x43,0xa1,0x3b,0x9c,0xa5,0xd3,0x40,0x87,0x9a,0x5a,0x2f,0x78,
  0xdb,0x41,0x9b,0xa6,0x46,0x17,0xbd,0xda,0x9c,0xff,0xf7,0xb9,0xa0,0xb1,0xfe,0x30,
  0x2c,0xbd,0x1d,0xf4,0x3e,0x2a,0xf4,0x7e,0x2b,0x68,0xb3,0x7c,0x74,0x61,0x33,0xbc,
  0xcd,0x8a,0xc7,0x0a,0xe8,0xef,0x7c,0xd0,0x85,0x31,0x3b,0xc5,0x66,0xa2,0x79,0x24,
  0x2d,0x27,0x34,0x72,0xa5,0x68,0x08,0x5d,0x6f,0x55,0xf1,0xa0,0x25,0x17,0x2e,0x13,
  0xc5,0xff,0xd0,0xff,0x55,0xe8,0x73,0xc0,0xd6,0x52,0x76,0xe8,0x2e,0xa8,0x4c,0x6f,
  0x5a,0x3d,0x9a,0xf2,0x07,0x42,0xd7,0x3f,0x0e,0x1a,0x58,0xda,0x96,0xf5,0x35,0x08,
  0x9a,0xe7,0x85,0xe6,0x61,0xd0,0x1b,0xce,0xf2,0x7e,0x20,0x34,0xb0,0x08,0x08,0x86,
  0xce,0xb9,0x72,0xf9,0xf3,0xa1,0x81,0xdd,0xde,0x1f,0x09,0x1d,0x5c,0xf2,0x36,0xdc,
  0x42,0x30,0xa7,0xd3,0x28,0x68,0x73,0x18,0xdf,0x2c,0x13,0xa1,0xe2,0x81,0x83,0x16,
  0xf9,0xa0,0xa1,0x3c,0x0c,0x87,0xde,0xc8,0xd4,0x50,0x1e,0xe2,0x66,0x79,0x00,0xf4,
  0x56,0xa6,0x86,0xa1,0x45,0xd0,0x72,0x6b,0x2b,0x7f,0x40,0x96,0xc6,0x42,0x43,0xdf,
  0x11,0xdb,0xc4,0x1f,0x90,0xa5,0xb1,0x0b,0xdb,0x2b,0xf0,0xf5,0xcd,0x4d,0xa0,0x21,
  0x77,0x60,0xb7,0x10,0xa0,0x42,0xcd,0xab,0xf4,0xfe,0xb0,0xb8,0x03,0xf7,0xb1,0xdc,
  0x19,0x36,0x75,0x95,0xc9,0x1d,0x5d,0xd8,0x5e,0x9e,0x71,0x08,0x61,0x43,0x77,0xa0,
  0x77,0xd5,0x21,0xe8,0xf4,0xf5,0xa3,0xaf,0x2a,0x5e,0x86,0x43,0xf3,0x3a,0x47,0xfd,
  0x80,0xd3,0x10,0xfd,0xa1,0x3e,0x94,0x89,0xc0,0xc9,0xd8,0x0d,0xd2,0x90,0xf0,0xf1,
  0x05,0x68,0xea,0xd4,0x52,0x57,0x50,0x1a,0x52,0x3e,0x68,0x06,0x95,0x4e,0x2b,0xf5,
  0x1e,0x98,0x95,0xd2,0x3e,0xb2,0xfd,0xae,0x37,0x1f,0x15,0x61,0xa1,0x29,0x1f,0x8e,
  0x9f,0x37,0x97,0xda,0x2a,0x34,0xfe,0x63,0x66,0xd0,0xd4,0x49,0xa5,0xb6,0x08,0x4d,
  0x3a,0xf0,0xc1,0xc1,0xee,0x11,0xbc,0x4a,0x45,0xdd,0x83,0xa5,0x83,0x78,0x8a,0xe9,
  0x6c,0x95,0x7a,0x9f,0xc8,0x1c,0x0d,0x5f,0xe9,0x0e,0x9b,0x3f,0x92,0xcd,0x40,0xac,
  0x59,0x48,0x3b,0x79,0xd5,0x59,0xba,0x8b,0x24,0x31,0x88,0xdd,0x1c,0x94,0x83,0x35,
  0x76,0xa9,0x9b,0x04,0x15,0xc4,0x66,0x0e,0xfa,0x69,0x42,0xdb,0x9f,0xde,0xc6,0xb7,
  0xb5,0xcd,0x1c,0xf4,0x73,0x9b,0x96,0x52,0x9d,0xc0,0xd6,0x15,0x38,0xbb,0x0b,0x3b,
  0x21,0x6b,0x83,0x8e,0x6d,0x6b,0x69,0x68,0xdb,0x5b,0x11,0x0f,0x40,0x5a,0xa7,0x7a,
  0xea,0x0b,0xc1,0x51,0x99,0x9b,0x68,0x42,0x5b,0xa5,0x8e,0x9b,0x8c,0x76,0xe6,0x10,
  0xa1,0xed,0x52,0xc7,0xa4,0x76,0x32,0x87,0x1c,0xaf,0xb7,0x4a,0x1d,0x8f,0xba,0xb7,
  0x16,0x8e,0x40,0xa1,0xed,0x05,0x24,0x1a,0xb5,0x9b,0x39,0x48,0x68,0x6b,0xad,0x8e,
  0x95,0x8d,0x0e,0x6f,0x84,0x7e,0x63,0xc4,0x3e,0x2c,0xae,0x6f,0x2a,0x30,0xd5,0x67,
  0x17,0xf3,0x39,0x10,0x1a,0x38,0x63,0xb3,0xa4,0xee,0x57,0x8d,0xdd,0x0e,0x66,0xea,
  0xac,0x03,0x97,0x8b,0xaa,0x85,0x4a,0xbf,0xc6,0xce,0xa5,0xe3,0xc5,0xc3,0xb2,0xd0,
  0x97,0x8b,0xab,0x2c,0xb2,0xaf,0x1c,0x29,0xb8,0x26,0x0b,0xbd,0x06,0x09,0x6c,0xa4,
  0x52,0x40,0xdd,0x3d,0x22,0x9a,0xc3,0x63,0x90,0xa9,0x61,0x46,0x4f,0x47,0x76,0x59,
  0x63,0xa5,0x39,0x3c,0x15,0xe4,0xd9,0xcf,0xa8,0x27,0x22,0x7b,0x64,0x0e,0xaf,0x1c,
  0xde,0xd1,0x7c,0xee,0x1c,0x55,0xa1,0x1b,0x04,0x3d,0x3a,0x5f,0x35,0xdc,0xc3,0xdc,
  0xb1,0xd5,0xc1,0xdd,0xd4,0x73,0xc3,0x2b,0x2f,0xf7,0xa3,0xe7,0x95,0x07,0x59,0xfa,
  0x86,0xb3,0x08,0x51,0xfb,0xdb,0x0d,0xaa,0x3e,0x5d,0x76,0x6e,0xd5,0x5d,0xac,0x6d,
  0xbc,0x2d,0x01,0x57,0x1b,0x1a,0x65,0xeb,0xa9,0xeb,0xdc,0xd4,0xc2,0x6d,0x6f,0x01,
  0x06,0xfa,0xa1,0xa5,0x63,0x46,0x51,0x8f,0x72,0x5b,0xfa,0xa2,0x3d,0x3b,0xe6,0xd9,
  0x7e,0x33,0x01,0x33,0x92,0x7a,0x6c,0x3f,0x38,0x82,0x1b,0xeb,0xd6,0xb6,0xb1,0x77,
  0xe3,0x94,0x89,0xfc,0x2c,0x26,0x11,0x0a,0x07,0x95,0x5a,0x35,0x7d,0x2c,0xd9,0x2b,
  0xf3,0xae,0xf4,0xb4,0x0f,0x7d,0x19,0x69,0xa2,0x14,0x0e,0xe4,0x78,0x6e,0x90,0xbf,
  0x7e,0x79,0xc4,0xfb,0x7b,0xed,0x3c,0xa6,0x47,0x66,0xa6,0x51,0xf3,0xf2,0xc9,0xcc,
  0xfc,0xa9,0xd7,0x28,0xea,0xe8,0xcc,0x04,0x87,0x94,0xfc,0x76,0xfb,0x98,0x98,0x3f,
  0x78,0x8d,0xa3,0x96,0x69,0x1a,0xd3,0xcf,0x4f,0xea,0x92,0xe3,0x98,0x8b,0xa2,0x54,
  0x06,0xd9,0xf1,0xa2,0xb8,0xb9,0xc7,0xbf,0x99,0x3a,0x5e,0xdd,0x58,0x52,0x23,0xc4,
  0xae,0xf9,0xc7,0x98,0x7c,0x0f,0xe8,0xdb,0xdd,0xde,0x1f,0xae,0x95,0x44,0x33,0x53,
  0x27,0x61,0x46,0x8c,0xe8,0xf7,0xf7,0xbf,0x33,0x5f,0xcb,0x47,0x05,0x91,0xb8,0x57,
  0x27,0xf5,0x54,0x39,0xae,0x32,0x1b,0x2f,0x89,0xa0,0x8d,0xd6,0x84,0x20,0xf3,0xc7,
  0x75,0x82,0x1f,0x79,0x98,0x83,0xba,0x7d,0x50,0xcb,0x14,0xfc,0xaa,0xaa,0x54,0xd0,
  0x3e,0x8b,0x28,0xe6,0xb1,0x43,0xaf,0xac,0x76,0x63,0xd5,0xbe,0x7e,0xd8,0xa9,0x95,
  0x9b,0x65,0x0a,0x5e,0xaa,0xea,0x33,0x1d,0x35,0x77,0x88,0x3d,0x32,0x8f,0x6d,0xbd,
  0x65,0xdc,0x6e,0xea,0x41,0x3d,0x89,0x6e,0xa3,0x1e,0xdb,0xc8,0x9e,0x12,0x4a,0x7d,
  0xaf,0xd8,0xf6,0x4d,0x9c,0x5b,0xc1,0xd8,0x5f,0x2a,0x0e,0x87,0xe9,0x11,0x63,0xb0,
  0xd4,0x5c,0xad,0x7d,0x46,0x89,0x2f,0x09,0x5d,0x7d,0x17,0xfb,0xdb,0x52,0xfc,0x78,
  0xfd,0xfb,0x70,0x38,0x02,0x71,0x38,0xfc,0x53,0x03,0x2b,0xb5,0xd1,0xcd,0xac,0x52,
  0x12,0x7f,0x55,0x27,0x96,0x32,0x04,0xdc,0x78,0x5a,0x32,0x1f,0xdf,0x7f,0x01,0xf1,
  0x7e,0x94,0xd4,0x1c,0x74,0x73,0x37,0xbb,0xf9,0x33,0xa9,0x3f,0x1e,0x65,0x04,0xc0,
  0xe6,0x07,0xf6,0xcb,0x12,0xc7,0x03,0x07,0x1a,0x05,0x56,0xe3,0xb8,0x7d,0xaa,0xbe,
  0xc6,0xd7,0x4c,0xeb,0x8f,0x51,0x6c,0x03,0x5b,0xfe,0xa7,0x0f,0x47,0x1b,0xf4,0xfb,
  0xe1,0xb7,0xde,0x14,0xb5,0x9e,0x6a,0xf3,0xe4,0xe6,0x2a,0xb1,0x3f,0x26,0xec,0x92,
  0x2f,0xdd,0xf1,0x6e,0x87,0x5e,0xf8,0xe3,0xde,0x56,0xbd,0x13,0x53,0x6d,0x56,0xfe,
  0x38,0x6d,0x00,0xad,0x9a,0xed,0xcf,0x72,0xa3,0xa1,0xf9,0x03,0xf9,0x29,0xb1,0x4a,
  0xc1,0x53,0x72,0x53,0x4f,0xf5,0x8f,0xcf,0xfd,0xeb,0x71,0xd0,0x23,0xf1,0xd4,0x04,
  0x7e,0x96,0xf8,0x6b,0x9b,0x4c,0x7c,0x91,0xfb,0xc1,0xcd,0xfd,0xd0,0x8f,0xdb,0x0d,
  0x5e,0x6e,0x65,0x50,0x12,0x2b,0xda,0xcb,0x76,0xd0,0x13,0x77,0xed,0x87,0x1e,0x2f,
  0xc0,0x78,0x9d,0xcc,0x9d,0x32,0x42,0x33,0x75,0x5f,0x87,0x0f,0xda,0xb8,0xb1,0x23,
  0x33,0xf4,0x08,0xce,0x5c,0xd0,0x07,0x60,0xbe,0x5c,0x4d,0x19,0xf8,0x95,0x0d,0x9a,
  0xb9,0xa1,0xcd,0xa7,0x4f,0xd5,0xe3,0x44,0xae,0x1e,0x22,0x1f,0xf4,0x69,0xae,0xd3,
  0xaf,0x82,0x63,0x90,0xf3,0x41,0x5f,0x14,0xac,0x56,0xae,0x31,0xd0,0x43,0x2e,0xe8,
  0xcb,0x34,0xbd,0x7b,0x0a,0x8e,0x9c,0x7b,0x0c,0xec,0x2d,0x17,0x74,0x35,0x41,0x3e,
  0x05,0x47,0xbe,0xcd,0x5b,0x3e,0xe8,0x93,0x62,0x3e,0xcd,0x82,0x7f,0xe1,0xa1,0x45,
  0x36,0x4f,0x2f,0x9c,0xcd,0xd0,0x8b,0x44,0xe1,0x6d,0x7b,0x4b,0x83,0x76,0x4c,0x4d,
  0x6d,0xbf,0xa3,0x3b,0x1b,0x61,0x69,0x44,0x57,0xe4,0x28,0xd0,0x47,0x1b,0xf4,0x97,
  0xe6,0x6c,0x8c,0x3b,0xe2,0x42,0x33,0xcb,0x72,0x4b,0x32,0x1f,0xac,0xcc,0x27,0xa2,
  0xd0,0x0c,0xd5,0x34,0x9b,0x02,0x6d,0x5b,0xd8,0x1e,0xac,0xde,0x38,0x2d,0x9d,0x8d,
  0x71,0x07,0xaa,0x3d,0x39,0x11,0x5b,0xc5,0xf1,0xe5,0xb1,0xed,0xd9,0x9f,0x53,0xf2,
  0x55,0x14,0xa1,0x71,0x8d,0xe0,0x83,0xe2,0xef,0x02,0xf1,0xa4,0x8b,0xe6,0x6c,0x4c,
  0xed,0x40,0xb6,0xdc,0x4f,0x07,0x3d,0x4f,0x42,0x3e,0x09,0x42,0x23,0x2f,0x37,0x48,
  0x0b,0x7d,0x22,0xac,0xc3,0x05,0xe1,0x1a,0x89,0x94,0xd0,0x15,0x65,0xef,0x80,0x74,
  0x61,0x47,0x3a,0x68,0x12,0xb3,0xa0,0x5d,0x8d,0x92,0x0c,0xfa,0x42,0xd9,0x58,0x22,
  0x5f,0x42,0x93,0x2e,0x11,0xd1,0x31,0xd0,0xaf,0xfb,0xc9,0x0e,0x2d,0x42,0x2e,0x56,
  0x22,0x52,0x47,0x7f,0xc5,0xb0,0x2b,0xac,0xf2,0x46,0xe8,0x65,0x61,0x7f,0x04,0x33,
  0xfd,0x5a,0xb6,0x6c,0x31,0xac,0xb9,0x00,0x2f,0x3f,0x73,0xd0,0x55,0x83,0xb9,0x99,
  0xc3,0x2e,0x75,0xcc,0xe9,0xe7,0x15,0xd7,0x67,0x66,0x65,0x0e,0xbf,0xa8,0x74,0xc3,
  0x10,0x11,0xaf,0x84,0xcd,0x62,0xe7,0xf5,0x97,0xef,0x66,0x91,0x79,0xfd,0x35,0xc7,
  0x39,0x90,0x63,0x5c,0x28,0xbd,0x39,0x72,0xa4,0xab,0xbb,0xb7,0xf2,0x72,0xec,0x4b,
  0xd2,0x63,0x6b,0x3c,0x04,0xdf,0xec,0x4e,0xbc,0x8e,0x3e,0x12,0xaf,0xff,0x3a,0xfa,
  0x7f,0x01,0x96,0x5d,0xd4,0x1e,0xb1,0x76,0x12,0xff,0x00,0x00,0x00,0x00,0x49,0x45,
  0x4e,0x44,0xae,0x42,0x60,0x82,
};
static const size_t APPLE_ICON_PNG_LEN = 2438;


static void webFavicon()   { server.send_P(200, "image/x-icon", (const char *)FAVICON_ICO, FAVICON_ICO_LEN); }
static void webAppleIcon() { server.send_P(200, "image/png", (const char *)APPLE_ICON_PNG, APPLE_ICON_PNG_LEN); }

static void webNotFound() {
  char m[104];
  snprintf(m, sizeof(m), "WLAN: Anfrage %s von %s (unbekannter Pfad -> 404)", server.uri().c_str(),
           server.client().remoteIP().toString().c_str());
  logLine(m);
  server.send(404, "text/plain", "nicht gefunden");
}

// Vorwaertsdeklaration: die eigentlichen Funktionen stehen erst weit unten bei den Menuefunktionen
// (menuValue()/menuStep()/menuApplyTyped()/editBuf, die sie benutzen, sind dort erst definiert) --
// ohne diese Deklarationen hier waeren sie an dieser Stelle (server.on()-Registrierung) noch unbekannt.
static void webMenu();
static void webMenuStep();
static void webMenuSet();
static void webMenuSelect();
static void webLive();
static void webViewSpace();
static void webViewFocus();
static void webMenuStat();
static void webAltStat();

static void wifiServerStart() {
  static bool routes = false;
  if (!routes) {
    routes = true;
    server.on("/", webIndex);
    server.on("/f", webShot);
    server.on("/g", webGpx);
    server.on("/l", webLog);
    server.on("/d", webDelete);
    server.on("/dm", HTTP_POST, webDeleteMulti);
    server.on("/zm", HTTP_POST, webZipSelected);
    server.on("/zf", webZipShots);
    server.on("/zg", webZipGpx);
    server.on("/zl", webZipLogs);
    server.on("/favicon.ico", webFavicon);
    server.on("/apple-touch-icon.png", webAppleIcon);
    server.on("/apple-touch-icon-precomposed.png", webAppleIcon);
    server.on("/menu", webMenu);
    server.on("/mstep", webMenuStep);
    server.on("/mset", webMenuSet);
    server.on("/mselect", webMenuSelect);
    server.on("/live.jpg", webLive);
    server.on("/vspace", webViewSpace);
    server.on("/vfocus", webViewFocus);
    server.on("/mstat", webMenuStat);
    server.on("/malt", webAltStat);
    server.onNotFound(webNotFound);
  }
  server.begin();
  wifiOn = true;
  logLine("WLAN: Webserver gestartet (Port 80)");
}

// ArduinoOTA + mDNS EINMAL PRO BOOT starten, egal wie oft WLAN danach ueber das Menue aus- und wieder
// eingeschaltet wird -- otaStarted/mdnsStarted verhindern ein erneutes begin(). Fruehere Version hat bei
// jedem WLAN-Aus (wifiStopAll()) auch MDNS.end() aufgerufen und otaStarted zurueckgesetzt, sodass beim
// naechsten Einschalten beides neu aufgebaut wurde -- das meldete sich im Log zwar brav als erfolgreich
// ("ArduinoOTA bereit"), aber der Port tauchte in der Arduino-IDE trotzdem oft minutenlang nicht (oder nie)
// wieder auf; nur ein echter Neustart (Reset/USB-Flash) brachte ihn sicher zurueck. Grund: der mDNS-Dienst
// des ESP32 (mdns_free()/mdns_init() unter der Haube von MDNS.end()/begin()) ist nicht zuverlaessig
// mehrfach neu startbar innerhalb eines Boots -- ein bekanntes Problem der Arduino-ESP32-Kernbibliothek,
// kein Fehler in diesem Code. Jetzt bleiben mDNS und ArduinoOTA einmal gestartet fuer die restliche
// Laufzeit stehen (ueberleben WLAN aus/an und auch den Wechsel Heimnetz<->eigener Zugangspunkt); nur ein
// echtes Herunterfahren (Menue Q, doShutdown()) beendet sie gar nicht mehr eigens, weil der Tiefschlaf
// ohnehin alles zuruecksetzt. ArduinoOTA.begin() meldet den "arduino"-Dienst (inkl. der TXT-Eintraege wie
// auth_upload=no, da nirgends ein Passwort gesetzt wird) bei mDNS BEREITS SELBST an -- ein zusaetzliches
// eigenes MDNS.addService("arduino", ...) hier wuerde den Dienst doppelt anmelden (frueher versehentlich
// drin: fuehrte zu einem Absturz direkt nach einem Upload, vermutlich durch die doppelte mDNS-
// Dienstanmeldung, und dazu, dass der Passwortdialog der Arduino-IDE faelschlich als Pflichtfeld
// auftauchte). Nur das MDNS.begin(WIFI_HOST) fuer den Hostnamen/"http"-Dienst macht nach wie vor der
// Aufrufer (siehe mdnsOtaEnsure() unten).
static bool mdnsStarted = false;
static bool otaStarted = false;
static void otaStart() {
  if (otaStarted) return;
  otaStarted = true;
  ArduinoOTA.setHostname(WIFI_HOST);
  ArduinoOTA.begin();
  logLine("WLAN: ArduinoOTA bereit (Werkzeuge -> Port -> Netzwerkport in der Arduino-IDE, kein Passwort)");
}
// mDNS einmal pro Boot starten (siehe Kommentar oben) und danach immer ArduinoOTA anstossen (otaStarted
// sorgt dafuer, dass das nur beim allerersten Mal wirklich etwas tut).
static void mdnsOtaEnsure() {
  if (!mdnsStarted && MDNS.begin(WIFI_HOST)) { MDNS.addService("http", "tcp", 80); mdnsStarted = true; }
  otaStart();
}

static void wifiStopAll() {
  if (wifiOn) server.stop();
  // Bewusst KEIN MDNS.end()/otaStarted=false mehr hier -- siehe Kommentar bei mdnsOtaEnsure() oben: das
  // war die Ursache dafuer, dass ArduinoOTA nach einem WLAN-Aus/-An nicht mehr zuverlaessig auftauchte.
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiOn = false;
  wifiConnecting = false;
}

// mode: 0 aus, 1 Heimnetz (Station), 2 eigener Zugangspunkt
static void wifiSet(int mode) {
  wifiStopAll();
  wifiMode = (uint8_t)mode;
  char m[120];
  if (mode == 2) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    wifiServerStart();
    mdnsOtaEnsure();                             // auch im eigenen Zugangspunkt per mDNS erreichbar -- vorher nur im Heimnetz
    wifiActivityMs = millis();          // Zaehler fuer die automatische Abschaltung (wifiIdleTick) neu starten
    snprintf(m, sizeof(m), "WLAN Zugangspunkt: SSID %s, Passwort %s, http://%s", WIFI_SSID, WIFI_PASS, WiFi.softAPIP().toString().c_str());
    logLine(m);
  } else if (mode == 1) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOST);
    WiFi.begin(wifiSsid, wifiPass[0] ? wifiPass : nullptr);
    wifiConnecting = true;
    wifiConnMs = millis();
    snprintf(m, sizeof(m), "WLAN: verbinde mit Heimnetz \"%s\" ...", wifiSsid);
    logLine(m);
  } else {
    logLine("WLAN aus");
  }
}

// Verbindungsaufbau zum Heimnetz begleiten (wird laufend aufgerufen)
static void wifiTick() {
  if (!wifiConnecting) return;
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnecting = false;
    wifiServerStart();
    mdnsOtaEnsure();
    char m[120];
    snprintf(m, sizeof(m), "WLAN Heimnetz verbunden: http://%s oder http://%s.local", WiFi.localIP().toString().c_str(), WIFI_HOST);
    logLine(m);
  } else if (millis() - wifiConnMs > 20000UL) {
    wifiConnecting = false;
    logLine("WLAN: Heimnetz nicht erreichbar, WLAN aus");   // kein automatischer Zugangspunkt mehr -- der wird nur
    wifiSet(0);                                             // manuell (Menue W) gestartet, sonst bleibt er ungenutzt an
  }
}

// Texteingabe ueber die Tastatur (alle Zeichen, Backspace loescht, Enter uebernimmt). Rueckgabe: Laenge der Eingabe (0 = leer)
#define FIND_BORDER 4           // Rahmenbreite (Px) fuer die Fundmeldungs-Bildschirme (findPick/textInput/findConfirm):
                                 // deutlich anderer Look als die normale Kartenansicht, bei Sonnenlicht im Feld
                                 // auf den ersten Blick von der Live-Anzeige zu unterscheiden
static void drawFindBorder(Adafruit_GFX *g) {
  g->fillRect(0, 0, 320, FIND_BORDER, ST77XX_RED);
  g->fillRect(0, 240 - FIND_BORDER, 320, FIND_BORDER, ST77XX_RED);
  g->fillRect(0, 0, FIND_BORDER, 240, ST77XX_RED);
  g->fillRect(320 - FIND_BORDER, 0, FIND_BORDER, 240, ST77XX_RED);
}

// Zwischenspeichert eine laufende Texteingabe ein paar Sekunden nach dem letzten Tastendruck auf der SD-
// Karte, damit bei einem Absturz/Reset mitten in der Eingabe (z.B. ein langer Fundbericht im Feld) nicht
// alles verloren ist. Schluessel ist der (auf a-z0-9 bereinigte) Titel des Feldes -- Passwortfelder werden
// nie zwischengespeichert. Der Entwurf wird beim normalen Verlassen von textInput() geloescht; ueberlebt er,
// heisst das: das Geraet ist waehrend dieser Eingabe abgestuerzt/neu gestartet, und textInput() bietet ihn
// beim naechsten Aufruf mit demselben Titel automatisch als Vorbelegung an (anstelle von "current").
#define DRAFT_IDLE_MS 3000
static void draftPathFor(const char *title, char *buf, size_t n) {
  size_t j = 0;
  if (j < n - 1) buf[j++] = '/';
  const char *pfx = "draft_";
  for (int i = 0; pfx[i] && j < n - 1; i++) buf[j++] = pfx[i];
  for (int i = 0; title[i] && j < n - 5; i++) if (isalnum((unsigned char)title[i])) buf[j++] = (char)tolower(title[i]);
  buf[j] = 0;
  strncat(buf, ".txt", n - strlen(buf) - 1);
}
static void draftSave(const char *title, const char *text) {
  if (!sdReady) return;
  char path[48];
  draftPathFor(title, path, sizeof(path));
  File f = SD.open(path, FILE_WRITE);   // ueberschreibt die Datei komplett neu (wie bei RECOVERY_QUEUE_PATH)
  if (!f) return;
  f.print(text);
  f.close();
}
static bool draftLoad(const char *title, char *out, size_t cap) {
  if (!sdReady) return false;
  char path[48];
  draftPathFor(title, path, sizeof(path));
  File f = SD.open(path);
  if (!f) return false;
  int n = f.read((uint8_t *)out, cap - 1);
  f.close();
  if (n <= 0) return false;
  out[n] = 0;
  return true;
}
static void draftClear(const char *title) {
  if (!sdReady) return;
  char path[48];
  draftPathFor(title, path, sizeof(path));
  SD.remove(path);
}

// Kuerzel fuer den Fundbericht: "z" + Ziffer + Leerzeichen wird automatisch durch den hinterlegten Satz
// ersetzt. Direkt nach dem Tippen eines einzelnen "z" (am Wortanfang) zeigt textInput() diese Liste neben
// dem Eingabefeld an, damit nur noch die Ziffer gelesen und getippt werden muss. Ein "#" im Satz markiert
// eine Einfuegestelle (z.B. die Hoehe in Metern bei z1) -- es wird selbst nicht eingefuegt, der Cursor
// landet nach dem Expandieren dort statt am Satzende, damit man direkt weiterschreiben kann.
struct TextShortcut { char key; const char *text; };
static const TextShortcut TEXT_SHORTCUTS[] = {
  {'1', "Haengt in # m, Bergung schwierig."},
  {'2', "Geborgen, Schirm nicht erreichbar."},
  {'3', "Geborgen, Sonde verbleibt ungeborgen."},
  {'4', "Komplett geborgen."},
  {'5', "Aus dem Baum."},
  {'6', "Vom Feld."},
};
#define TEXT_SHORTCUT_N (int)(sizeof(TEXT_SHORTCUTS) / sizeof(TEXT_SHORTCUTS[0]))

// Prueft, ob direkt vor dem Cursor soeben ein Leerzeichen nach einem "z<Ziffer>"-Kuerzel eingefuegt wurde,
// und ersetzt das Kuerzel dann durch den zugehoerigen Satz (das Leerzeichen bleibt erhalten). Passt der
// volle Satz nicht mehr in maxLen, wird er lieber abgeschnitten, als das Kuerzel unveraendert stehen zu
// lassen -- ein gekuerzter Fundbericht ist brauchbarer als einer mit einem nicht aufgeloesten "z1 " drin.
static bool shortcutExpand(char *tmp, int &len, int &cur, int maxLen) {
  int end = cur - 1;                        // Position des soeben eingefuegten Leerzeichens
  if (end < 2 || tmp[end] != ' ') return false;
  int start = end;
  while (start > 0 && tmp[start - 1] != ' ') start--;
  if (end - start != 2 || tmp[start] != 'z') return false;
  char d = tmp[start + 1];
  if (d < '1' || d > '6') return false;
  const char *exp = TEXT_SHORTCUTS[d - '1'].text;
  int expRawLen = (int)strlen(exp);
  int markerPos = -1;                       // Position von "#" im Satz (Einfuegestelle), falls vorhanden
  for (int i = 0; i < expRawLen; i++) if (exp[i] == '#') { markerPos = i; break; }
  int expLen = markerPos >= 0 ? expRawLen - 1 : expRawLen;   // "#" selbst wird nicht mit eingefuegt
  int restStart = end + 1, restLen = len - restStart;
  int avail = maxLen - start - 1 - restLen;   // Platz fuer den Ersatztext (minus dem Leerzeichen danach)
  if (avail < 0) avail = 0;
  if (expLen > avail) expLen = avail;         // nicht alles passt -> Satz an dieser Stelle kappen
  int newLen = start + expLen + 1 + restLen;
  memmove(tmp + start + expLen + 1, tmp + restStart, restLen + 1);   // Rest (inkl. abschliessender NUL) verschieben
  int w = start, written = 0;
  for (int i = 0; i < expRawLen && written < expLen; i++) {
    if (exp[i] == '#') continue;
    tmp[w++] = exp[i];
    written++;
  }
  tmp[start + expLen] = ' ';
  len = newLen;
  cur = (markerPos >= 0) ? start + (markerPos < expLen ? markerPos : expLen) : start + expLen + 1;
  return true;
}

// border=true: roter Rahmen + Titel in Rot statt Cyan -- fuer die Fundmeldung (Q), damit sie sich von den
// sonst gleich aussehenden Eingabefeldern (Heimnetz/Passwort usw.) klar abhebt.
// shortcuts=true: "z1".."z6" + Leerzeichen werden automatisch zu den hinterlegten Fundberichts-Saetzen
// expandiert (siehe TEXT_SHORTCUTS); nur fuer das Fundbericht-Feld gesetzt.
static int textInput(const char *title, char *out, size_t cap, bool mask, const char *current, const char *hint, bool border = false, bool shortcuts = false) {
  char tmp[64];
  int maxLen = (int)cap - 1;
  if (maxLen > 63) maxLen = 63;
  int len = 0;
  tmp[0] = 0;
  char draft[64];
  bool haveDraft = !mask && draftLoad(title, draft, sizeof(draft)) && draft[0];
  const char *prefill = haveDraft ? draft : current;    // ueberlebter Entwurf hat Vorrang vor "current"
  if (prefill && prefill[0]) {         // vorhandenen Text direkt editierbar vorbelegen (Cursor ans Ende) --
    len = (int)strlen(prefill);        // nicht nur anzeigen, damit man ihn aendern statt immer neu eintippen kann
    if (len > maxLen) len = maxLen;
    memcpy(tmp, prefill, len);
    tmp[len] = 0;
  }
  int cur = len;                       // Einfuegeposition in tmp[0..len]; per Trackball links/rechts verschiebbar
  uint32_t lastChangeMs = millis();
  bool dirty = false;                  // seit dem letzten Speichern geaendert?
  auto draw = [&]() {
    Adafruit_GFX *g = scr();
    g->setTextWrap(true);          // frameCanvas kann von der Kartenanzeige noch auf false stehen (dort bewusst
                                    // abgeschaltet, siehe mapUpdate/idleMapUpdate) -- hier soll der Eingabetext
                                    // umbrechen, statt am rechten Rand unsichtbar weiterzulaufen
    g->fillScreen(ST77XX_BLACK);
    g->setTextSize(2);
    g->setTextColor(border ? ST77XX_RED : ST77XX_CYAN);
    g->setCursor(4, 8);
    g->print(title);
    g->setTextColor(ST77XX_WHITE);
    g->setCursor(4, 50);
    for (int i = 0; i < cur; i++) g->print(mask && i < len - 1 ? '*' : tmp[i]);
    {   // Cursor als senkrechter Strich an der aktuellen Einfuegeposition (Breite/Hoehe passend zu setTextSize(2))
      int16_t cx = g->getCursorX(), cy = g->getCursorY();
      g->fillRect(cx, cy, 2, 16, ST77XX_YELLOW);
      g->setCursor(cx + 3, cy);
    }
    for (int i = cur; i < len; i++) g->print(mask && i < len - 1 ? '*' : tmp[i]);
    // Kuerzelliste: erscheint, sobald direkt vor dem Cursor ein einzelnes "z" am Wortanfang steht, damit nur
    // noch die Ziffer gelesen und eingetippt werden muss. Ueberschreibt dafuer den unteren Bildschirmteil.
    bool showMenu = shortcuts && cur > 0 && tmp[cur - 1] == 'z' && (cur == 1 || tmp[cur - 2] == ' ');
    if (showMenu) {
      g->fillRect(0, 128, 320, 68, ST77XX_BLACK);
      g->setTextSize(1);
      g->setTextColor(ST77XX_YELLOW);
      for (int i = 0; i < TEXT_SHORTCUT_N; i++) {
        g->setCursor(2, 130 + i * 11);
        g->print('z'); g->print(TEXT_SHORTCUTS[i].key); g->print(' ');
        g->setTextColor(ST77XX_WHITE);
        g->print(TEXT_SHORTCUTS[i].text);
        g->setTextColor(ST77XX_YELLOW);
      }
    }
    g->setTextSize(1);
    g->setTextColor(0x8410);
    g->setCursor(4, 198);
    g->print(hint);
    g->setCursor(4, 214);
    g->print("Links/Rechts: Cursor   Backspace: loeschen");
    g->setCursor(4, 227);
    g->print("2x Backspace: Feld leeren   Backspace bei leer: Abbruch");
    if (border) drawFindBorder(g);
    if (frameCanvas) blit(frameCanvas);
  };
  draw();
  uint32_t lastBackspaceMs = 0;                      // fuer "2x Backspace: Feld leeren" (siehe unten)
  bool aborted = false;
  while (true) {
    gpsPoll();
    int e = pollInput();
    if (e == EV_NONE) {
      if (!mask && dirty && millis() - lastChangeMs >= DRAFT_IDLE_MS) { draftSave(title, tmp); dirty = false; }
      delay(5);
      continue;
    }
    if (e == '\r' || e == '\n' || e == EV_CLICK) break;
    bool changed = true;
    if (e == EV_LEFT) { if (cur > 0) cur--; changed = false; }
    else if (e == EV_RIGHT) { if (cur < len) cur++; changed = false; }
    else if (e == 0x08) {                              // Backspace, drei Faelle (siehe Hinweiszeile):
      if (len == 0) { aborted = true; break; }          // Feld schon leer -- kein Reset mehr noetig, um rauszukommen
      if (millis() - lastBackspaceMs < 500) {           // kurz zuvor schon einmal Backspace: ganzes Feld auf einen
        len = 0; cur = 0; tmp[0] = 0;                   // Schlag leeren statt Zeichen fuer Zeichen (die Tastatur
      } else {                                           // meldet gedrueckt-Halten nicht, nur den einzelnen
        memmove(tmp + cur - 1, tmp + cur, len - cur);    // Tastendruck -- "echtes" Wiederholen durchs Halten geht
        cur--; len--; tmp[len] = 0;                       // mit diesem Tastatur-IC nicht)
      }
      lastBackspaceMs = millis();
    } else if (e >= 32 && e < 127 && len < maxLen) {    // Zeichen an der Cursorposition einfuegen, Rest schieben
      memmove(tmp + cur + 1, tmp + cur, len - cur);
      tmp[cur] = (char)e;
      cur++; len++; tmp[len] = 0;
      if (shortcuts && e == ' ') shortcutExpand(tmp, len, cur, maxLen);
    } else changed = false;
    if (changed) { dirty = true; lastChangeMs = millis(); }
    draw();
  }
  if (!mask) draftClear(title);     // normal verlassen (gesendet/uebernommen/abgebrochen) -- kein Wiederherstellen mehr noetig
  strncpy(out, tmp, cap - 1);
  out[cap - 1] = 0;
  return aborted ? -1 : len;        // -1: per Backspace auf leerem Feld abgebrochen -- von den Aufrufern zu
                                      // unterscheiden von einer (je Feld unterschiedlich behandelten) leeren
                                      // Eingabe per Enter
}

// Heimnetz einrichten: Name und Passwort eintippen, speichern, ggf. neu verbinden
static void netSetup() {
  char s[33], p[64];
  int n = textInput("Heimnetz (SSID)", s, sizeof(s), false, wifiSsid, "Enter: uebernehmen   leer + Enter: abbrechen");
  if (n <= 0) return;
  int pn = textInput("Passwort", p, sizeof(p), true, "", "Enter: uebernehmen   leer + Enter: offenes Netz");
  if (pn < 0) return;                             // per Backspace auf leerem Feld abgebrochen -- nichts speichern
                                                     // (anders als eine bewusst leere Eingabe: die bedeutet hier
                                                     // "offenes Netz" und soll weiter gespeichert werden)
  strncpy(wifiSsid, s, sizeof(wifiSsid) - 1);
  wifiSsid[sizeof(wifiSsid) - 1] = 0;
  strncpy(wifiPass, p, sizeof(wifiPass) - 1);
  wifiPass[sizeof(wifiPass) - 1] = 0;
  prefs.putString("ssid", wifiSsid);
  prefs.putString("wpass", wifiPass);
  if (wifiMode == 1) wifiSet(1);                  // mit den neuen Daten neu verbinden
}

// ---------- Sonde + Position fuer die Fundmeldung waehlen (Taste Q) ----------
// Liste aller Sonden, von denen eine GPX-Spur auf der SD-Karte liegt (Dateiname = Sonden-ID, kein Bindestrich --
// eigene Tracks heissen "JJJJ-MM-TT_hhmmss.gpx" und werden daran erkannt/uebersprungen), neueste zuerst.
// Vorbelegt ist die zuletzt tatsaechlich empfangene Sonde (falls gerade/zuletzt eine lief); mit dem Trackball
// (hoch/runter) oder O/P blaettert man durch, die Spur + der Endpunkt der gewaehlten Sonde werden auf der Karte
// angezeigt. Bestaetigt (Klick/Enter) wird mit dem LETZTEN GPX-Punkt dieser Sonde -- bei der gerade laufend
// empfangenen Sonde ist das die frischeste Live-Position (noch nicht unbedingt auf die SD geschrieben).
#define FIND_PICK_MAX 40
static char findPickId[FIND_PICK_MAX][9];
static uint32_t findPickTime[FIND_PICK_MAX];
static int findPickN = 0;
static int findPickSel = 0;

// In dieser Sitzung zuletzt empfangene Sonden-IDs (neueste zuerst) -- damit findPickScan() im Normalfall GAR
// NICHT die SD-Karte durchsuchen muss (das machte Taste Q langsam: jede GPX-Datei im Verzeichnis wurde dafuer
// angefasst). Wird bei jedem Sondenwechsel aktualisiert (siehe gpxSafeId()-Aufruf im RS41-Decoder).
#define FIND_PICK_RECENT_MAX 20
static char findPickRecentId[FIND_PICK_RECENT_MAX][9];
static int findPickRecentN = 0;

static void findPickRecentNote(const char *id) {
  int found = -1;
  for (int i = 0; i < findPickRecentN; i++) if (strncmp(findPickRecentId[i], id, 8) == 0) { found = i; break; }
  int from = (found >= 0) ? found : min(findPickRecentN, FIND_PICK_RECENT_MAX - 1);
  for (int i = from; i > 0; i--) strcpy(findPickRecentId[i], findPickRecentId[i - 1]);
  strncpy(findPickRecentId[0], id, 8);
  findPickRecentId[0][8] = 0;
  if (found < 0 && findPickRecentN < FIND_PICK_RECENT_MAX) findPickRecentN++;
}

#define FIND_TRACK_MAX 300
static float findTrackLat[FIND_TRACK_MAX], findTrackLon[FIND_TRACK_MAX];
static int findTrackN = 0;

// Extrahiert lat/lon/ele aus einem "<trkpt lat=\"..\" lon=\"..\">[<ele>..</ele>]"-Fragment. Bewusst kein sscanf
// mit %f/%lf: auf dem ESP32-Newlib nicht zuverlaessig -- daher wie im Rest der Datei (NMEA-Parser) atof().
static bool trkptParse(const char *p, double &lat, double &lon, float &alt) {
  const char *la = strstr(p, "lat=\"");
  const char *lo = strstr(p, "lon=\"");
  const char *el = strstr(p, "<ele>");
  if (!la || !lo) return false;
  lat = atof(la + 5);
  lon = atof(lo + 5);
  alt = el ? (float)atof(el + 5) : 0.0f;
  return true;
}

// Fortschrittsanzeige fuer Taste Q -- schreibt direkt aufs Display wie bootSplash()/shutdown(), weil hier
// bewusst kein Kartenrendering laeuft, das man sonst durchscheinen sehen wuerde. Gemeinsamer Zeichen-Code fuer
// zwei Phasen, die beide etwas dauern koennen (siehe findPickScan()/findTrackLoad()): die Dateisuche und das
// Laden der Vorschau-Spur -- letzteres lief bisher OHNE jede Anzeige, auch wenn die Dateisuche selbst (RAM-
// Liste) blitzschnell war, weil die Zeit tatsaechlich im Laden+Kartenzeichnen danach steckt.
static void findPickDrawBar(const char *label, int cur, int total) {
  tft.fillRect(0, 90, 320, 60, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(20, 92);
  tft.print(label);
  const int barX = 20, barY = 122, barW = 280, barH = 14;
  tft.drawRect(barX, barY, barW, barH, ST77XX_WHITE);
  int fillW = (total > 0) ? constrain((cur * (barW - 2)) / total, 0, barW - 2) : 0;
  if (fillW > 0) tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  char m[24];
  if (total > 0) snprintf(m, sizeof(m), "%d %%", constrain((cur * 100) / total, 0, 100));
  else snprintf(m, sizeof(m), "%d", cur);
  tft.setCursor(barX, barY + barH + 6);
  tft.print(m);
}

// 400 = die Obergrenze, bis zu der listDir() ueberhaupt Eintraege sammelt -- ohne bekannte Gesamtzahl vorab ist
// das der beste verfuegbare Bezugswert fuer den Balken der Dateisuche.
static void findPickProgressDraw(int n) { findPickDrawBar("Suche Sonden...", n, 400); }

// Fuer das Laden der Vorschau-Spur (findTrackLoad()) kennen wir die Dateigroesse vorher (f.size()) und damit
// einen echten Fortschritt -- genauer als bei der Dateisuche oben.
static void findPickTrackProgressDraw(int pos, int size) { findPickDrawBar("Lade Spur...", pos, size); }

// Letzten Trackpunkt einer Sonden-GPX-Datei lesen (liest nur das Dateiende zurueck, kein Komplett-Parsing).
static bool gpxLastPoint(const char *idStr, double &lat, double &lon, float &alt) {
  char path[48];
  snprintf(path, sizeof(path), "%s/%s.gpx", GPX_DIR, idStr);
  File f = SD.open(path);
  if (!f) return false;
  uint32_t sz = f.size();
  uint32_t back = (sz > 600) ? sz - 600 : 0;
  f.seek(back);
  char buf[610];
  int rn = f.read((uint8_t *)buf, sz - back);
  f.close();
  if (rn <= 0) return false;
  buf[rn] = 0;
  char *m = nullptr, *m2 = buf;
  while ((m2 = strstr(m2, "<trkpt")) != nullptr) { m = m2; m2++; }
  if (!m) return false;
  return trkptParse(m, lat, lon, alt);
}

// Vorschau-Spur einer Sonde aus ihrer GPX-Datei laden (auf FIND_TRACK_MAX Punkte ausgeduennt), nur fuer die
// Anzeige im Auswahlbildschirm -- ruehrt die laufende Live-Spur (trailLat/trailLon) nicht an.
// War bisher ZWEI komplette Durchgaenge durch die Datei (einmal zaehlen, um die Ausduennungs-Schrittweite zu
// kennen, dann erst auslesen) -- gemessen ca. 1.5 s fuer nur 649 Punkte, also der groesste Teil der Zeit hinter
// Taste Q. Jetzt EIN Durchgang: online ausduennen (Puffer wird bei FIND_TRACK_MAX voll, dann jeden 2. bereits
// gespeicherten Punkt behalten und die Schrittweite verdoppeln -- klassisches "Halbieren bei Ueberlauf", dadurch
// nie mehr als FIND_TRACK_MAX Punkte im RAM, ohne die Gesamtzahl vorher zu kennen).
static void findTrackLoad(const char *idStr) {
  uint32_t t0 = millis();                           // DIAGNOSE (temporaer): siehe findPickLoad()
  findTrackN = 0;
  char path[48];
  snprintf(path, sizeof(path), "%s/%s.gpx", GPX_DIR, idStr);
  File f = SD.open(path);
  if (!f) return;
  int fsize = (int)f.size();                        // fuer den Fortschrittsbalken: echte Byte-Position/-groesse,
  findPickTrackProgressDraw(0, fsize);               // keine zweite Schaetzung noetig wie beim Dateisuche-Balken
  uint32_t lastProgMs = millis();
  char line[160];
  int total = 0;                                    // Anzahl <trkpt> insgesamt gesehen (nur fuer die Log-Zeile)
  int keepEvery = 1;
  while (f.available()) {
    int len = f.readBytesUntil('\n', line, sizeof(line) - 1);
    if (millis() - lastProgMs >= 120) { lastProgMs = millis(); findPickTrackProgressDraw((int)f.position(), fsize); }
    if (len <= 0) continue;
    line[len] = 0;
    char *p = strstr(line, "<trkpt");
    if (!p) continue;
    if (total % keepEvery == 0) {
      double la, lo; float al;
      if (trkptParse(p, la, lo, al)) {
        if (findTrackN >= FIND_TRACK_MAX) {                  // Puffer voll: halbieren, Schrittweite verdoppeln
          for (int i = 0; i * 2 < findTrackN; i++) {
            findTrackLat[i] = findTrackLat[i * 2];
            findTrackLon[i] = findTrackLon[i * 2];
          }
          findTrackN /= 2;
          keepEvery *= 2;
        }
        findTrackLat[findTrackN] = (float)la;
        findTrackLon[findTrackN] = (float)lo;
        findTrackN++;
      }
    }
    total++;
  }
  f.close();
  findPickTrackProgressDraw(fsize, fsize);          // Balken sichtbar fertig, auch wenn der letzte Teil unter
                                                      // den 120 ms lag
  char m[112];
  snprintf(m, sizeof(m), "Fund-Zeit: Spur %s einlesen %lu ms, 1 Durchgang (%d Punkte in Datei, %d geladen)",
           idStr, (unsigned long)(millis() - t0), total, findTrackN);
  logLine(m);
}

// Liste der zur Auswahl stehenden Sonden (neueste zuerst). Schneller Weg: die in dieser Sitzung bereits
// empfangenen Sonden-IDs (findPickRecentId) direkt verwenden, ohne die SD-Karte zu durchsuchen -- das ist der
// Normalfall (man meldet fast immer eine Sonde, die man gerade selbst getrackt hat). Nur wenn diese Liste leer
// ist (z. B. direkt nach dem Einschalten, bevor in dieser Sitzung schon eine Sonde empfangen wurde) wird wie
// bisher die komplette SD-Karte durchsucht -- dabei zeigt findPickProgressDraw() einen Fortschrittsbalken, statt
// dass der Bildschirm scheinbar einfriert.
static void findPickScan() {
  uint32_t t0 = millis();                           // DIAGNOSE (temporaer): siehe findPickLoad()/findTrackLoad()
  findPickProgressDraw(0);                          // sofort zeigen, auch wenn der RAM-Pfad gleich greift -- die
                                                      // eigentliche Wartezeit danach (Spur laden/Karte zeichnen)
                                                      // war bisher komplett ohne jede Anzeige
  findPickN = 0;
  for (int i = 0; i < findPickRecentN && findPickN < FIND_PICK_MAX; i++) {
    char path[48];
    snprintf(path, sizeof(path), "%s/%s.gpx", GPX_DIR, findPickRecentId[i]);
    if (!SD.exists(path)) continue;                     // Datei zwischenzeitlich geloescht -- ueberspringen
    strcpy(findPickId[findPickN], findPickRecentId[i]);
    findPickTime[findPickN] = 0;
    findPickN++;
  }
  if (findPickN > 0) {
    char m[96];
    snprintf(m, sizeof(m), "Fund-Zeit: RAM-Liste %lu ms (%d Kandidaten, %d Treffer)",
             (unsigned long)(millis() - t0), findPickRecentN, findPickN);
    logLine(m);
    return;
  }
  std::vector<FileInfo> v;
  listDir(GPX_DIR, "", ".gpx", v, findPickProgressDraw);
  uint32_t t1 = millis();
  for (auto &f : v) {
    if (findPickN >= FIND_PICK_MAX) break;
    if (f.name.indexOf('-') >= 0) continue;             // eigener Track (Datum_Uhrzeit.gpx) -- ueberspringen
    String base = f.name.substring(0, f.name.length() - 4);
    if (base.length() == 0 || base == "unbekannt") continue;
    strncpy(findPickId[findPickN], base.c_str(), 8);
    findPickId[findPickN][8] = 0;
    findPickTime[findPickN] = (uint32_t)f.t;
    findPickN++;
  }
  for (int i = 1; i < findPickN; i++) {                 // Einfuegesortierung nach Zeit, neueste zuerst
    char idT[9]; strcpy(idT, findPickId[i]);
    uint32_t tT = findPickTime[i];
    int j = i - 1;
    while (j >= 0 && findPickTime[j] < tT) {
      strcpy(findPickId[j + 1], findPickId[j]);
      findPickTime[j + 1] = findPickTime[j];
      j--;
    }
    strcpy(findPickId[j + 1], idT);
    findPickTime[j + 1] = tT;
  }
  for (int i = findPickN - 1; i >= 0; i--) findPickRecentNote(findPickId[i]);   // Liste fuer den NAECHSTEN Q-Druck
                                                                                 // befuellen (rueckwaerts: aelteste
                                                                                 // zuerst eingefuegt, neueste landet vorn)
  char m[96];
  snprintf(m, sizeof(m), "Fund-Zeit: SD-Scan %lu ms (listDir), Rest %lu ms, %d Sonden gefunden",
           (unsigned long)(t1 - t0), (unsigned long)(millis() - t1), findPickN);
  logLine(m);
}

// Position fuer den Listeneintrag idx ermitteln (siehe Kommentar oben: live, falls das die gerade laufende
// Sonde ist, sonst letzter GPX-Punkt) und die Vorschau-Spur dazu laden.
static bool findPickLoad(int idx, double &lat, double &lon, float &alt) {
  uint32_t t0 = millis();                           // DIAGNOSE (temporaer): Zeitpunkte, siehe findTrackLoad()
  findTrackLoad(findPickId[idx]);
  uint32_t t1 = millis();
  bool ok;
  if (sondeId[0] && sondeId[0] != '?' && strncmp(sondeId, findPickId[idx], 8) == 0) {
    lat = sondeLat; lon = sondeLon; alt = sondeAlt;
    ok = true;
  } else {
    ok = gpxLastPoint(findPickId[idx], lat, lon, alt);
  }
  char m[96];
  snprintf(m, sizeof(m), "Fund-Zeit: findPickLoad gesamt %lu ms (Spur %lu ms, Position %lu ms)",
           (unsigned long)(millis() - t0), (unsigned long)(t1 - t0), (unsigned long)(millis() - t1));
  logLine(m);
  return ok;
}

// Zeichnet die aktuell gewaehlte Sonde (Vorschau-Spur + Endpunkt) direkt in den bgCanvas und auf den Schirm --
// unabhaengig von der Live-Kartenansicht (mapUpdate), die dabei unveraendert bleibt (siehe showCentered()).
static void findPickRenderMap(double lat, double lon, float alt, bool ok) {
  if (!bgCanvas) return;
  if (ok) {
    double plat[1] = {lat}, plon[1] = {lon};
    mapViewSetup(plat, plon, 1, false, true);
    renderBackground();
  } else if (viewZ < 0) {                               // noch nie gerendert und keine Position: irgendeinen Ausschnitt zeigen
    double hl, ho;
    homePosition(hl, ho);
    double plat[1] = {hl}, plon[1] = {ho};
    mapViewSetup(plat, plon, 1, false, true);
    renderBackground();
  }
  double ox = viewCx - 160.0, oy = viewCy - 120.0;
  if (ok && findTrackN > 1) {
    int px0 = 0, py0 = 0;
    for (int i = 0; i < findTrackN; i++) {
      double tx, ty;
      latLonToPixel(findTrackLat[i], findTrackLon[i], viewZ, tx, ty);
      int x = (int)floor(tx - ox), y = (int)floor(ty - oy);
      if (i > 0) { bgCanvas->drawLine(px0, py0, x, y, COL_TRAIL); bgCanvas->drawLine(px0 + 1, py0, x + 1, y, COL_TRAIL); }
      px0 = x; py0 = y;
    }
  }
  if (ok) {
    double ex, ey;
    latLonToPixel(lat, lon, viewZ, ex, ey);
    int eX = (int)floor(ex - ox), eY = (int)floor(ey - oy);
    bgCanvas->fillCircle(eX, eY, 6, ST77XX_WHITE);
    bgCanvas->fillCircle(eX, eY, 5, COL_SONDE_LOST);    // rot: alte/nicht-live Spur, nicht zu verwechseln mit der Live-Karte
  }
  bgCanvas->fillRect(0, 178, 320, 62, ST77XX_BLACK);
  bgCanvas->setTextWrap(false);
  char l1[48], l2[48];
  snprintf(l1, sizeof(l1), "Fund %d/%d: %s", findPickSel + 1, findPickN, findPickId[findPickSel]);
  bgCanvas->setTextSize(2);                             // groesser/rot statt der sonst ueberall kleinen weissen Schrift --
  bgCanvas->setTextColor(ST77XX_RED);                    // soll im Feld (Sonnenlicht, Eile) sofort als Fundmeldung erkennbar sein
  bgCanvas->setCursor(4, 182);
  bgCanvas->print(l1);
  bgCanvas->setTextSize(1);
  bgCanvas->setTextColor(ST77XX_WHITE);
  if (ok) snprintf(l2, sizeof(l2), "Hoehe %.0f m", alt);
  else strncpy(l2, "keine Position gefunden", sizeof(l2));
  bgCanvas->setCursor(4, 206); bgCanvas->print(l2);
  bgCanvas->setCursor(4, 220); bgCanvas->print("Rad/O/P waehlen  Klick melden  <- Abbruch");
  drawFindBorder(bgCanvas);
  blit(bgCanvas);
}

// Fuehrt den Auswahlbildschirm aus. Rueckgabe true: serial/lat/lon/alt gesetzt (Melder/Bemerkung folgen danach
// wie gehabt); false: Abbruch -- der Bildschirm ist dann schon wiederhergestellt (ausser es gab von vornherein
// keine GPX-Daten, dann wurde die Live-Ansicht nie verlassen).
static bool findPick(char *serial, double &lat, double &lon, float &alt) {
  uint32_t t0 = millis();                           // DIAGNOSE (temporaer): Gesamtbild zu den Einzelzeiten oben
  findPickScan();
  uint32_t t1 = millis();
  if (findPickN == 0) { toast("Sonde gefunden", "keine GPX-Daten"); return false; }
  findPickSel = 0;
  if (sondeId[0] && sondeId[0] != '?')
    for (int i = 0; i < findPickN; i++) if (strncmp(findPickId[i], sondeId, 8) == 0) { findPickSel = i; break; }
  bool savedManual = mapManual;
  mapManual = false;                                    // waehrend der Auswahl immer auf den gewaehlten Punkt zentrieren
  bool ok = findPickLoad(findPickSel, lat, lon, alt);
  uint32_t t2 = millis();
  findPickRenderMap(lat, lon, alt, ok);
  char m[112];
  snprintf(m, sizeof(m), "Fund-Zeit gesamt bis zur Anzeige: %lu ms (Scan %lu ms, Laden %lu ms, Karte zeichnen %lu ms)",
           (unsigned long)(millis() - t0), (unsigned long)(t1 - t0), (unsigned long)(t2 - t1), (unsigned long)(millis() - t2));
  logLine(m);
  bool confirmed = false;
  while (true) {
    gpsPoll();
    int e = pollInput();
    if (e == EV_NONE) { delay(5); continue; }
    if (e == 0x08) break;                               // Backspace: Abbruch
    if (e == '\r' || e == '\n' || e == EV_CLICK) { confirmed = true; break; }
    int nsel = findPickSel;
    if (e == EV_UP || e == 'o') nsel--;
    else if (e == EV_DOWN || e == 'p') nsel++;
    else continue;
    if (nsel < 0) nsel = findPickN - 1;
    if (nsel >= findPickN) nsel = 0;
    if (nsel != findPickSel) {
      findPickSel = nsel;
      ok = findPickLoad(findPickSel, lat, lon, alt);
      findPickRenderMap(lat, lon, alt, ok);
    }
  }
  mapManual = savedManual;
  viewZ = -1;                                           // fuer die normale Kartenansicht neu rendern (siehe showCentered())
  showView();
  if (!confirmed) { toast("Sonde gefunden", "abgebrochen"); return false; }
  if (!ok) { toast("Sonde gefunden", "keine Position"); return false; }
  strncpy(serial, findPickId[findPickSel], 8);
  serial[8] = 0;
  return true;
}

// Letzter Schritt vor dem Absenden: Sonde/Position/Melder/Bemerkung nochmal zur Kontrolle anzeigen, mit der
// Moeglichkeit, Melder (C) oder Bemerkung (B) direkt noch zu korrigieren, bevor wirklich gesendet wird.
// Rueckgabe true: senden (by/desc enthalten den -- ggf. korrigierten -- Endstand); false: Abbruch.
static bool findConfirm(const char *serial, double lat, double lon, float alt, char *by, size_t byCap, char *desc, size_t descCap) {
  auto draw = [&]() {
    Adafruit_GFX *g = scr();
    g->setTextWrap(true);
    g->fillScreen(ST77XX_BLACK);
    g->setTextSize(2);
    g->setTextColor(ST77XX_RED);
    g->setCursor(4, 6);
    g->print("Fundmeldung pruefen");
    g->setTextSize(1);
    g->setTextColor(ST77XX_WHITE);
    g->setCursor(4, 32);
    g->print("Sonde: "); g->print(serial);
    g->setCursor(4, 44);
    g->print(String(lat, 6)); g->print(", "); g->print(String(lon, 6));
    g->print("   "); g->print(String(alt, 0)); g->print(" m");
    g->setTextColor(ST77XX_YELLOW);
    g->setCursor(4, 62);
    g->print("Melder [C]: "); g->setTextColor(ST77XX_WHITE); g->print(by);
    g->setTextColor(ST77XX_YELLOW);
    g->setCursor(4, 76);
    g->print("Bemerkung [B]:");
    g->setTextColor(ST77XX_WHITE);
    g->setCursor(4, 88);
    g->print(desc);
    g->setTextColor(0x8410);
    g->setCursor(4, 206);
    g->print("Klick: senden   C/B: aendern");   // bewusst NUR Klick, kein Enter -- sonst koennte ein
                                                  // ueberzaehliges Enter aus der Texteingabe hier versehentlich senden
    g->setCursor(4, 220);
    g->print("Backspace: abbrechen");
    drawFindBorder(g);
    if (frameCanvas) blit(frameCanvas);
  };
  while (true) {
    draw();
    int e;
    do { gpsPoll(); e = pollInput(); if (e == EV_NONE) delay(5); } while (e == EV_NONE);
    if (e == 0x08) return false;
    if (e == EV_CLICK) return true;
    if (e == 'c') {
      char tmp[40];
      textInput("Melder (Rufzeichen/Name)", tmp, byCap < sizeof(tmp) ? byCap : sizeof(tmp), false, by,
                "Enter: uebernehmen   leer: vorigen Melder behalten", true);
      if (tmp[0]) { strncpy(by, tmp, byCap - 1); by[byCap - 1] = 0; }
    } else if (e == 'b') {
      char tmp[160];
      textInput("Fundbericht", tmp, descCap < sizeof(tmp) ? descCap : sizeof(tmp), false, desc,
                "Enter: uebernehmen   leer: Bemerkung unveraendert lassen", true, true);
      if (tmp[0]) { strncpy(desc, tmp, descCap - 1); desc[descCap - 1] = 0; }
    }
    // andere Taste: ignorieren, Pruefbildschirm bleibt stehen
  }
}

// ---------- SondeHub-Fundmeldung (Taste Q) ----------
// PUT https://api.v2.sondehub.org/recovered -- Meldungen werden von SondeHub automatisch auch zu
// radiosondy.info weitergereicht, ein zweiter Upload dorthin ist also nicht noetig.
// Braucht eine Verbindung mit echtem Internetzugang (Menue W: Heimnetz) -- der eigene Zugangspunkt
// hat keinen Internet-Zugang und kann diese Meldung nicht absetzen.
static void httpDateStr(char *buf, size_t n) {       // RFC7231, von der API verlangt (Zeitversatz-Korrektur)
  static const char *const wd[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *const mo[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  time_t t = time(nullptr);
  struct tm tmv;
  gmtime_r(&t, &tmv);
  snprintf(buf, n, "%s, %02d %s %04d %02d:%02d:%02d GMT", wd[tmv.tm_wday], tmv.tm_mday, mo[tmv.tm_mon],
           tmv.tm_year + 1900, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

// Minimaler JSON-String-Escaper: Anfuehrungszeichen/Backslash maskieren, Steuerzeichen auslassen,
// alles andere (auch mehrbytiges UTF-8, z. B. Umlaute von der Tastatur) unveraendert durchreichen.
static void jsonEsc(const char *s, char *out, size_t cap) {
  size_t o = 0;
  for (; s && *s && o + 2 < cap; s++) {
    unsigned char c = (unsigned char)*s;
    if (c == '"' || c == '\\') { out[o++] = '\\'; out[o++] = (char)c; }
    else if (c == '\n') { out[o++] = '\\'; out[o++] = 'n'; }
    else if (c >= 0x20) { out[o++] = (char)c; }
  }
  out[o] = 0;
}

// Sendet die Fundmeldung; lat/lon/alt = Sondenposition zum Zeitpunkt des Tastendrucks (also die zuletzt
// empfangene, da sondeLat/-Lon/-Alt sich nur bei einem neuen Paket aendern). Bei Erfolg true, sonst false
// mit einer kurzen Fehlermeldung in errOut.
// Warteliste fuer Fundmeldungen ohne Internet (bis RECOVERY_QUEUE_MAX Eintraege, als fertige JSON-Zeilen
// auf der SD-Karte). Aelteste Zeile zuerst; wird im Heimnetz automatisch abgearbeitet (recoveryQueueTick()).
#define RECOVERY_QUEUE_PATH "/recovery_queue.txt"
#define RECOVERY_QUEUE_MAX  10
#define RECOVERY_RETRY_MS   60000     // sporadische Pruefung, ob wieder (echtes) Internet da ist
#define RECOVERY_DRAIN_GAP_MS 5000    // Abstand zwischen zwei automatischen Sendungen (keine Sperre riskieren)

static int recoveryCount() {
  if (!sdReady) return 0;
  File f = SD.open(RECOVERY_QUEUE_PATH);
  if (!f) return 0;
  int n = 0;
  while (f.available()) if (f.read() == '\n') n++;
  f.close();
  return n;
}

static bool recoveryPush(const char *body) {
  if (!sdInit()) return false;
  if (recoveryCount() >= RECOVERY_QUEUE_MAX) return false;
  File f = SD.open(RECOVERY_QUEUE_PATH, FILE_APPEND);
  if (!f) return false;
  f.println(body);
  f.close();
  return true;
}

// Aelteste Zeile (bis zu cap-1 Zeichen); true, wenn eine vorhanden war.
static bool recoveryPeek(char *out, size_t cap) {
  if (!sdReady) return false;
  File f = SD.open(RECOVERY_QUEUE_PATH);
  if (!f) return false;
  size_t n = 0;
  bool got = false;
  while (f.available()) {
    int c = f.read();
    if (c == '\n') { got = (n > 0); break; }
    if (n < cap - 1) out[n++] = (char)c;
  }
  out[n] = 0;
  f.close();
  return got;
}

// Entfernt die aelteste Zeile (Rest einlesen, Datei neu schreiben -- die Liste ist immer klein, max. 10 Zeilen).
static void recoveryPop() {
  if (!sdReady) return;
  File f = SD.open(RECOVERY_QUEUE_PATH);
  if (!f) return;
  String rest;
  bool skippedFirst = false;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (!skippedFirst) { skippedFirst = true; continue; }
    rest += line;
    rest += '\n';
  }
  f.close();
  File nf = SD.open(RECOVERY_QUEUE_PATH, FILE_WRITE);   // ueberschreibt die Datei komplett neu
  if (nf) {
    if (rest.length()) nf.print(rest);
    nf.close();
  }
}

// Sendet einen fertigen JSON-Body direkt -- fuer den Sofortversuch bei Tastendruck UND fuer das
// automatische Abarbeiten der Warteliste (recoveryQueueTick()).
static bool sendRecoveryBody(const char *body, char *errOut, size_t errLen) {
  char dateStr[40];
  httpDateStr(dateStr, sizeof(dateStr));
  NetworkClientSecure client;
  client.setInsecure();                            // keine Zertifikatspruefung (kein sensibler Inhalt, nur eine Fundmeldung)
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(client, "https://api.v2.sondehub.org/recovered")) {
    snprintf(errOut, errLen, "Verbindung fehlgeschlagen");
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Date", dateStr);
  http.addHeader("User-Agent", "tdeck-rs41");
  int code = http.PUT((uint8_t *)body, strlen(body));
  String resp = code > 0 ? http.getString() : String(http.errorToString(code).c_str());
  http.end();
  char m[260];
  snprintf(m, sizeof(m), "SondeHub-Fundmeldung: HTTP %d -- %s", code, resp.c_str());
  logLine(m);
  if (code == 200) return true;
  snprintf(errOut, errLen, "HTTP %d", code);
  return false;
}

// Baut den JSON-Body fuer eine Fundmeldung.
static void buildRecoveryBody(char *buf, size_t cap, const char *serial, double lat, double lon, float alt,
                               const char *by, const char *desc) {
  char eBy[80], eDesc[220];
  jsonEsc(by, eBy, sizeof(eBy));
  jsonEsc(desc, eDesc, sizeof(eDesc));
  snprintf(buf, cap,
           "{\"serial\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,\"recovered\":true,"
           "\"recovered_by\":\"%s\",\"description\":\"%s\",\"recovery_software\":\"tdeck-rs41\"}",
           serial, lat, lon, (double)alt, eBy, eDesc);
}

// Versucht die Fundmeldung sofort zu senden (nur sinnvoll, wenn gerade das Heimnetz aktiv ist); klappt das
// nicht (kein echtes Internet trotz Verbindung, oder gar keine Verbindung), landet sie in der Warteliste.
// Rueckgabe: 0 sofort gesendet, 1 in Warteliste gelegt, 2 Warteliste war voll (nichts gespeichert).
static int queueOrSendRecovery(const char *serial, double lat, double lon, float alt, const char *by, const char *desc) {
  char body[420];
  buildRecoveryBody(body, sizeof(body), serial, lat, lon, alt, by, desc);
  if (wifiOn && wifiMode == 1) {
    char err[40];
    if (sendRecoveryBody(body, err, sizeof(err))) return 0;
  }
  return recoveryPush(body) ? 1 : 2;
}

// ---------- SondeHub-Live-Upload (Menue A: aus / Chase-Car) ----------
// NUR noch die eigene Empfaengerposition (PUT /listeners, dieselbe API, die auch radiosonde_auto_rx/
// ChaseMapper benutzen) -- fuer die Karte auf sondehub.org/sondehub-amateur. Rufzeichen kommt von
// FW_CALLSIGN, als "Antenne" steht die tatsaechlich verwendete Antenne (SONDEHUB_ANTENNA), damit auf der
// Karte erkennbar ist, womit empfangen wurde. "mobile" ist nur true, waehrend sich die Station auch
// wirklich bewegt (dann alle 30 s, erscheint als fahrender Chase-Car-Marker) -- im Stand wird viel seltener
// (alle 15 min) als mobile:false gesendet, auf Mark VK5QIs Wunsch, keinen staendig "fahrenden" Marker fuer
// eine eigentlich stehende Station zu zeigen. Die Station bleibt dabei aber immer auf der Karte sichtbar --
// siehe SONDEHUB_MOVE_THRESHOLD_M/sondehubListenerTick().
//
// Den Sonden-Telemetrie-Upload (PUT /sondes/telemetry) gibt es bewusst NICHT mehr: beim Pruefen der
// Decodier-Logik (Antwort von Mark VK5QI auf den ersten Payload-Dump, Okt. 2026) ist aufgefallen, dass
// "datetime" (Block 0x7C, Schritt 1, Frame N) und Position/Hoehe (Block 0x7B, Schritt 2, ERST im naechsten
// Frame N+1, ca. 0,7-1,3 s spaeter) nicht aus demselben Moment stammen -- bei den hier beobachteten
// Geschwindigkeiten ein systematischer Versatz von ca. 15-20 m horizontal, 2-3 m vertikal. Eine Korrektur
// (Zeit nachstellen oder Position per vel_h/vel_v/heading zurueckrechnen) waere beim Abgleich mit anderen
// Empfangsstationen irgendwann aufgefallen -- lieber gar keine Sondenposition hochladen, als eine, die nicht
// zuverlaessig zum angegebenen Zeitpunkt passt.
// Braucht echtes Internet (Heimnetz, wifiMode 1).
#define SONDEHUB_LISTENER_URL       "https://api.v2.sondehub.org/listeners"
#define SONDEHUB_LISTENER_CHECK_MS    30000UL     // Bewegung wird alle 30 s geprueft (Marks Vorschlag fuer eine mobile Station)
                                                   // -- bei Bewegung wird auch in diesem Takt als mobile:true gesendet
#define SONDEHUB_LISTENER_STATIC_RETRY_MS 900000UL      // im Stand seltener (alle 15 min) als mobile:false -- bewusst NICHT
                                                         // die bei auto_rx uebliche fixe Station mit Stunden-Takt, weil das
                                                         // T-Deck nie stundenlang durchlaeuft, sondern pro Einsatz ein- und
                                                         // wieder ausgeschaltet wird; Mark moechte nur keinen staendig
                                                         // "fahrenden" Chase-Car-Marker von einer eigentlich stehenden
                                                         // Station sehen, nicht dass sie ganz verschwindet
#define SONDEHUB_MOVE_THRESHOLD_M    25.0         // ab dieser Distanz (je Pruefung) gilt die Station als "bewegt" -- GPS-
                                                   // Jitter im Stand liegt beim T-Deck-GPS-Modul typ. deutlich darunter
#define SONDEHUB_ANTENNA            "Comet SMA-24/tuned T-Deck Antenna"   // echte Antenne statt nur Geraetename

// sondehubMode selbst steht ganz oben bei den anderen Einstellungs-Variablen (vor loadSettings()).
static uint32_t sondehubListenerNextMs = 0;        // naechste Bewegungspruefung (alle 30 s)
static uint32_t sondehubListenerStaticNextMs = 0;  // naechstes Pflicht-Update im Stand (alle 15 min, mobile:false)
static double sondehubMoveAnchorLat = 0, sondehubMoveAnchorLon = 0;   // letzte geprueft Position, fuer die Bewegungserkennung
static bool sondehubMoveAnchorSet = false;

// mobile: true waehrend sich die Station tatsaechlich bewegt (erscheint dann auf sondehub.org als fahrender
// Chase-Car-Marker), false im Stand (ganz normale, statische Empfangsstation -- kein Chase-Car-Marker, aber
// trotzdem sichtbar). Wir senden IMMER etwas, nur eben seltener und als mobile:false, wenn wir stehen --
// Mark VK5QIs Wunsch war nur, keinen staendig "fahrenden" Marker fuer eine eigentlich stehende Station zu
// zeigen, nicht, dass die Station beim Stehen ganz von der Karte verschwindet.
static void sondehubUploadListener(bool mobile) {
  double hl, ho;
  if (!homePosition(hl, ho)) return;   // kein aktueller GPS-Fix -- lieber gar nichts melden und es beim
                                        // naechsten Versuch erneut probieren, statt eine falsche Position zu senden
  char body[300];                                     // etwas groesser wegen des laengeren SONDEHUB_ANTENNA-Texts
  snprintf(body, sizeof(body),
           "{\"software_name\":\"tdeck-rs41\",\"software_version\":\"%s\",\"uploader_callsign\":\"%s\","
           "\"uploader_position\":[%.6f,%.6f,%.0f],\"uploader_antenna\":\"" SONDEHUB_ANTENNA "\","
           "\"uploader_contact_email\":\"none@none.com\",\"mobile\":%s}",
           FW_VERSION, FW_CALLSIGN, hl, ho, (double)gps.alt, mobile ? "true" : "false");
  NetworkClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(client, SONDEHUB_LISTENER_URL)) { logLine("SondeHub-Chase-Car: Verbindung fehlgeschlagen"); return; }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "tdeck-rs41");
  int code = http.PUT((uint8_t *)body, strlen(body));
  char m[420];                                       // bei Nicht-200 Antworttext mitloggen
  if (code == 200) {
    snprintf(m, sizeof(m), "SondeHub-Chase-Car: HTTP %d", code);
  } else {
    String resp = http.getString();
    snprintf(m, sizeof(m), "SondeHub-Chase-Car: HTTP %d -- %.370s", code, resp.c_str());
  }
  logLine(m);
  http.end();
}

// Periodischer Listener-Upload im Hintergrund -- nur mit echtem Internet. Alle SONDEHUB_LISTENER_CHECK_MS
// (30 s) wird geprueft, ob sich die Position seit der letzten Pruefung um mehr als SONDEHUB_MOVE_THRESHOLD_M
// veraendert hat (Anker wandert bei jeder Pruefung mit, damit ein langsames Herumstehen mit leicht wanderndem
// GPS-Fix nicht ueber viele Takte hinweg aufsummiert faelschlich als Bewegung durchgeht):
//  - bewegt: sofort als mobile:true senden (also im selben 30-s-Takt wie die Pruefung) -- erscheint auf der
//    Karte als fahrender Chase-Car-Marker, genau wie von Mark VK5QI vorgeschlagen.
//  - steht: nicht bei jeder Pruefung senden (das waere viel zu oft fuer eine ruhende Station), sondern nur
//    alle SONDEHUB_LISTENER_STATIC_RETRY_MS (15 min) als mobile:false -- die Station bleibt damit immer auf der
//    Karte sichtbar, zeigt im Stand aber keinen staendig "fahrenden" Marker (das wollte Mark vermeiden).
static void sondehubListenerTick() {
  if (sondehubMode < 1 || !(wifiOn && wifiMode == 1)) return;
  if ((int32_t)(millis() - sondehubListenerNextMs) < 0) return;
  sondehubListenerNextMs = millis() + SONDEHUB_LISTENER_CHECK_MS;

  double hl, ho;
  if (!homePosition(hl, ho)) return;             // kein aktueller GPS-Fix -- siehe sondehubUploadListener()
  if (!gpsFixGoodEnoughToSend()) return;         // Fix da, aber Guete (Sat-Minimum/HDOP) nicht ausreichend -- lieber warten

  if (!sondehubMoveAnchorSet) {        // erster Durchlauf (oder nach Fix-Verlust/-Neuerwerb): nur Anker setzen
    sondehubMoveAnchorLat = hl;
    sondehubMoveAnchorLon = ho;
    sondehubMoveAnchorSet = true;
    sondehubListenerStaticNextMs = millis() + SONDEHUB_LISTENER_STATIC_RETRY_MS;
    return;
  }
  double movedM = haversineKm(sondehubMoveAnchorLat, sondehubMoveAnchorLon, hl, ho) * 1000.0;
  sondehubMoveAnchorLat = hl;           // Anker immer auf die aktuelle Position nachziehen, s. o.
  sondehubMoveAnchorLon = ho;
  if (movedM >= SONDEHUB_MOVE_THRESHOLD_M) {
    sondehubUploadListener(true);                                        // bewegt: als Chase-Car, alle 30 s
    sondehubListenerStaticNextMs = millis() + SONDEHUB_LISTENER_STATIC_RETRY_MS;  // Stand-Zaehler neu starten
    return;
  }
  if ((int32_t)(millis() - sondehubListenerStaticNextMs) < 0) return;     // steht noch, naechstes Pflicht-Update erst spaeter
  sondehubListenerStaticNextMs = millis() + SONDEHUB_LISTENER_STATIC_RETRY_MS;
  sondehubUploadListener(false);                                          // steht: seltenes Update als normale Station
}

// Erkennt den Verbindungsverlust im Heimnetz (Reichweite verlassen, Netz weg) und schaltet das WLAN dann
// sauber ab. Im Heimnetz selbst (das Geraet haengt dort ohnehin am Strom) bleibt WLAN dagegen einfach an,
// solange die Verbindung steht -- es gibt kein "fertig, also wieder aus" mehr wie frueher.
static uint32_t wifiLostMs = 0;
static void wifiLinkTick() {
  if (!(wifiOn && wifiMode == 1)) { wifiLostMs = 0; return; }
  if (WiFi.status() == WL_CONNECTED) { wifiLostMs = 0; return; }
  if (!wifiLostMs) { wifiLostMs = millis(); return; }     // kurzen Aussetzer (Roaming o.ae.) nicht sofort werten
  if (millis() - wifiLostMs > WIFI_LOST_MS) {
    logLine("WLAN: Heimnetz-Verbindung verloren (ausser Reichweite?), WLAN aus");
    wifiSet(0);
  }
}

// Nur der eigene Zugangspunkt (immer manuell gestartet, siehe Menue W) schaltet bei Nichtnutzung automatisch ab --
// das Heimnetz bleibt an, solange die Verbindung steht (dort haengt das T-Deck ohnehin am Strom, siehe wifiLinkTick
// fuer den Fall "ausser Reichweite"). Zugangspunkt gilt als genutzt, solange ein Geraet verbunden ist.
static void wifiIdleTick() {
  if (!wifiOn || wifiMode != 2) return;
  if (WiFi.softAPgetStationNum() > 0) { wifiActivityMs = millis(); return; }
  if (millis() - wifiActivityMs > WIFI_IDLE_OFF_MS) {
    logLine("WLAN: Zugangspunkt ohne Verbindung, WLAN aus");
    wifiSet(0);
  }
}

// Uhrzeit per NTP holen (nutzt ein ohnehin -- fuer Fundmeldungen oder eigens dafuer -- gestartetes WLAN
// mit): solange noch kein GPS-Fix vorliegt, sonst stuenden Log-/GPX-Zeitstempel bis dahin auf 1980.
static void ntpTick() {
  if (!ntpWanted) return;
  if (timeSet) { ntpWanted = false; return; }        // GPS hat die Zeit inzwischen selbst gesetzt: NTP nicht mehr noetig
  if (!(wifiOn && wifiMode == 1)) return;
  if (!ntpConfigured) {
    configTzTime(TZ_RULE, NTP_SERVER);                // TZ-String direkt uebergeben, sonst wirft configTime() die DST-Regel weg (-> UTC statt Lokalzeit)
    ntpConfigured = true;
    ntpStartMs = millis();
    logLine("WLAN: hole Uhrzeit per NTP (" NTP_SERVER ")");
  }
  time_t now = time(nullptr);
  if (now > 1700000000) {                          // > 2023: NTP hat die Systemzeit gesetzt
    onTimeSet("NTP");
    ntpWanted = false;
  } else if (millis() - ntpStartMs > NTP_TIMEOUT_MS) {
    logLine("WLAN: NTP nicht erreichbar, gebe auf");
    ntpWanted = false;
  }
}

// Periodischer, sparsamer Versuch, die Warteliste abzuarbeiten: nur im Heimnetz, hoechstens alle
// RECOVERY_RETRY_MS ein Versuch, nach einem Erfolg nur RECOVERY_DRAIN_GAP_MS bis zum naechsten
// (um den Rest der Liste zuegig loszuwerden, ohne die API mit vielen Anfragen kurz hintereinander zu belasten).
static uint32_t recoveryNextMs = 0;
static void recoveryQueueTick() {
  if (!(wifiOn && wifiMode == 1)) return;
  if ((int32_t)(millis() - recoveryNextMs) < 0) return;
  char body[420];
  if (!recoveryPeek(body, sizeof(body))) {
    recoveryNextMs = millis() + RECOVERY_RETRY_MS;
    return;
  }
  char err[40];
  if (sendRecoveryBody(body, err, sizeof(err))) {
    recoveryPop();
    int left = recoveryCount();
    char m[48];
    snprintf(m, sizeof(m), "Warteliste: gesendet, %d verbleiben", left);
    logLine(m);
    recoveryNextMs = millis() + (left > 0 ? RECOVERY_DRAIN_GAP_MS : RECOVERY_RETRY_MS);
  } else {
    recoveryNextMs = millis() + RECOVERY_RETRY_MS;   // vermutlich noch kein echtes Internet -- spaeter erneut versuchen
  }
}


// ---------- Geordnetes Herunterfahren (Menue Q) ----------
static void doShutdown() {
  logLine("### Geordnetes Herunterfahren");
  gpxDrainAll();                                   // GPX-Zwischenspeicher noch sichern
  if (wifiOn) wifiStopAll();
  radio.standby();
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(6, 100);
  tft.print("Fertig, schalte ab ...");
  tft.setTextSize(1);
  tft.setTextColor(0x8410);
  tft.setCursor(6, 130);
  tft.print("Zum Neustart: Reset oder Ein-/Ausschalter");
  delay(3000);                                      // Text in Ruhe lesbar, bevor es wirklich abschaltet
  digitalWrite(PIN_POWER_ON, LOW);                   // T-Deck Plus: GPIO10 haelt im Batteriebetrieb die Stromversorgung
                                                      // des Boards; LOW schaltet es dadurch tatsaechlich komplett ab
  delay(500);                                        // falls das Board trotzdem weiterlaeuft (z. B. am USB-Kabel):
  analogWrite(PIN_TFT_BL, 0);                        // wenigstens Hintergrundbeleuchtung aus und in Tiefschlaf
  esp_deep_sleep_start();                            // schlaeft, bis per Reset/Schalter neu gestartet wird
}

static void menuValue(int i, char *b, size_t n) {
  switch (i) {
    case MI_SEARCH: snprintf(b, n, "%s", fixedFreq ? "fest" : "Auto"); break;
    case MI_FREQ:   snprintf(b, n, "%.3f MHz", freqMHz); break;
    case MI_BW:     snprintf(b, n, "%.1f kHz", rxBw); break;
    case MI_FIX:    snprintf(b, n, "%s", fixOn ? "an" : "aus"); break;
    case MI_ID:   snprintf(b, n, "%s", radioMode == 1 ? "LoRa" : "Sondenempfang"); break;
    case MI_PRED:   snprintf(b, n, "%s", predOn ? "an" : "aus"); break;
    case MI_LAND:   snprintf(b, n, "%.0f m", landAlt); break;
    case MI_ZOOM:   if (mapManual) snprintf(b, n, "%.2f", manZoom); else snprintf(b, n, "auto"); break;
    case MI_PAN:    snprintf(b, n, "%d px/Impuls", panPx); break;
    case MI_TBLOCK: snprintf(b, n, "%s", tbLockMode == 0 ? "aus" : (tbLockMode == 1 ? "an" : "an (+Tastatur)")); break;
    case MI_OWN:    snprintf(b, n, "%s", ownTrailOn ? "an" : "aus"); break;
    case MI_BEEP:   if (beepSec == 0) snprintf(b, n, "aus"); else if (beepSec == 1) snprintf(b, n, "je Paket"); else snprintf(b, n, "alle %d s", beepSec); break;
    case MI_MAP: {
      static const char *const sn[NSTYLE] = {"Rad", "Luftbild", "Topo", "Relief", "TopPlusOpen"};
      snprintf(b, n, "%s%s", sn[mapStyle % NSTYLE], styleAvail[mapStyle % NSTYLE] ? "" : " (fehlt)");
      break;
    }
    case MI_BRIGHT: snprintf(b, n, "%d%%", brightPct); break;
    case MI_KB:     snprintf(b, n, "%s", kbMode == 0 ? "aus" : (kbMode == 1 ? "Taste" : "Taste+dunkel")); break;
    case MI_GPX:    snprintf(b, n, "%s", gpxOn ? "an" : "aus"); break;
    case MI_SDLOG:  snprintf(b, n, "%s", sdLogOn ? "an" : "aus"); break;
    case MI_WIFI: {
      int rq = recoveryCount();
      char suf[16] = "";
      if (rq > 0) snprintf(suf, sizeof(suf), " (%d wartend)", rq);
      if (wifiMode == 1) snprintf(b, n, "%s%s", wifiOn ? "Heimnetz OK" : (wifiConnecting ? "Heimnetz ..." : "Heimnetz"), suf);
      else if (wifiMode == 2) snprintf(b, n, "Zugangspunkt%s", suf);
      else snprintf(b, n, "aus%s", suf);
      break;
    }
    case MI_NET:
      if (wifiSsid[0]) snprintf(b, n, "%.13s", wifiSsid);
      else snprintf(b, n, "(nicht gesetzt)");
      break;
    case MI_SHUB:
      if (radioMode == 1) snprintf(b, n, "%s", loraTxOn ? "an" : "aus");   // LoRa Upload (siehe loraTxOn)
      else snprintf(b, n, "%s", sondehubMode == 0 ? "aus" : "Chase-Car");
      break;
    case MI_TXPWR:  snprintf(b, n, "%d dBm", txPowerDbm); break;
    case MI_SHUTDOWN: snprintf(b, n, "%s", shutdownConfirm ? "sicher?" : "Space x2"); break;
    default:        b[0] = 0;
  }
}

// dir = +1 / -1; big = grosser Schritt (Trackball hoch/runter). Umschalter und Listen: jeder Schritt schaltet weiter.
static void menuStep(int i, int dir, bool big) {
  if (i != MI_SHUTDOWN) shutdownConfirm = false;
  switch (i) {
    case MI_SEARCH: fixedFreq = !fixedFreq; restartRx = true; break;
    case MI_FREQ:                                  // klein 10 kHz, gross 100 kHz; rollt am Rand auf die andere Seite um
      freqMHz += (big ? 0.100f : 0.010f) * dir;
      if (freqMHz > 406.0f) freqMHz = 400.15f + (freqMHz - 406.0f);
      if (freqMHz < 400.15f) freqMHz = 406.0f - (400.15f - freqMHz);
      fixedFreq = true;                            // Frequenz aendern = feste Frequenz
      restartRx = true;
      break;
    case MI_BW: {
      int idx = 0;
      for (int k = 0; k < 7; k++) if (fabsf(bwList[k] - rxBw) < fabsf(bwList[idx] - rxBw)) idx = k;
      idx = (idx + (dir > 0 ? 1 : 6)) % 7;
      rxBw = bwList[idx];
      restartRx = true;
      break;
    }
    case MI_FIX: fixOn = !fixOn; break;
    case MI_ID:                                      // Sondenempfang <-> LoRa-APRS, siehe radioApplyMode()
      radioMode = 1 - radioMode;
      radioModeChanged = true;
      break;
    case MI_PRED: predOn = !predOn; predictLanding(); break;
    case MI_LAND:                                  // klein 10 m, gross 100 m; rollt am Rand auf die andere Seite um
      landAlt += (big ? 100.0 : 10.0) * dir;
      if (landAlt > 5000.0) landAlt = -500.0 + (landAlt - 5000.0);
      if (landAlt < -500.0) landAlt = 5000.0 - (-500.0 - landAlt);
      predictLanding();
      break;
    case MI_ZOOM: {                                  // seit FW 3.0 immer eine ganze Zoomstufe (effZoom() rundet sowieso) -- klein
                                                       // wie gross daher gleich; unter der kleinsten Stufe wieder auto
      double cur = mapManual ? manZoom : (shownZ > 0 ? shownZ : (double)MAP_IDLE_Z);
      double nz = cur + dir * 1.0;
      if (nz < (double)MAP_MIN_Z - 1e-9) {
        if (mapManual) logLine("Karte: automatisch (Zoomtaste unter Minimum)");
        mapManual = false;
      } else {
        enterManual("Zoomtaste");
        manZoom = nz > (double)MAP_MAX_Z ? (double)MAP_MAX_Z : nz;
      }
      viewZ = -1;
      break;
    }
    case MI_PAN: {                                    // klein 1 px, gross 5 px; rollt am Rand auf die andere Seite um
      int v = (int)panPx + dir * (big ? 5 : 1);
      const int lo = 1, span = 30 - 1 + 1;
      panPx = (uint8_t)(lo + ((v - lo) % span + span) % span);
      break;
    }
    case MI_TBLOCK:                                   // aus -> an -> an+Tastatur -> aus (gegen versehentliche Eingaben
                                                       // durch Vibration/Stoesse); aus Stufe 2 fuehrt NUR Taste 0 heraus,
                                                       // siehe handleInput() -- das Menue selbst (und die Weboberflaeche)
                                                       // bleiben davon unberuehrt und koennen frei weiterschalten
      tbLockMode = (uint8_t)((tbLockMode + (dir > 0 ? 1 : 2)) % 3);
      break;
    case MI_OWN:                                     // an/aus; beim Ausschalten wird der bisherige Track verworfen
      ownTrailOn = !ownTrailOn;
      if (!ownTrailOn) { ownN = 0; ownMinDist = 10.0f; }
      break;
    case MI_BEEP: {                                  // aus -> je Paket -> 5 -> 6 ... -> 20 -> aus -> ... (rollt um);
                                                       // gross: 5-Sekunden-Schritte
      int step = big ? 5 : 1, v = beepSec;
      if (dir > 0) { if (v == 0) v = 1; else if (v == 1) v = 5; else { v += step; if (v > 20) v = 0; } }
      else         { if (v == 0) v = 20; else if (v <= 1) v = 0; else { v -= step; if (v < 5) v = 1; } }
      beepSec = (uint8_t)v;
      break;
    }
    case MI_MAP: {                                   // naechste vorhandene Kartenart (Rad / Luftbild / Topo / Relief / TopPlusOpen)
      int cand = mapStyle;
      for (int k = 0; k < NSTYLE; k++) {
        cand = (cand + (dir > 0 ? 1 : NSTYLE - 1)) % NSTYLE;
        if (styleAvail[cand]) { mapStyle = cand; break; }
      }
      viewZ = -1;                                    // Hintergrund neu rendern
      break;
    }
    case MI_BRIGHT: {                                 // klein 5 %, gross 20 %; rollt am Rand auf die andere Seite um
      int v = (int)brightPct + dir * (big ? 20 : 5);
      const int lo = 10, span = 100 - 10 + 1;
      brightPct = (uint8_t)(lo + ((v - lo) % span + span) % span);
      applyBrightness();
      break;
    }
    case MI_KB:                                      // aus -> bei Tastendruck -> dauerhaft im Dunkeln
      kbMode = (kbMode + (dir > 0 ? 1 : 2)) % 3;
      kbBrightUntil = 0;                              // Modus-2-Resthelligkeitstimer verwerfen, egal wohin gewechselt wird
      if (kbMode == 0) kbOff();
      else if (kbMode == 1) {
        if (kbLit) kbSetRaw(KB_LEVEL);                 // von Modus 2 kommend ggf. noch gedimmt -- auf volle Helligkeit
        kbOffAt = millis() + KB_ON_MS;                 // frisch setzen, sonst wuerde kbTick() sofort abschalten
      }
      break;
    case MI_GPX: gpxOn = !gpxOn; break;
    case MI_SDLOG: sdLogOn = !sdLogOn; if (sdLogOn) logEnableNow(); break;
    case MI_WIFI: {                                  // aus -> Heimnetz -> Zugangspunkt -> aus
      int next = (wifiMode + (dir > 0 ? 1 : 2)) % 3;
      if (next == 1 && !wifiSsid[0]) next = (dir > 0) ? 2 : 0;       // noch kein Heimnetz gespeichert
      wifiSet(next);
      break;
    }
    case MI_NET: if (dir > 0) netSetup(); break;
    case MI_SHUB:                                     // im LoRa-Modus: LoRa Upload aus<->an; sonst wie bisher aus<->Chase-Car
      if (radioMode == 1) {
        loraTxOn = !loraTxOn;
        if (loraTxOn) { aprsBeaconNextMs = 0; aprsBeaconLastSet = false; }   // gleich die erste eigene Position senden, Wegstrecke-Anker neu
      } else {
        sondehubMode = (uint8_t)((sondehubMode + 1) % 2);
        if (sondehubMode == 1) sondehubListenerNextMs = 0;   // gleich die erste Chase-Car-Position senden, nicht erst nach 2 Min.
      }
      break;
    case MI_TXPWR:                                    // klein 1 dBm, gross 5 dBm; SX1262-Bereich -9..+22 dBm,
                                                        // angewendet in aprsSendBeacon() vor jedem Senden
      txPowerDbm = (int8_t)constrain((int)txPowerDbm + (big ? 5 : 1) * dir, -9, 22);
      break;
    case MI_SHUTDOWN:                                 // zweimal weiter (Space) schaltet wirklich ab
      if (dir > 0) {
        if (shutdownConfirm && millis() - shutdownConfirmMs < 4000) { doShutdown(); }
        else { shutdownConfirm = true; shutdownConfirmMs = millis(); }
      } else {
        shutdownConfirm = false;
      }
      break;
  }
  saveSettings();
}

// Getippten Wert uebernehmen; false = ungueltig (dann bleibt der alte Wert)
static bool menuApplyTyped(int i) {
  if (editLen <= 0) return false;
  editBuf[editLen] = 0;
  switch (i) {
    case MI_FREQ: {
      double mhz;
      if (strchr(editBuf, '.') || strchr(editBuf, ',')) {
        char t[12];
        strncpy(t, editBuf, sizeof(t) - 1);
        t[sizeof(t) - 1] = 0;
        for (char *c = t; *c; c++) if (*c == ',') *c = '.';
        mhz = atof(t);
      } else {                                     // Ziffern: Komma nach der dritten Stelle (4053 -> 405.3, 405300 -> 405.300)
        if (editLen < 3) return false;
        char t[16];
        snprintf(t, sizeof(t), "%.3s.%s", editBuf, editBuf + 3);
        mhz = atof(t);
      }
      if (mhz < 400.15 || mhz > 406.0) return false;
      freqMHz = (float)mhz;
      fixedFreq = true;
      restartRx = true;
      return true;
    }
    case MI_LAND:
      landAlt = constrain(atof(editBuf), -500.0, 5000.0);
      predictLanding();
      return true;
    case MI_ZOOM: {                                  // z. B. 14.5; 0 = automatisch
      char t[12];
      strncpy(t, editBuf, sizeof(t) - 1);
      t[sizeof(t) - 1] = 0;
      for (char *c = t; *c; c++) if (*c == ',') *c = '.';
      double z = atof(t);
      if (z < 1.0) mapManual = false;
      else {
        enterManual("Zoom-Eingabe");
        manZoom = z < (double)MAP_MIN_Z ? (double)MAP_MIN_Z : (z > (double)MAP_MAX_Z ? (double)MAP_MAX_Z : z);
      }
      viewZ = -1;
      return true;
    }
    case MI_PAN:
      panPx = (uint8_t)constrain(atoi(editBuf), 1, 30);
      return true;
    case MI_BEEP: {                                  // 0 aus, 1 je Paket, 2..4 wird auf 5 angehoben, 5..20 Sekunden, darueber 20
      int v = atoi(editBuf);
      beepSec = (uint8_t)((v <= 0) ? 0 : (v == 1) ? 1 : (v < 5) ? 5 : (v > 20) ? 20 : v);
      return true;
    }
    case MI_BRIGHT: {
      int pct = atoi(editBuf);
      brightPct = (uint8_t)constrain(pct, 10, 100);
      applyBrightness();
      return true;
    }
    case MI_TXPWR:
      txPowerDbm = (int8_t)constrain(atoi(editBuf), -9, 22);
      return true;
  }
  return false;
}

// Taste R in der Kartenansicht: war die Karte manuell verschoben, erst nur zurueck zur Mitte (Modus bleibt);
// war sie das schon nicht (voriges R hat schon zentriert, seitdem nicht neu verschoben), naechster Modus
// Sonde+Ich+Landung -> Sonde+Ich -> Sonde -> Ich -> ... Dadurch zykelt JEDER weitere Druck zuverlaessig
// weiter, egal wie schnell/langsam getippt wird. Ausserhalb der Kartenansicht (oder ohne Bildpuffer) passiert
// nichts -- das deckt sich mit der Taste R am Geraet, die dort stattdessen das Menue "Trackball-Sperre"
// oeffnet (siehe menuKey[]); das von hier aus nachzubilden wuerde das blockierende Menue-UI simulieren
// muessen und ergibt ferngesteuert keinen Sinn, deshalb nur die Kartenfunktion. Auch von der Weboberflaeche
// (/vfocus) genutzt.
static void focusCycleOrRecenter() {
  if (!(mapMode && gfxView == 0 && gfxReady())) return;
  bool wasManual = mapManual;
  mapManual = false;
  if (radioMode == 1) {                          // LoRa-APRS-Modus: eigener Fokus statt rFocus (keine Sonde vorhanden)
    if (aprsSoloIdx >= 0) {                      // Einzelstations-Fokus aktiv (aus der Stationsliste, siehe
                                                  // handleInput()) -- R auf der Karte verlaesst ihn wieder,
                                                  // statt den normalen aprsFocus-Zyklus fortzusetzen
      aprsSoloIdx = -1;
      aprsFocus = 0;
      if (soloZoom >= (double)MAP_MIN_Z) viewZ = soloZoom;
      toast("Focus", "Ich");
      return;
    }
    if (!wasManual) aprsFocus = (aprsFocus + 1) % 3;
    if (aprsFocus != 0) viewZ = -1;              // Ich+Stationen: neu einzoomen, damit alle Punkte passen
    else if (soloZoom >= (double)MAP_MIN_Z) viewZ = soloZoom;   // zurueck zum zuletzt fuer "nur Ich" genutzten Zoom
    static const char *const aname[3] = {"Ich", "Ich+alle Stationen", "Ich+direkt gehoert"};
    toast("Focus", aname[aprsFocus]);
    return;
  }
  if (!wasManual) rFocus = (rFocus + 1) % 4;
  bool multiPoint = (rFocus == 0 || rFocus == 1);
  if (multiPoint && haveSondePos) {
    viewZ = -1;                                 // Sonde+Ich(+Landung) mit echter Sonde: neu einzoomen, damit beide passen
  } else {
    if (soloZoom >= (double)MAP_MIN_Z) viewZ = soloZoom;   // zurueck zum zuletzt fuer Einzelpunkt genutzten Zoom
    forceRecenter = true;                       // Zoom bleibt (bzw. wird wiederhergestellt), nur neu zentrieren
  }
  static const char *const fname[4] = {"Sonde+Ich+Landung", "Sonde+Ich", "Sonde", "Ich"};
  toast("Focus", fname[rFocus]);
}

// Anzahl der Auswahlmoeglichkeiten eines Listen-Menuepunkts -- nur fuer die Punkte, die auf der
// Weboberflaeche als Dropdown statt als "-"/"+"-Schrittweiter erscheinen sollen (0 = kein solcher Punkt,
// bleibt dann bei "-"/"+").
static int menuOptionCount(int i) {
  switch (i) {
    case MI_SEARCH: case MI_FIX: case MI_PRED: case MI_OWN: case MI_GPX: case MI_SDLOG: case MI_SHUB: case MI_ID: return 2;
    case MI_TBLOCK: case MI_KB: return 3;
    case MI_BW: return 7;
    case MI_MAP: return NSTYLE;
  }
  return 0;
}

// Anzeigetext der Option 'idx' (0-basiert) -- fuer den Dropdown-Text und (ueber menuCurrentIndex())
// die aktuell ausgewaehlte Option.
static void menuOptionLabel(int i, int idx, char *b, size_t n) {
  switch (i) {
    case MI_SEARCH: snprintf(b, n, "%s", idx == 0 ? "Auto" : "fest"); break;
    case MI_BW:     snprintf(b, n, "%.1f kHz", bwList[idx]); break;
    case MI_FIX: case MI_PRED: case MI_OWN: case MI_GPX: case MI_SDLOG:
      snprintf(b, n, "%s", idx == 0 ? "aus" : "an"); break;
    case MI_SHUB:   snprintf(b, n, "%s", radioMode == 1 ? (idx == 0 ? "aus" : "an") : (idx == 0 ? "aus" : "Chase-Car")); break;
    case MI_ID:     snprintf(b, n, "%s", idx == 0 ? "Sondenempfang" : "LoRa"); break;
    case MI_TBLOCK: snprintf(b, n, "%s", idx == 0 ? "aus" : (idx == 1 ? "an" : "an (+Tastatur)")); break;
    case MI_MAP: {
      static const char *const sn[NSTYLE] = {"Rad", "Luftbild", "Topo", "Relief", "TopPlusOpen"};
      snprintf(b, n, "%s%s", sn[idx], styleAvail[idx] ? "" : " (fehlt)");
      break;
    }
    case MI_KB: snprintf(b, n, "%s", idx == 0 ? "aus" : (idx == 1 ? "Taste" : "Taste+dunkel")); break;
    default: b[0] = 0;
  }
}

// Aktuell gewaehlte Option (0-basiert), fuer "selected" im Dropdown.
static int menuCurrentIndex(int i) {
  switch (i) {
    case MI_SEARCH: return fixedFreq ? 1 : 0;
    case MI_BW: {
      int idx = 0;
      for (int k = 0; k < 7; k++) if (fabsf(bwList[k] - rxBw) < fabsf(bwList[idx] - rxBw)) idx = k;
      return idx;
    }
    case MI_FIX:    return fixOn ? 1 : 0;
    case MI_PRED:   return predOn ? 1 : 0;
    case MI_OWN:    return ownTrailOn ? 1 : 0;
    case MI_GPX:    return gpxOn ? 1 : 0;
    case MI_SDLOG:  return sdLogOn ? 1 : 0;
    case MI_SHUB:   return radioMode == 1 ? (loraTxOn ? 1 : 0) : sondehubMode;
    case MI_ID:     return radioMode;
    case MI_TBLOCK: return tbLockMode;
    case MI_MAP:    return mapStyle % NSTYLE;
    case MI_KB:     return kbMode;
  }
  return 0;
}

// Direktes Setzen auf eine Option (0-basiert) -- fuer das Dropdown der Weboberflaeche. Spiegelt dieselben
// Seiteneffekte wie der jeweilige Schritt in menuStep().
static void menuSetIndex(int i, int idx) {
  switch (i) {
    case MI_SEARCH: fixedFreq = (idx != 0); restartRx = true; break;
    case MI_BW: rxBw = bwList[constrain(idx, 0, 6)]; restartRx = true; break;
    case MI_FIX: fixOn = (idx != 0); break;
    case MI_PRED: predOn = (idx != 0); predictLanding(); break;
    case MI_OWN:
      ownTrailOn = (idx != 0);
      if (!ownTrailOn) { ownN = 0; ownMinDist = 10.0f; }
      break;
    case MI_GPX: gpxOn = (idx != 0); break;
    case MI_SDLOG: sdLogOn = (idx != 0); if (sdLogOn) logEnableNow(); break;
    case MI_SHUB:
      if (radioMode == 1) {
        loraTxOn = (idx != 0);
        if (loraTxOn) { aprsBeaconNextMs = 0; aprsBeaconLastSet = false; }
      } else {
        sondehubMode = (uint8_t)(idx != 0 ? 1 : 0);
        if (sondehubMode == 1) sondehubListenerNextMs = 0;
      }
      break;
    case MI_ID:
      radioMode = (uint8_t)(idx != 0 ? 1 : 0);
      radioModeChanged = true;
      break;
    case MI_TBLOCK: tbLockMode = (uint8_t)constrain(idx, 0, 2); break;
    case MI_MAP: mapStyle = constrain(idx, 0, NSTYLE - 1); viewZ = -1; break;
    case MI_KB:
      kbMode = (uint8_t)constrain(idx, 0, 2);
      kbBrightUntil = 0;
      if (kbMode == 0) kbOff();
      else if (kbMode == 1) {
        if (kbLit) kbSetRaw(KB_LEVEL);
        kbOffAt = millis() + KB_ON_MS;
      }
      break;
  }
  saveSettings();
}

// ---------- Fernsteuerung der Menueoptionen (Weboberflaeche, /menu) ----------
// Jeder Menuepunkt ausser "Herunterfahren" (Sicherheit) und "Netz" (das Eintippen von SSID/Passwort
// blockiert das Geraet auf die eingebaute Tastatur, geht von der Weboberflaeche aus also nicht) laesst
// sich hier genauso verstellen wie am Geraet: "-"/"+" entspricht Trackball links/rechts, bei den
// Zahlenfeldern zusaetzlich "<<"/">>" fuer den groesseren Schritt (Trackball hoch/runter) sowie ein
// Eingabefeld fuer einen genauen Wert (wie das Eintippen am Geraet selbst).
// MI_WIFI zusaetzlich zu MI_SHUTDOWN/MI_NET ausgenommen: wifiSet() (siehe menuStep()) reisst dabei per
// wifiStopAll() das WLAN (und damit genau die Verbindung, ueber die der Browser gerade diese Anfrage
// stellt) komplett ein -- von der Weboberflaeche aus waere das der Ast, auf dem man sitzt.
static bool menuWebSkip(int i) { return i == MI_SHUTDOWN || i == MI_NET || i == MI_WIFI; }

// Menuepunkt-Name fuer die Weboberflaeche, mit echten Umlauten als HTML-Entity (menuName[] selbst bleibt
// ASCII, weil es unveraendert auch die Geraete-Anzeige beschriftet).
static String menuNameHtml(int i) { return i == MI_LAND ? String("Landeh&ouml;he") : String(menuDisplayName(i)); }

static void webMenu() {
  char lm[64];
  snprintf(lm, sizeof(lm), "WLAN: Anfrage GET /menu von %s", server.client().remoteIP().toString().c_str());
  logLine(lm);
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent("<!doctype html><html><head><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\">"
                     "<link rel=icon href=\"/favicon.ico\"><link rel=apple-touch-icon href=\"/apple-touch-icon.png\">"
                     "<title>T-Deck RS41 -- Men&uuml;</title><style>body{font-family:sans-serif;margin:12px auto;"
                     "max-width:760px;background:#000;color:#fff}"
                     "table{border-collapse:collapse;width:100%}a{color:#6cf}"
                     "td{padding:4px 8px;border-bottom:1px solid #444;vertical-align:middle;text-align:left}"
                     "img{max-width:100%;border:1px solid #444;image-rendering:pixelated}"
                     ".b{display:inline-block;padding:3px 10px;margin:1px;border:1px solid #6cf;border-radius:4px;"
                     "text-decoration:none;font-size:16px}"
                     "input[type=text],select{width:auto;max-width:120px;text-align:left}form{display:inline}"
                     ".row{display:flex;flex-wrap:wrap;gap:16px;align-items:flex-start}"
                     ".row>div{flex:1 1 320px;min-width:300px}"
                     "#altc{max-width:100%;background:#111;border:1px solid #444}"
                     "#altlist table{width:auto;font-size:12px}#altlist td,#altlist th{padding:1px 6px;border-bottom:1px solid #444}"
                     "@media (max-width:600px){table,tbody,tr,td{display:block;width:100%}"
                     "tr{margin-bottom:10px;border-bottom:2px solid #555}td{border-bottom:none}}"
                     "</style></head><body>");
  // altTitle/altc richten sich schon beim ersten Ausliefern (nicht erst nachtraeglich per JS, siehe loadAlt())
  // nach dem AKTUELLEN radioMode: im LoRa-Modus sofort "LoRa-APRS-Stationen" + Canvas versteckt (display:none),
  // damit der (dort ungenutzte) Hoehentrack beim Laden nicht einmal kurz aufblitzt, sondern sofort die
  // Stationsliste (#altlist, von loadAlt() befuellt) an seiner Stelle steht. Canvas-Groesse 320x240 = genau
  // die Aufloesung des T-Deck-Displays (loadAlt() liest c.width/c.height dynamisch, skaliert also automatisch mit).
  server.sendContent(String("<p><a href=\"/\">&larr; Zur Startseite</a></p>"
                     "<div class=row>"
                     "<div><h2>Aktuelles Bild</h2><p><img id=live src=\"/live.jpg?t=") + String(millis()) +
                     "\" alt=\"Live-Bild\"></p></div>"
                     "<div><h2 id=altTitle>" + (radioMode == 1 ? "LoRa-APRS-Stationen" : "Hoehentrack") + "</h2>"
                     "<canvas id=altc width=320 height=240" + (radioMode == 1 ? " style=\"display:none\"" : "") + "></canvas>"
                     "<div id=altlist></div></div>"
                     "</div>"
                     "<p>Automatisch aktualisieren: "
                     "<select id=refint onchange=\"setRefresh(this.value)\">"
                     "<option value=0>aus</option><option value=30 selected>alle 30 s</option>"
                     "<option value=60>alle 60 s</option></select></p>"
                     "<p><a class=b href=\"/vspace\">Ansicht wechseln (Space)</a> "
                     "<a class=b href=\"/vfocus\">Zentrieren/Fokus (R)</a></p>"
                     // Bewusst echte UTF-8-/reine ASCII-JS ohne HTML-Entities (wirken in <script> nicht, siehe
                     // webIndex()-Kommentar). pollStat() fragt alle 5 s /mstat ab und piept bei Aenderung von
                     // "seq" (= neuer Sondenfund, siehe sondeFoundSeq) ueber die Web Audio API -- unabhaengig
                     // vom gewaehlten Bild-Aktualisierungsintervall. loadAlt() holt dasselbe Hoehentrack/
                     // Stationsfeld wie die Geraete-Ansicht (siehe altTrackUpdate()/webAltStat()) und laeuft
                     // im selben Takt wie loadLive() (siehe setRefresh()).
                     "<script>"
                     "var refTimer=null,lastSeq=null;"
                     "function loadLive(){document.getElementById('live').src='/live.jpg?t='+Date.now();}"
                     "function loadAlt(){fetch('/malt').then(function(r){return r.json();}).then(function(d){"
                     "var c=document.getElementById('altc'),ctx=c.getContext('2d'),"
                     "t=document.getElementById('altTitle'),lst=document.getElementById('altlist');"
                     "ctx.clearRect(0,0,c.width,c.height);"
                     "if(d.mode==1){"
                     "t.textContent='LoRa-APRS-Stationen';c.style.display='none';"
                     // Rohempfangs-Diagnose zuerst (siehe aprsRxTotal/aprsRxOk in aprsPoll()): zeigt auch an,
                     // wenn ueberhaupt Pakete ankommen, aber keine Station daraus wird (Parameter pruefen).
                     "var h='<p style=\"color:#888\">Pakete: '+(d.rxtot||0)+' ('+(d.rxok||0)+' ok)'+"
                     "((d.rxtot)?(', letztes vor '+d.rxage+'s, '+d.rssi+' dBm'):'')+'</p>';"
                     "if(!d.st||!d.st.length)h+='<p>Noch keine Station gehoert</p>';"
                     "else{h+='<table><tr><th>Rufzeichen</th><th>km</th><th>zuletzt</th></tr>';"
                     "d.st.forEach(function(s){var a=s.age,as=a<3600?"
                     "(Math.floor(a/60)+':'+String(a%60).padStart(2,'0')):(Math.floor(a/3600)+'h');"
                     "h+='<tr><td>'+s.c+'</td><td>'+s.km.toFixed(1)+'</td><td>'+as+'</td></tr>';});"
                     "h+='</table>';}"
                     "lst.innerHTML=h;"
                     "}else{"
                     "t.textContent='Hoehentrack';lst.innerHTML='';c.style.display='';"
                     "var alt=d.alt||[];"
                     // Die Werte sind bereits auf die Chartbreite herunterinterpoliert (siehe webAltStat()),
                     // decken also immer die GESAMTE gespeicherte Kurve ab (erstes bis letztes Paket) -- der
                     // Index ist daher schon die X-Achse. amin/amax kommen vom Server (echtes Minimum/Maximum
                     // des Rohpuffers, nicht nur der hier gezeigten Stuetzstellen) -- die Flaeche wird bis
                     // dorthin (amin = unterer Rand) gefuellt, fuer bessere Ablesbarkeit.
                     "if(alt.length<2){ctx.fillStyle='#888';ctx.font='14px sans-serif';"
                     "ctx.fillText(d.have?'Sammle Daten...':'Keine Sonde empfangen',10,30);"
                     "}else{"
                     "var aMin=(d.amin!==undefined)?d.amin:Math.min.apply(null,alt);"
                     "var aMax=(d.amax!==undefined)?d.amax:Math.max.apply(null,alt);"
                     "if(aMax-aMin<10)aMax=aMin+10;"
                     "var x0=10,x1=c.width-10,y0=10,y1=c.height-10,nMax=alt.length-1;"
                     "ctx.fillStyle='#145e5e';"
                     "for(var i=0;i<alt.length;i++){"
                     "var x=x0+(x1-x0)*i/nMax,y=y1-(y1-y0)*(alt[i]-aMin)/(aMax-aMin);"
                     "ctx.fillRect(x-1,y,2,y1-y);}"
                     "ctx.strokeStyle='#ff0';ctx.lineWidth=2;ctx.beginPath();"
                     "for(var i=0;i<alt.length;i++){"
                     "var x=x0+(x1-x0)*i/nMax,y=y1-(y1-y0)*(alt[i]-aMin)/(aMax-aMin);"
                     "if(i==0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}"
                     "ctx.stroke();"
                     "ctx.fillStyle='#fff';ctx.font='12px sans-serif';ctx.fillText(d.cur+' m',10,c.height-2);"
                     "}}}).catch(function(){});}"
                     "function setRefresh(s){if(refTimer){clearInterval(refTimer);refTimer=null;}s=parseInt(s);"
                     "if(s>0)refTimer=setInterval(function(){loadLive();loadAlt();},s*1000);}"
                     "function beep(){try{var ctx=new (window.AudioContext||window.webkitAudioContext)();"
                     "function tone(t){var o=ctx.createOscillator(),g=ctx.createGain();o.frequency.value=880;"
                     "o.connect(g);g.connect(ctx.destination);g.gain.value=0.2;"
                     "o.start(ctx.currentTime+t);o.stop(ctx.currentTime+t+0.18);}"
                     "tone(0);tone(0.25);setTimeout(function(){ctx.close();},600);}catch(e){}}"
                     "function pollStat(){fetch('/mstat').then(function(r){return r.json();}).then(function(d){"
                     "if(lastSeq!==null&&d.seq!==lastSeq)beep();lastSeq=d.seq;}).catch(function(){});}"
                     "setRefresh(30);loadAlt();setInterval(pollStat,5000);pollStat();"
                     "</script>"
                     "<h2>Men&uuml;</h2><table>");
  for (int i = 0; i < MI_COUNT; i++) {
    if (menuWebSkip(i)) continue;
    char val[24];
    menuValue(i, val, sizeof(val));
    String row = "<tr><td>" + menuNameHtml(i) + "</td><td>" + String(val) + "</td><td>";
    String qs = "i=" + String(i);
    int nOpt = menuOptionCount(i);
    if (menuIsNumeric(i)) {
      row += "<form method=get action=\"/mset\">"
             "<input type=hidden name=i value=\"" + String(i) + "\">"
             "<input type=text name=v placeholder=\"Wert\">"
             "<button type=submit>Setzen</button></form> "
             "<a class=b href=\"/mstep?" + qs + "&dir=-1&big=1\">&laquo;</a>"
             "<a class=b href=\"/mstep?" + qs + "&dir=-1&big=0\">-</a>"
             "<a class=b href=\"/mstep?" + qs + "&dir=1&big=0\">+</a>"
             "<a class=b href=\"/mstep?" + qs + "&dir=1&big=1\">&raquo;</a>";
    } else if (nOpt > 0) {
      int cur = menuCurrentIndex(i);
      row += "<select onchange=\"location.href='/mselect?" + qs + "&idx='+this.value\">";
      for (int k = 0; k < nOpt; k++) {
        char lbl[24];
        menuOptionLabel(i, k, lbl, sizeof(lbl));
        row += "<option value=\"" + String(k) + "\"" + (k == cur ? " selected" : "") + ">" + String(lbl) + "</option>";
      }
      row += "</select>";
    } else {
      row += "<a class=b href=\"/mstep?" + qs + "&dir=-1&big=0\">-</a>"
             "<a class=b href=\"/mstep?" + qs + "&dir=1&big=0\">+</a>";
    }
    row += "</td></tr>";
    server.sendContent(row);
  }
  server.sendContent(String("</table><p style=\"color:#666;font-size:12px;margin-top:20px\">T-Deck RS41 v") + FW_VERSION + "</p></body></html>");
  server.sendContent("");
}

// "-"/"+" (dir -1/+1), bei Zahlenfeldern zusaetzlich big=1 fuer den groesseren Schritt -- entspricht
// genau menuStep() am Geraet (Trackball links/rechts bzw. hoch/runter).
static void webMenuStep() {
  int i = server.arg("i").toInt();
  int dir = server.arg("dir").toInt();
  bool big = server.arg("big").toInt() != 0;
  if (i >= 0 && i < MI_COUNT && !menuWebSkip(i) && dir != 0) {
    menuStep(i, dir > 0 ? 1 : -1, big);               // ruft am Ende selbst schon saveSettings() auf
    char m[96];
    snprintf(m, sizeof(m), "WLAN: Menue \"%s\" per Weboberflaeche geaendert (%s%s) von %s", menuDisplayName(i),
             dir > 0 ? "+" : "-", big ? " gross" : "", server.client().remoteIP().toString().c_str());
    logLine(m);
  }
  server.sendHeader("Location", "/menu");
  server.send(303, "text/plain", "");
}

// Genauer Wert fuer ein Zahlenfeld, wie das Eintippen am Geraet (editBuf/editLen werden nur kurz fuer
// menuApplyTyped() benutzt, 'editing' bleibt unberuehrt -- die Geraete-Anzeige zeigt deshalb keinen
// Eingabecursor von dieser Web-Aenderung).
static void webMenuSet() {
  int i = server.arg("i").toInt();
  String v = server.arg("v");
  if (i >= 0 && i < MI_COUNT && !menuWebSkip(i) && menuIsNumeric(i) && v.length() > 0) {
    strncpy(editBuf, v.c_str(), sizeof(editBuf) - 1);
    editBuf[sizeof(editBuf) - 1] = 0;
    editLen = (int)strlen(editBuf);
    if (menuApplyTyped(i)) {
      saveSettings();
      char m[104];
      snprintf(m, sizeof(m), "WLAN: Menue \"%s\" per Weboberflaeche auf \"%s\" gesetzt von %s", menuDisplayName(i), editBuf,
               server.client().remoteIP().toString().c_str());
      logLine(m);
    }
    editLen = 0;
  }
  server.sendHeader("Location", "/menu");
  server.send(303, "text/plain", "");
}

// Aktueller Bildschirminhalt als JPEG, direkt aus dem Bildpuffer kodiert (keine SD-Karte noetig) --
// fuer das Live-Bild auf /menu: zeigt beim (Neu-)Laden der Seite genau das, was gerade auf dem
// T-Deck-Display steht.
static void webLive() {
  if (!frameCanvas) { server.send(503, "text/plain", "kein Bildpuffer"); return; }
  const size_t cap = 96 * 1024;
  uint8_t *out = (uint8_t *)ps_malloc(cap);
  if (!out) out = (uint8_t *)malloc(cap);
  if (!out) { server.send(503, "text/plain", "kein Speicher"); return; }
  int size = encodeFrameJpeg(out, cap);
  if (size <= 0) { free(out); server.send(500, "text/plain", "JPEG-Fehler"); return; }
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(size);
  server.send(200, "image/jpeg", "");
  server.client().write(out, size);
  free(out);
}

// Winziger JSON-Status fuer die Weboberflaeche (/menu) -- der Browser pollt das in kurzen Abstaenden und
// erkennt an einer Aenderung von "seq" einen neuen Sondenfund (siehe sondeFoundSeq), um dann selbst einen
// Ton auszugeben (Web Audio API, siehe webMenu()). Bewusst kein grosses JSON, das soll billig genug sein,
// um es alle paar Sekunden abzufragen, ohne die Bildabfrage (/live.jpg) zu belasten.
static void webMenuStat() {
  char buf[40];
  snprintf(buf, sizeof(buf), "{\"seq\":%lu}", (unsigned long)sondeFoundSeq);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", buf);
}

// Daten fuer das Hoehentrack-/Stationen-Feld neben dem Live-Bild auf /menu (siehe webMenu()):
// im Sondenmodus die Hoehenkurve, auf eine feste Stuetzstellenzahl (WEB_PTS) herunterinterpoliert, damit
// die Antwort klein bleibt -- genau wie beim Zeichnen am Geraet (altTrackUpdate()) deckt sie IMMER die
// gesamte gespeicherte Kurve (erstes bis letztes Paket) ab; der Rohpuffer altTrackAlt/altTrackMs selbst
// wird dabei nicht veraendert. Im LoRa-Modus stattdessen die gehoerten APRS-Stationen mit Entfernung/Alter.
static void webAltStat() {
  String j = "{\"mode\":";
  j += (radioMode == 1) ? "1" : "0";
  if (radioMode == 1) {
    j += ",\"rxtot\":" + String((unsigned long)aprsRxTotal);
    j += ",\"rxok\":" + String((unsigned long)aprsRxOk);
    j += ",\"rxage\":" + String(aprsRxTotal ? (unsigned long)((millis() - aprsLastRxMs) / 1000) : 0);
    j += ",\"rssi\":" + String(aprsLastRssi, 0);
    j += ",\"st\":[";
    double hLat, hLon;
    homePosition(hLat, hLon);
    // Gleiche Sortierung nach zuletzt gehoert wie altTrackUpdate() am Geraet (siehe dort), damit die
    // Webtabelle genauso "nach oben wandert" statt wie ein fester Array-Index stehenzubleiben.
    int idx[APRS_MAX];
    for (int i = 0; i < aprsN; i++) idx[i] = i;
    for (int i = 1; i < aprsN; i++) {
      int v = idx[i], k = i - 1;
      while (k >= 0 && aprsSeenMs[idx[k]] < aprsSeenMs[v]) { idx[k + 1] = idx[k]; k--; }
      idx[k + 1] = v;
    }
    for (int k = 0; k < aprsN; k++) {
      int i = idx[k];
      double d = haversineKm(hLat, hLon, aprsLat[i], aprsLon[i]);
      uint32_t ageS = (millis() - aprsSeenMs[i]) / 1000;
      if (k) j += ",";
      j += "{\"c\":\"" + String(aprsCall[i]) + "\",\"km\":" + String(d, 1) + ",\"age\":" + String((unsigned long)ageS) + "}";
    }
    j += "]}";
  } else {
    j += ",\"have\":"; j += haveSondePos ? "1" : "0";
    j += ",\"cur\":" + String(sondeAlt, 0);
    if (altTrackAlt && altTrackN >= 2) {
      float aMin = altTrackAlt[0], aMax = altTrackAlt[0];
      for (int i = 1; i < altTrackN; i++) {
        if (altTrackAlt[i] < aMin) aMin = altTrackAlt[i];
        if (altTrackAlt[i] > aMax) aMax = altTrackAlt[i];
      }
      j += ",\"amin\":" + String(aMin, 0) + ",\"amax\":" + String(aMax, 0);
      const int WEB_PTS = 300;
      uint32_t tMin = altTrackMs[0], tMax = altTrackMs[altTrackN - 1];
      if (tMax <= tMin) tMax = tMin + 1;
      j += ",\"alt\":[";
      int k = 0;
      for (int p = 0; p < WEB_PTS; p++) {
        uint32_t t = tMin + (uint32_t)((double)(tMax - tMin) * p / (WEB_PTS - 1));
        while (k < altTrackN - 2 && altTrackMs[k + 1] < t) k++;
        uint32_t t0 = altTrackMs[k], t1 = altTrackMs[k + 1];
        float frac = (t1 > t0) ? (float)(t - t0) / (float)(t1 - t0) : 0.0f;
        float a = altTrackAlt[k] + (altTrackAlt[k + 1] - altTrackAlt[k]) * frac;
        if (p) j += ",";
        j += String(a, 0);
      }
      j += "]";
    } else {
      j += ",\"alt\":[]";
    }
    j += "}";
  }
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

// Direktes Setzen einer Auswahlliste (Dropdown auf /menu) -- siehe menuSetIndex()/menuOptionCount().
static void webMenuSelect() {
  int i = server.arg("i").toInt();
  int idx = server.arg("idx").toInt();
  if (i >= 0 && i < MI_COUNT && !menuWebSkip(i) && idx >= 0 && idx < menuOptionCount(i)) {
    menuSetIndex(i, idx);
    char m[96];
    snprintf(m, sizeof(m), "WLAN: Menue \"%s\" per Weboberflaeche auf Option %d gesetzt von %s", menuDisplayName(i), idx,
             server.client().remoteIP().toString().c_str());
    logLine(m);
  }
  server.sendHeader("Location", "/menu");
  server.send(303, "text/plain", "");
}

// Taste Space/Trackball-Klick (Ansicht wechseln Text -> Karte -> Hoehentrack/Stationen) von der Weboberflaeche aus.
static void webViewSpace() {
  cycleView();
  char m[64];
  snprintf(m, sizeof(m), "WLAN: Ansicht per Weboberflaeche gewechselt von %s", server.client().remoteIP().toString().c_str());
  logLine(m);
  server.sendHeader("Location", "/menu");
  server.send(303, "text/plain", "");
}

// Taste R (Zentrieren/naechster Fokus in der Kartenansicht) von der Weboberflaeche aus -- siehe
// focusCycleOrRecenter() fuer die Einschraenkung ausserhalb der Kartenansicht.
static void webViewFocus() {
  focusCycleOrRecenter();
  char m[64];
  snprintf(m, sizeof(m), "WLAN: Fokus/Zentrieren per Weboberflaeche von %s", server.client().remoteIP().toString().c_str());
  logLine(m);
  server.sendHeader("Location", "/menu");
  server.send(303, "text/plain", "");
}

static void drawMenu(int sel) {
  Adafruit_GFX *g = scr();
  g->fillScreen(ST77XX_BLACK);
  g->setTextSize(1);                              // kleine Schrift: MI_COUNT Zeilen zu je 11 Pixeln
  const int rowH = 10;                            // MI_COUNT Zeilen a 10 px + Fusszeile muessen auf 240 px passen
  for (int i = 0; i < MI_COUNT; i++) {
    int y = 1 + i * rowH;
    char v[24], nm[24];
    if (i == sel && editing) snprintf(v, sizeof(v), "%.*s_", editLen, editBuf);
    else menuValue(i, v, sizeof(v));
    snprintf(nm, sizeof(nm), "[%c] %s", toupper(menuKey[i]), menuDisplayName(i));
    if (i == sel) {
      g->fillRect(0, y - 1, (y < 25) ? 262 : 320, rowH, ST77XX_CYAN);   // oben rechts bleibt Platz fuer den Statusblock
      g->setTextColor(ST77XX_BLACK);
    } else {
      g->setTextColor((i == MI_FREQ && !fixedFreq) ? 0x8410 : ST77XX_WHITE);
    }
    g->setCursor(4, y + 1);
    g->print(nm);
    g->setCursor(136, y + 1);                     // Werte in einer Spalte (Abstand fuer laengste Namen,
                                                   // z. B. "[R] Trackball-Sperre" = 21 Zeichen = 126 px bei 6 px/Zeichen)
    g->print(v);
  }
  drawStatus(g, 264, 3);                          // nach den Zeilen zeichnen, damit die Auswahlleiste ihn nicht ueberdeckt
  g->setTextColor(0x8410);                        // Firmware-Version dauerhaft im Menuekopf (vorher nur kurz im
  g->setCursor(264, 27);                           // Bootsplash zu sehen) -- Luecke zwischen Statusblock (y 3..23)
  g->print("v");                                   // und der naechsten Zeile, die diese Spalte nutzt (Item 3+);
  g->print(FW_VERSION);                            // wird als letztes gezeichnet, bleibt also auch sichtbar, wenn
                                                     // die Auswahlleiste eines davor liegenden Punkts darueberlaeuft
  g->setTextSize(1);
  const int footY = 1 + MI_COUNT * rowH + 2;      // direkt unter der letzten Zeile, ueberlappt nie
  g->fillRect(0, footY - 1, 320, 240 - footY, ST77XX_BLACK);
  g->setCursor(4, footY);
  if (menuNoteUntil && millis() < menuNoteUntil) {
    g->setTextColor(ST77XX_YELLOW);
    g->print(menuNote);
  } else if (sel == MI_WIFI && wifiOn && wifiMode == 1) {
    g->setTextColor(ST77XX_CYAN);
    g->print("http://");
    g->print(WiFi.localIP().toString());
    g->print("  oder  " WIFI_HOST ".local");
  } else if (sel == MI_WIFI && wifiOn && wifiMode == 2) {
    g->setTextColor(ST77XX_CYAN);
    g->print("SSID " WIFI_SSID "  PW " WIFI_PASS "  http://192.168.4.1");
  } else if (sel == MI_NET) {
    g->setTextColor(0x8410);
    g->print("Space/Klick: Heimnetz-Name und Passwort eingeben");
  } else if (sel == MI_FREQ) {
    g->setTextColor(0x8410);
    char fb[64];
    if (lastScanFreq > 0 && lastRxFreq > 0) snprintf(fb, sizeof(fb), "Scan %.3f  Sonde %.3f (F: uebernehmen)", lastScanFreq, lastRxFreq);
    else if (lastRxFreq > 0) snprintf(fb, sizeof(fb), "letzte Sonde %.3f MHz (F: uebernehmen)", lastRxFreq);
    else if (lastScanFreq > 0) snprintf(fb, sizeof(fb), "letzter Scan %.3f MHz, noch keine Sonde", lastScanFreq);
    else snprintf(fb, sizeof(fb), "noch kein Scan/Empfang in dieser Sitzung");
    g->print(fb);
  } else if (sel == MI_ZOOM) {
    g->setTextColor(0x8410);
    g->print("Wert z.B. 14.5   0 = automatisch   (Karte: A/D)");
  } else if (sel == MI_PAN) {
    g->setTextColor(0x8410);
    g->print("Kartenpixel je Trackball-Impuls, 1..30");
  } else if (sel == MI_BEEP) {
    g->setTextColor(0x8410);
    g->print("0 aus, 1 je Paket, 5..20 Sekunden im Empfang");
  } else {
    g->setTextColor(0x8410);
    g->print("Enter:Ende  Space:weiter  Ziffern:Wert  C:Foto");
  }
  if (frameCanvas) blit(frameCanvas);
}

// Menue: laeuft blockierend, bis Enter (ohne Eingabe) gedrueckt wird; der Empfang pausiert dabei kurz.
// Der Buchstabe waehlt nur den Punkt; Werte per Ziffern + Enter oder Trackball.
static void runMenu(int startSel = 0) {
  int sel = startSel;
  editing = false;
  editLen = 0;
  drawMenu(sel);
  bool done = false;
  while (!done) {
    gpsPoll();
    if (wifiOn) { server.handleClient(); ArduinoOTA.handle(); }  // sonst ist die Webseite/OTA tot, solange das Menue offen ist
    if (shutdownConfirm && millis() - shutdownConfirmMs > 4000) { shutdownConfirm = false; drawMenu(sel); }
    if (menuNoteUntil && millis() > menuNoteUntil) { menuNoteUntil = 0; drawMenu(sel); }
    int e = pollInput();
    if (e == EV_NONE) { delay(5); continue; }
    bool redraw = true;
    if (e == '\r' || e == '\n') {
      if (editing) { menuApplyTyped(sel); editing = false; editLen = 0; saveSettings(); }
      else done = true;
    } else if (e == ' ' || e == EV_CLICK) {
      if (editing) { menuApplyTyped(sel); editing = false; editLen = 0; saveSettings(); }
      else menuStep(sel, +1, false);
    } else if (e == 0x08) {
      if (editing) { if (--editLen <= 0) { editing = false; editLen = 0; } }
      else menuStep(sel, -1, false);
    } else if (e == EV_LEFT)  { editing = false; editLen = 0; menuStep(sel, -1, false); }
    else if (e == EV_RIGHT)   { editing = false; editLen = 0; menuStep(sel, +1, false); }
    else if (e == EV_UP)      { editing = false; editLen = 0; menuStep(sel, +1, true); }
    else if (e == EV_DOWN)    { editing = false; editLen = 0; menuStep(sel, -1, true); }
    else if (isdigit(e) || e == '.' || e == ',' || (e == '-' && sel == MI_LAND && editLen == 0)) {
      if (menuIsNumeric(sel)) {
        if (!editing) { editing = true; editLen = 0; }
        if (editLen < 9) editBuf[editLen++] = (char)e;
      } else {
        redraw = false;
      }
    } else {                                         // Buchstabe: nur den Menuepunkt waehlen
      int c = tolower(e);
      bool found = false;
      if (c == 'c') {                                // Screenshot des Menues (vor dem Hinweis aufnehmen)
        char m[40];
        bool ok = takeScreenshot(true, m, sizeof(m));
        snprintf(menuNote, sizeof(menuNote), ok ? "Foto: %s" : "Foto fehlgeschlagen: %s", m);
        menuNoteUntil = millis() + 2500;
        found = true;
      }
      if (c == 'f' && sel == MI_FREQ) {              // schon auf F: zweites F uebernimmt die letzte Sondenfrequenz
        if (lastRxFreq > 0) {
          freqMHz = lastRxFreq;
          fixedFreq = true;
          restartRx = true;
          editing = false; editLen = 0;
          saveSettings();
          snprintf(menuNote, sizeof(menuNote), "Frequenz uebernommen: %.3f MHz", freqMHz);
        } else {
          snprintf(menuNote, sizeof(menuNote), "noch keine Sonde empfangen");
        }
        menuNoteUntil = millis() + 2500;
        found = true;
      }
      for (int i = 0; i < MI_COUNT && !found; i++)
        if (menuKey[i] == c) { sel = i; editing = false; editLen = 0; shutdownConfirm = false; found = true; break; }
      if (!found) redraw = false;
    }
    if (redraw) drawMenu(sel);
  }
  editing = false;
  showView();
}

// Kurzreferenz der Kartentasten (Taste H in der Kartenansicht, siehe handleInput()): schreibt direkt aufs
// Display (wie drawMenu()/findPickDrawBar()), damit es unabhaengig vom aktuellen Bildpuffer immer geht.
static void drawHelpScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(4, 2);
  tft.print("Hilfe -- Kartenansicht");
  struct { const char *k, *t; } rows[] = {
    {"Trackball", "Karte verschieben"},
    {"A/D, O/P", "Zoom raus / rein"},
    {"M", "Kartenart wechseln"},
    {"R", "Fokus wechseln (Sonde/Ich/beide bzw. im LoRa-Modus Ich/alle Stationen/direkt gehoert)"},
    {"0", "Trackball sperren/entsperren"},
    {"Leertaste/Klick", "Ansicht Karte/Hoehentrack wechseln"},
    {"Enter", "Einstellungsmenue"},
    {"$", "Ton an/aus"},
    {"C", "Screenshot"},
    {"U", "GPX-Zwischenspeicher sichern"},
    {"Q", "Sonde gefunden melden"},
    {"Buchstabe", "Menuepunkt direkt oeffnen"},
  };
  int y = 16;
  for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(4, y);
    tft.print(rows[i].k);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(112, y);
    tft.print(rows[i].t);
    y += 17;
  }
  tft.setTextColor(0x8410);
  tft.setCursor(4, 228);
  tft.print("Beliebige Taste/Klick: zurueck zur Karte");
}

// Blockiert, bis eine beliebige Taste oder ein Trackball-Klick kommt, dann zurueck zur Karte (showView()).
static void showHelpScreen() {
  drawHelpScreen();
  while (true) {
    gpsPoll();
    int e = pollInput();
    if (e == EV_NONE) { delay(5); continue; }
    break;
  }
  showView();
}

static void toast(const char *name, const char *val) {
  snprintf(toastText, sizeof(toastText), "%s: %s", name, val);
  toastUntil = millis() + 1800;
  if (mapMode) {
    if (gfxReady()) gfxUpdate();
    else {                                            // kein Bildpuffer: Hinweis direkt zeichnen
      tft.fillRect(0, 0, 262, 12, ST77XX_BLACK);
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_YELLOW);
      tft.setCursor(2, 2);
      tft.print(toastText);
    }
  } else line(106, toastText, ST77XX_YELLOW);
}

static void toastTick() {
  if (toastUntil && millis() >= toastUntil) {
    toastUntil = 0;
    toastText[0] = 0;
    if (mapMode) {
      if (gfxReady()) gfxUpdate();
      else showView();
    } else line(106, "");
  }
}

// Karte: gesammelte Zoomtasten (A/D) und Trackball-Impulse anwenden. Die erste Bedienung schaltet auf manuell.
static void mapNavTick() {
  bool inMap = mapMode && gfxView == 0 && gfxReady();
  if (!inMap) { panAccX = 0; panAccY = 0; zoomPend = 0; return; }
  bool zoomDue = (zoomPend != 0.0) && (millis() - zoomPendMs >= 500);  // 0,5 s seit der letzten Zoomtaste warten,
                                                                        // damit mehrere schnelle Tastendruecke nur
                                                                        // EIN (langsames) Neuzeichnen ausloesen
  bool panDue = (panAccX != 0 || panAccY != 0);   // kein Mindestschwellwert mehr -- jeder Impuls scrollt sofort
                                                    // (waehrend der Fahrt Trackball stattdessen mit Taste 0 sperren)
  if (!zoomDue && !panDue) return;
  if (shownZ < 0) { panAccX = 0; panAccY = 0; zoomPend = 0; return; }   // noch keine Karte gezeichnet
  enterManual((zoomDue && panDue) ? "Zoomtaste+Trackball" : (panDue ? "Trackball" : "Zoomtaste"));
  if (zoomDue) {
    manZoom += zoomPend;
    zoomPend = 0;
    if (manZoom < (double)MAP_MIN_Z) manZoom = (double)MAP_MIN_Z;
    if (manZoom > (double)MAP_MAX_Z) manZoom = (double)MAP_MAX_Z;
  }
  if (panDue) {
    // Pan-Diagnose: Ausloeser scharf stellen, BEVOR panAccX/Y verworfen werden -- zeigt, wie viele Impulse sich
    // bis zum Ausloesen angesammelt haben. Wie lange das Sammeln gedauert hat (und ob der Trackball zwischendurch
    // durch ein blockierendes Rendern gar nicht abgefragt wurde), zeigen die "PAN Impuls"-Zeilen aus pollInput().
    Serial.printf("PAN Ausloeser: dx=%d dy=%d\n", panAccX, panAccY);
    panLogArm = true;
    panLogT0 = millis();
    double z = effZoom(manZoom), px, py;
    latLonToPixel(manLat, manLon, z, px, py);
    px += (double)panAccX * (double)panPx * PAN_DIR;
    py += (double)panAccY * (double)panPx * PAN_DIR;
    panAccX = 0;
    panAccY = 0;
    double n = 256.0 * exp2(z);
    if (px < 0.0) px = 0.0;
    if (px > n - 1.0) px = n - 1.0;
    if (py < 0.0) py = 0.0;
    if (py > n - 1.0) py = n - 1.0;
    pixelToLatLon(px, py, z, manLat, manLon);
  }
  viewZ = -1;                                       // Hintergrund neu rendern
  if (zoomDue) {
    char v[12];
    snprintf(v, sizeof(v), "%.2f", effZoom(manZoom));
    toast("Zoom", v);                               // zeichnet die Karte neu
  } else {
    gfxUpdate();
  }
}

static void aprsBeaconTick();   // weiter unten definiert (nach aprsPoll()/radio.transmit()), siehe dort
static void aprsSymbolTick();   // weiter unten definiert, direkt vor aprsBeaconTick()
static bool aprsSendBeacon();   // weiter unten definiert, vor aprsBeaconTick() -- fuer den Sofort-Versand per
                                 // Trackballklick in handleInput() (unabhaengig vom Takt in aprsBeaconTick())

#define TBLOCK_DOUBLE_TAP_MS 2000UL   // Zeitfenster fuer den schnellen zweiten Tipp auf Taste 0 (an -> an+Tastatur)

// Eingaben in den normalen Ansichten: Enter = Menue, Space/Klick = Ansicht wechseln,
// Buchstaben = Menue mit gewaehltem Punkt; in der Karte: A/D = Zoom, Trackball = scrollen, R = automatisch
static void handleInput() {
  wifiTick();                                      // Verbindungsaufbau zum Heimnetz begleiten
  if (wifiOn) { server.handleClient(); ArduinoOTA.handle(); }  // Webseite fuer Screenshots/Logs bedienen, OTA-Anfragen bedienen
  batteryTick();
  batteryLogTick();
  gpsLogTick();
  ownTrailTick();
  gpxTick();
  wifiLinkTick();
  wifiIdleTick();
  recoveryQueueTick();
  ntpTick();
  onlinePredTick();                                // Landevorhersage per Internet, siehe oben bei predictLanding()
  sondehubListenerTick();                          // Chase-Car-Position an SondeHub, siehe Menue A
  aprsSymbolTick();                                // APRS-Symbol (Stationaer/Fussgaenger/Fahrrad/Bus) nach Tempo
  aprsBeaconTick();                                // eigene Position per LoRa Upload, siehe Menue A im LoRa-Modus
  beepTick();
  toastTick();
  int e = pollInput();
  mapNavTick();                                    // Zoomtasten und Trackball in der Karte anwenden
  bigRefillTick();                                 // hoechstens 1 Randkachel fuer bigCanvas nachladen (meist No-op)
  if (e == EV_NONE) return;
  if (tbLockMode >= 2 && e != '0') return;           // Tastatursperre (Stufe 2): jede physische Eingabe ausser 0
                                                      // wird verworfen -- die Weboberflaeche ist davon NICHT betroffen
                                                      // (sie laeuft an handleInput() vorbei direkt ueber die /m...-Routen)
  if (e == '0') {                                    // Trackball-Sperre direkt per Taste, ohne Menue, in jeder Ansicht:
                                                      // aus -> an -> an+Tastatur -> aus, aber mit Zeitfenster statt
                                                      // stumpfem Rundlauf -- aus Stufe 1 ("an") fuehrt ein SCHNELLER
                                                      // zweiter Tipp (binnen TBLOCK_DOUBLE_TAP_MS) gleich weiter auf
                                                      // Stufe 2 (an+Tastatur), ein spaeterer Tipp (Stufe 1 steht schon
                                                      // laenger) geht dagegen zuerst wieder auf aus zurueck. Aus Stufe 2
                                                      // fuehrt IMMER nur diese Taste direkt zurueck auf aus (einzige
                                                      // Moeglichkeit dort heraus, s. o.)
    uint32_t now = millis();
    if (tbLockMode == 0) {
      tbLockMode = 1;
      tbLock1SetMs = now;
    } else if (tbLockMode == 1 && now - tbLock1SetMs <= TBLOCK_DOUBLE_TAP_MS) {
      tbLockMode = 2;
    } else {
      tbLockMode = 0;
    }
    saveSettings();
    toast("Trackball", tbLockMode == 0 ? "entsperrt" : (tbLockMode == 1 ? "gesperrt" : "gesperrt (+Tastatur)"));
    return;
  }
  if (e == '\r' || e == '\n') { runMenu(); return; }
  if (e == ' ') { cycleView(); return; }
  bool inAprsList = (mapMode && gfxView == 1 && radioMode == 1);   // LoRa-APRS-Stationsliste, siehe altTrackUpdate()
  if (inAprsList && e == 0x08) {                       // Backspace: Auswahl aufheben (gleiche Taste wie sonst
                                                        // "Abbruch/loeschen" bei Texteingabe, siehe readTextInput()) --
                                                        // bisher kam man aus einer Auswahl nur per vielen EV_UP-
                                                        // Schritten wieder heraus
    aprsListSel = -1;
    aprsPopupShown = false;
    gfxUpdate();
    return;
  }
  if (inAprsList && (e == EV_UP || e == EV_DOWN)) {    // Trackball hoch/runter: Auswahl in der Liste bewegen
                                                        // (sonst ungenutzt ausserhalb der Karte, siehe mapNavTick())
    aprsListMoveSel(e == EV_DOWN ? 1 : -1);
    gfxUpdate();
    return;
  }
  if (inAprsList && e == EV_CLICK) {                   // Trackballklick in der Stationsliste: NICHT wie sonst die
                                                        // eigene Position senden (siehe unten), sondern Zusatzinfo
                                                        // (Kommentar/Kurs/Hoehe hinter Position+Symbol, siehe
                                                        // aprsExtractComment()) der markierten Station ein-/ausblenden
    aprsListMoveSel(0);                                 // noch keine Auswahl: oberste (neueste) Station markieren
    if (aprsListSel >= 0 && aprsComment[aprsListSel][0]) {
      aprsPopupShown = !aprsPopupShown;
      gfxUpdate();
    } else {
      toast("LoRa-APRS", aprsListSel >= 0 ? "keine Zusatzinfo" : "keine Station gehoert");
    }
    return;
  }
  if (inAprsList && tolower(e) == 'r' && aprsPopupShown && aprsListSel >= 0) {
    // Direkt aus dem Zusatzinfo-Popup der Stationsliste auf die Karte springen, eingezoomt auf "Ich" + genau
    // diese eine Station (aprsSoloIdx, siehe idleMapUpdate()) -- bisher ging per "R" auf der Karte selbst nur
    // ein Fokus auf ALLE bzw. alle DIREKT gehoerten Stationen (aprsFocus), nie auf eine einzelne ausgewaehlte.
    // "R" ist hier noch frei (siehe unten: dort nur innerhalb der Kartenansicht wirksam), gleiche Taste wie
    // auf der Karte selbst ("Fokus") liegt also nahe.
    aprsSoloIdx = aprsListSel;
    aprsPopupShown = false;
    gfxView = 0;
    viewZ = -1;                 // neu einzoomen, damit "Ich" und die Station beide ins Bild passen
    toast("Fokus", aprsCall[aprsSoloIdx]);
    gfxUpdate();
    return;
  }
  if (e == EV_CLICK) {                               // Trackballklick: im LoRa-Modus mit eingeschaltetem Upload
                                                      // (Menue A) sofort die eigene Position senden, unabhaengig vom
                                                      // Zeit-/Wegstrecke-Takt in aprsBeaconTick() (der laeuft
                                                      // unveraendert weiter) -- sonst (wie bisher) nur Ansicht wechseln
    if (radioMode == 1 && loraTxOn) {
      double lat, lon;
      if (homePosition(lat, lon) && gpsFixGoodEnoughToSend()) {
        aprsSendBeacon();
        toast("LoRa", "Position gesendet");
      } else {
        toast("LoRa", "kein brauchbarer GPS-Fix");
      }
    } else {
      cycleView();
    }
    return;
  }
  if (e == '$') {                                   // Lautsprecher-Taste: Ton direkt an/aus, ohne Menue, in jeder Ansicht
    if (beepSec) { beepSecMute = beepSec; beepSec = 0; }
    else beepSec = beepSecMute;
    saveSettings();
    toast("Ton", beepSec == 0 ? "aus" : (beepSec == 1 ? "je Paket" : "an"));
    return;
  }
  int c = tolower(e);
  if (mapMode && gfxView == 0 && gfxReady()) {
    if (c == 'a' || c == 'd' || c == 'o' || c == 'p') {   // A/D links, O/P rechts auf der Tastatur (Einhandbedienung): heraus/hinein
      zoomPend += (c == 'd' || c == 'p') ? 1.0 : -1.0;    // seit FW 3.0 immer eine ganze Zoomstufe pro Tastendruck
      zoomPendMs = millis();
      return;
    }
    if (c == 'r') { focusCycleOrRecenter(); return; }   // siehe focusCycleOrRecenter() -- auch von der Weboberflaeche
                                                         // (/vfocus) genutzt, deshalb als eigene Funktion ausgelagert
  }
  if (c == 'n') {                                 // Testtaste: grosse Statuszeile auf der Karte erzwingen, ohne
                                                     // tatsaechlich > 8 km/h fahren zu muessen (bleibt rein im RAM,
                                                     // kein Eintrag in Settings oder SD-Log)
    dbgSpeedOverride = (dbgSpeedOverride >= 0.0f) ? -1.0f : (OWN_SPEED_BIG_KMH + 12.0f);
    char v[24];
    if (dbgSpeedOverride >= 0.0f) snprintf(v, sizeof(v), "%.0f km/h (Test)", dbgSpeedOverride);
    else snprintf(v, sizeof(v), "aus");
    toast("Test-Tempo", v);
    return;
  }
  if (c == 'c') {                                 // Screenshot des aktuellen Bildschirms
    char m[40];
    bool ok = takeScreenshot(false, m, sizeof(m));
    toast(ok ? "Foto" : "Foto fehlt", m);
    return;
  }
  if (c == 'u') {                                 // GPX-Zwischenspeicher sofort auf die SD schreiben (vor dem Ausschalten!)
    int pending = gpxCountIch + gpxCountSonde;
    gpxDrainAll();
    char v[24];
    snprintf(v, sizeof(v), pending == 0 ? "nichts zu tun" : "%d Punkt(e)", pending);
    toast("GPX gesichert", v);
    return;
  }
  if (c == 'q') {                                 // Sonde gefunden: Sonde+Position aus den GPX-Spuren waehlen
                                                     // (findPick(), siehe oben), dann senden bzw. in die Warteliste (bis 10)
    char serial[9] = "";
    double repLat = 0, repLon = 0;
    float repAlt = 0;
    if (!findPick(serial, repLat, repLon, repAlt)) return;
    char by[40];
    int nBy = textInput("Melder (Rufzeichen/Name)", by, sizeof(by), false, gpxRecoveredBy,
              "Enter: uebernehmen   leer: vorigen Melder behalten", true);
    if (nBy < 0) { showView(); toast("Sonde gefunden", "abgebrochen"); return; }   // per Backspace auf leerem
                                                     // Feld -- bisher ging das nur ueber einen Geraete-Reset
    if (!by[0]) { strncpy(by, gpxRecoveredBy, sizeof(by) - 1); by[sizeof(by) - 1] = 0; }
    if (!by[0]) { showView(); toast("Sonde gefunden", "kein Melder angegeben"); return; }
    char desc[160];
    int n = textInput("Fundbericht", desc, sizeof(desc), false, "", "Enter: senden/speichern   leer + Enter: abbrechen", true, true);
    if (n <= 0) { showView(); toast("Sonde gefunden", "abgebrochen"); return; }
    if (!findConfirm(serial, repLat, repLon, repAlt, by, sizeof(by), desc, sizeof(desc))) {
      showView();
      toast("Sonde gefunden", "abgebrochen");
      return;
    }
    strncpy(gpxRecoveredBy, by, sizeof(gpxRecoveredBy) - 1);
    gpxRecoveredBy[sizeof(gpxRecoveredBy) - 1] = 0;
    prefs.putString("recby", gpxRecoveredBy);
    showView();
    int r = queueOrSendRecovery(serial, repLat, repLon, repAlt, by, desc);
    char v[32];
    if (r == 0) snprintf(v, sizeof(v), "%s gemeldet", serial);
    else if (r == 1) snprintf(v, sizeof(v), "gespeichert (%d wartend)", recoveryCount());
    else snprintf(v, sizeof(v), "Warteliste voll (10)");
    toast(r == 2 ? "Fehlgeschlagen" : "SondeHub", v);
    return;
  }
  if (c == 'm' && mapMode && gfxView == 0) {      // Kartenart direkt umschalten (ohne Menue)
    menuStep(MI_MAP, +1, false);
    char v[24];
    menuValue(MI_MAP, v, sizeof(v));
    toast("Karte", v);
    return;
  }
  if (c == 'h' && mapMode && gfxView == 0) {      // Kurzreferenz der Kartentasten (frueher Helligkeit, siehe Taste Y)
    showHelpScreen();
    return;
  }
  for (int i = 0; i < MI_COUNT; i++)              // sonst: Buchstabe oeffnet das Menue, Punkt ist schon gewaehlt
    if (menuKey[i] == c) { runMenu(i); return; }
}

// Welche Zoomstufen und x-Bereiche liegen auf der Karte?
static void tileCoverageReport() {
  Serial.println("--- Zoomstufen unter " TILE_DIR " ---");
  for (int z = 1; z <= MAP_MAX_Z; z++) {
    char dir[32];
    snprintf(dir, sizeof(dir), "%s/%d", TILE_DIR, z);
    File d = SD.open(dir);
    if (!d || !d.isDirectory()) {
      Serial.printf("z%-2d: Ordner fehlt\n", z);
      if (d) d.close();
      continue;
    }
    long xmin = 0x7FFFFFFF, xmax = -1;
    int cnt = 0;
    File e = d.openNextFile();
    while (e && cnt < 4000) {
      if (e.isDirectory()) {
        const char *nm = strrchr(e.name(), '/');
        nm = nm ? nm + 1 : e.name();
        if (nm[0] >= '0' && nm[0] <= '9') {       // Mac-Dateien wie .Spotlight-V100 ueberspringen
          long x = atol(nm);
          if (x < xmin) xmin = x;
          if (x > xmax) xmax = x;
          cnt++;
        }
      }
      e.close();
      e = d.openNextFile();
    }
    if (e) e.close();
    d.close();
    double n = (double)(1UL << z);
    Serial.printf("z%-2d: %d%s x-Ordner, x %ld..%ld, Laenge %.3f..%.3f\n", z, cnt, cnt >= 4000 ? "+" : "",
                  xmin, xmax, xmin / n * 360.0 - 180.0, (xmax + 1) / n * 360.0 - 180.0);
  }
}

// Gibt fuer jede Zoomstufe an, ob die Kachel an der Testposition existiert; liefert die hoechste Stufe <= TILE_ZOOM
static int tileAtPositionReport() {
  Serial.printf("--- Kacheln an Testposition %.6f, %.6f ---\n", TEST_LAT, TEST_LON);
  int sel = 0;
  for (int z = 1; z <= 18; z++) {
    double px, py;
    latLonToPixel(TEST_LAT, TEST_LON, z, px, py);
    char path[64];
    tilePath(path, sizeof(path), z, (long)(px / 256.0), (long)(py / 256.0));
    File f = SD.open(path);
    if (f) {
      Serial.printf("z%-2d: %s vorhanden (%u Bytes)\n", z, path, (unsigned)f.size());
      f.close();
      if (z <= TILE_ZOOM) sel = z;
    } else {
      Serial.printf("z%-2d: %s FEHLT\n", z, path);
    }
  }
  return sel;
}

__attribute__((unused)) static void tileSelfTest() {
  Serial.println("\n##### Kartentest #####");
  line(12, "Kartentest: SD-Karte ...");
  if (!sdInit()) {
    Serial.println("SD.begin fehlgeschlagen: Karte nicht gefunden oder nicht FAT32 (exFAT wird nicht gelesen)");
    line(12, "SD-Karte nicht lesbar (FAT32?)", ST77XX_RED);
    delay(3000);
    return;
  }
  Serial.printf("SD ok: Kartentyp %d, Groesse %llu MB\n", SD.cardType(), SD.cardSize() / (1024ULL * 1024ULL));
  File ex = SD.open("/tiles/18/136206/87000.png");
  Serial.printf("Beispielkachel /tiles/18/136206/87000.png: %s\n", ex ? "vorhanden" : "FEHLT");
  if (ex) ex.close();
  tileCoverageReport();
  int z = tileAtPositionReport();
  if (z > 0) {
    Serial.printf("Zeichne Testkarte bei Zoom %d ...\n", z);
    uint32_t t0 = millis();
    showCentered(TEST_LAT, TEST_LON, z);
    Serial.printf("Testkarte gezeichnet in %lu ms\n", (unsigned long)(millis() - t0));
    delay(8000);
  } else {
    line(12, "Keine Kachel an Testposition", ST77XX_ORANGE);
    delay(3000);
  }
  Serial.println("##### Kartentest Ende #####\n");
}

// ---------- Funk ----------
float measure(float fMHz) {
  radio.standby();
  radio.setFrequency(fMHz, false);
  radio.startReceive();
  delayMicroseconds(SETTLE_US);
  return radio.getRSSI(false);       // Momentan-RSSI
}

// Grobscan: 31 Schritte mit 467 kHz Filter, SCAN_SWEEPS Durchlaeufe gemittelt (schwaecheres Signal erkennbar)
void coarseScan() {
  radio.standby();
  radio.setRxBandwidth(COARSE_BW);
  float sum[NCOARSE] = {0};
  for (int k = 0; k < SCAN_SWEEPS; k++)
    for (int i = 0; i < NCOARSE; i++) sum[i] += measure(SCAN_START + i * COARSE_STEP);
  for (int i = 0; i < NCOARSE; i++) spec[i] = sum[i] / SCAN_SWEEPS;
}

// Waehlt bis zu MAX_CAND lokale Spitzen ueber Median + REL_MARGIN
int pickCandidates(float &median) {
  float tmp[NCOARSE];
  memcpy(tmp, spec, sizeof(tmp));
  for (int i = 1; i < NCOARSE; i++) {            // Insertion Sort
    float v = tmp[i]; int j = i - 1;
    while (j >= 0 && tmp[j] > v) { tmp[j + 1] = tmp[j]; j--; }
    tmp[j + 1] = v;
  }
  median = tmp[NCOARSE / 2];
  bool used[NCOARSE] = {false};
  memset(isCand, 0, sizeof(isCand));
  int n = 0;
  while (n < MAX_CAND) {
    int best = -1;
    float bv = median + REL_MARGIN;
    for (int i = 0; i < NCOARSE; i++)
      if (!used[i] && SCAN_START + i * COARSE_STEP >= BAND_MIN && spec[i] > bv) { best = i; bv = spec[i]; }
    if (best < 0) break;
    candF[n] = SCAN_START + best * COARSE_STEP;
    candR[n] = spec[best];
    isCand[best] = true;
    n++;
    for (int j = best - 1; j <= best + 1; j++) if (j >= 0 && j < NCOARSE) used[j] = true;
  }
  return n;
}

// Feinscan um einen Kandidaten mit dem Empfangsfilter.
// Die Sonde sendet nicht durchgehend (Log: im Grobscan nur in etwa der Haelfte der Messungen sichtbar).
// Darum FINE_PASSES Durchlaeufe hintereinander und je Frequenz das Maximum nehmen: Bei 4 Durchlaeufen
// (ca. 170 ms Abstand je Frequenz) liegt mindestens eine Messung sicher in der Sendephase.
// margin = Abstand des Peaks zum Median des Feinscans (schmales Signal = grosser Abstand)
float fineScan(float center, float &bestR, float &margin) {
  radio.standby();
  radio.setRxBandwidth(rxBw);
  float v[48];
  int nf = (int)(2 * FINE_SPAN / FINE_STEP + 0.5) + 1;
  if (nf > 48) nf = 48;
  for (int i = 0; i < nf; i++) v[i] = -200;
  for (int pass = 0; pass < FINE_PASSES; pass++)
    for (int i = 0; i < nf; i++) {
      float r = measure(center - FINE_SPAN + i * FINE_STEP);
      if (r > v[i]) v[i] = r;
    }
  float bestF = center;
  bestR = -200;
  for (int i = 0; i < nf; i++)
    if (v[i] > bestR) { bestR = v[i]; bestF = center - FINE_SPAN + i * FINE_STEP; }
  float tmp[48];
  memcpy(tmp, v, nf * sizeof(float));
  for (int i = 1; i < nf; i++) {                 // Insertion Sort
    float x = tmp[i]; int j = i - 1;
    while (j >= 0 && tmp[j] > x) { tmp[j + 1] = tmp[j]; j--; }
    tmp[j + 1] = x;
  }
  margin = bestR - tmp[nf / 2];
  return bestF;
}

bool waitPacket(uint8_t *buf, int len, uint32_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    if (rxFlag) {
      rxFlag = false;
      return radio.readData(buf, len) == RADIOLIB_ERR_NONE;
    }
    gpsPoll();
    handleInput();
    delay(1);
  }
  return false;
}

// Schritt 1: Header-Sync, danach CAP1_LEN Bytes
bool captureHeader(float fMHz, uint32_t timeoutMs) {
  radio.standby();
  radio.setRxBandwidth(rxBw);
  radio.setFrequency(fMHz, false);
  radio.setSyncWord((uint8_t *)SYNC_HDR, 8);
  radio.setPreambleLength(16);
  radio.fixedPacketLengthMode(CAP1_LEN);
  rxFlag = false;
  radio.startReceive();
  return waitPacket(raw1, CAP1_LEN, timeoutMs);
}

// ---------- Direkte SX126x-Befehle (an RadioLib vorbei) ----------
// Grund: RadioLib schreibt bei jedem startReceive() die FSK-Paketparameter neu. Fuer Schritt 2
// (Sync mitten im Frame, ohne Praeambel) muss der Praeambel-Detektor aber aus sein.
static void waitBusy() {
  uint32_t t0 = millis();
  while (digitalRead(PIN_LORA_BUSY) && millis() - t0 < 50) {}
}

static void sxCommand(uint8_t op, const uint8_t *d, int n) {
  waitBusy();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_LORA_CS, LOW);
  SPI.transfer(op);
  for (int i = 0; i < n; i++) SPI.transfer(d[i]);
  digitalWrite(PIN_LORA_CS, HIGH);
  SPI.endTransaction();
  waitBusy();
}

// SetPacketParams (0x8C): Praeambel-Detektor AUS, feste Laenge, CRC/Whitening/Adressfilter aus
static void setPacketParamsNoPreamble(uint8_t syncBits, uint8_t payloadLen) {
  uint8_t d[9] = {0x00, 0x00,    // Praeambellaenge 0
                  0x00,          // Praeambel-Detektor AUS
                  syncBits,      // Sync-Wort-Laenge in Bit
                  0x00,          // keine Adressfilterung
                  0x00,          // feste Paketlaenge
                  payloadLen,
                  0x01,          // CRC aus
                  0x00};         // Whitening aus
  sxCommand(0x8C, d, 9);
}

// Schritt 2: Sync auf ID/Laenge des Blocks 0x7D, danach CAP2_LEN Bytes (ohne Praeambel-Erkennung)
void armBlockPos() {
  radio.standby();
  radio.setSyncWord(syncPos, 2);
  radio.setPreambleLength(0);             // RadioLib: Wert 0 = kein Praeambel-Detektor (in der Version vom Test: Rueckgabe 0)
  radio.fixedPacketLengthMode(CAP2_LEN);
  radio.startReceive();                   // richtet IRQ/DIO1 und Puffer ein und startet RX
  // Sicherheitshalber Paketparameter direkt setzen und RX neu starten:
  uint8_t sb = 0x00;
  sxCommand(0x80, &sb, 1);                // SetStandby (STDBY_RC)
  setPacketParamsNoPreamble(16, CAP2_LEN);
  rxFlag = false;
  uint8_t rx[3] = {0xFF, 0xFF, 0xFF};
  sxCommand(0x82, rx, 3);                 // SetRx, Timeout 0xFFFFFF = dauerhaft empfangen
}

void haltWithError(const char *msg, int code) {
  Serial.printf("FEHLER: %s, Code %d\n", msg, code);
  line(12, String(msg) + " Code " + String(code), ST77XX_RED);
  while (true) delay(1000);
}

// ---------- LoRa-APRS-Empfang (Menue I, radioMode==1) ----------
// Funk auf RS41-FSK-Empfang stellen (bisheriger Inhalt von setup()s radio.beginFSK()-Block, jetzt
// wiederverwendbar fuer den Rueckwechsel aus dem LoRa-Modus, siehe radioApplyMode()).
static int sondeRadioInit() {
  int st = radio.beginFSK(SCAN_START, BITRATE, FREQ_DEV, rxBw, 10, 16, TCXO_V);
  Serial.printf("radio.beginFSK -> %d\n", st);
  if (st != RADIOLIB_ERR_NONE) return st;
  radio.setDio2AsRfSwitch(true);
  radio.setCRC(0);
  radio.setDataShaping(RADIOLIB_SHAPING_0_5);
  radio.setDio1Action(onRx);
  return RADIOLIB_ERR_NONE;
}

// Funk auf LoRa-APRS-Empfang stellen (433.775 MHz / SF12 / BW125 / CR4-5, siehe APRS_*-Defines).
static int aprsRadioInit() {
  int st = radio.begin(APRS_FREQ_MHZ, APRS_BW_KHZ, APRS_SF, APRS_CR, APRS_SYNCWORD, 2, APRS_PREAMBLE, TCXO_V);
  Serial.printf("radio.begin (LoRa-APRS) -> %d\n", st);
  if (st != RADIOLIB_ERR_NONE) return st;
  radio.setDio2AsRfSwitch(true);
  radio.setCRC(1);          // 1 Byte Laenge = CRC an (Standard-APRS-Pakete tragen ein CRC)
  radio.setDio1Action(onRx);
  rxFlag = false;
  radio.startReceive();
  return RADIOLIB_ERR_NONE;
}

// radioMode wurde im Menue/per Web geaendert (radioModeChanged) -- Funk umschalten, Sondenanzeige und
// Stationsliste zuruecksetzen. Wird aus loop() aufgerufen, nicht direkt aus dem Menue (vermeidet einen
// Funk-Reinit mitten in einer laufenden Empfangsrunde).
static void radioApplyMode() {
  radio.standby();
  haveSondePos = false;     // Sonde ist im LoRa-Modus nicht relevant; nach Ruecksprung neu auf Empfang warten
  aprsN = 0;                 // Stationsliste beim Moduswechsel neu beginnen
  aprsSoloIdx = -1;          // Einzelstations-Fokus (siehe dort) gehoert zur alten Liste, nicht mehr gueltig
  altTrackReset();           // Hoehentrack beim Moduswechsel neu beginnen (sonst alte Sonde ueber neuem Modus)
  loraTxOn = false;          // LoRa Upload nach jedem Moduswechsel immer aus (siehe loraTxOn-Deklaration)
  aprsBeaconLastSet = false; // Wegstrecke-Anker fuer den naechsten Upload verwerfen (gehoert zum vorigen Modus/Einschalten)
  restartRx = true;
  int st = (radioMode == 1) ? aprsRadioInit() : sondeRadioInit();
  if (radioMode == 0) {       // Sync fuer Schritt 2 neu berechnen (sonst identisch zu setup())
    syncPos[0] = rev8(SYNC2_ID ^ MASK[SYNC2_POS & 63]);
    syncPos[1] = rev8(SYNC2_LEN ^ MASK[(SYNC2_POS + 1) & 63]);
  }
  if (radioMode == 1 && !mapMode) {   // Textansicht (Frequenzscan) gibt's im LoRa-Modus nicht -> auf Karte ausweichen
    mapMode = true;
    gfxView = 0;
    showView();
  }
  char m[48];
  snprintf(m, sizeof(m), "Funkmodus: %s (Init %d)", radioMode == 1 ? "LoRa-APRS" : "Sondenempfang", st);
  logLine(m);
  toast("Funkmodus", radioMode == 1 ? "LoRa-APRS" : "Sondenempfang");
}

// Rohpayload beginnt bei manchen LoRa-APRS-Implementierungen mit einem 3-Byte-Header 0x3C/0xFF/0x01 vor
// der eigentlichen TNC2-Textzeile -- keine offizielle APRS-Vorgabe, nur eine verbreitete Konvention
// (siehe Hinweis bei den APRS_*-Defines). Ist er nicht da, bleibt der Puffer unveraendert.
static void aprsStripHeader(uint8_t *d, size_t &len) {
  if (len > 3 && d[0] == 0x3C && d[1] == 0xFF && d[2] == 0x01) { memmove(d, d + 3, len - 3); len -= 3; }
}

// Rufzeichen (vor dem ersten '>') aus einer TNC2-Zeile "RUFZEICHEN-SSID>PFAD:INFO" lesen.
static bool aprsParseCall(const char *s, char *out, size_t outLen) {
  const char *gt = strchr(s, '>');
  if (!gt || gt == s) return false;
  size_t n = (size_t)(gt - s);
  if (n >= outLen) n = outLen - 1;
  memcpy(out, s, n);
  out[n] = 0;
  return true;
}

// Liest GENAU 5 Zeichen ("MM.mm", die Minuten-Nachkommastelle von Lat/Lon) und wandelt sie in eine Zahl um --
// NICHT einfach atof(p) auf den Rest des Strings, denn das liest beliebig viele Folgezeichen mit: steht direkt
// danach zufaellig ein 'E' (z. B. das "Ost"-Zeichen der Laengenangabe selbst, siehe aprsParsePosition()/
// aprsParseObject()) gefolgt von weiteren Ziffern (Kurs/Geschwindigkeit im Rest des Pakets), liest die libc das
// als wissenschaftliche Notation ("12.345E123" = 12,345 * 10^123) -- ergibt durch Ueberlauf "inf". Genau das
// ist einem Nutzer tatsaechlich passiert (Objekt mit Laenge "inf" im Log) und haette als Karten-Fokuspunkt
// (Taste R, "Ich + alle Stationen") mit nicht-endlichen Pixelkoordinaten vermutlich den gemeldeten Haenger
// ausgeloest (per Checkpoint-Logs bestaetigt, inzwischen wieder entfernt). Zusaetzlich am Ende beider Parser nochmal
// explizit auf endliche, plausible Koordinaten geprueft (isfinite()+Bereichscheck) als zweite Sicherung.
static float aprsReadMin5(const char *p) {
  char buf[6];
  memcpy(buf, p, 5);
  buf[5] = 0;
  return atof(buf);
}

// Komprimierte APRS-Position lesen (Gegenstueck zu aprsEncodeCompressed(), APRS101.pdf Kapitel 9): Symboltabelle(1)
// + Breite(4) + Laenge(4) je Base91 -- Symbolcode/Kurs/Tempo/Typ-Byte danach werden fuer die reine Position
// nicht gebraucht. Mehrere tatsaechlich gehoerte lokale Stationen (LoRa-Tracker, -iGates) senden NUR in diesem
// platzsparenden Format -- ohne diese Erweiterung landeten sie bisher als "nicht geparst" im Log und NIE auf
// Karte/Stationsliste (siehe aprsParsePosition() unten, das jetzt bei fehlendem Zahlen-Praefix hierher verzweigt).
static bool aprsParseCompressed(const char *p, double &lat, double &lon) {
  if (strlen(p) < 9) return false;
  long latVal = 0, lonVal = 0;
  for (int i = 1; i <= 4; i++) {
    if ((unsigned char)p[i] < 33 || (unsigned char)p[i] > 126) return false;
    latVal = latVal * 91 + (p[i] - 33);
  }
  for (int i = 5; i <= 8; i++) {
    if ((unsigned char)p[i] < 33 || (unsigned char)p[i] > 126) return false;
    lonVal = lonVal * 91 + (p[i] - 33);
  }
  lat = 90.0 - latVal / 380926.0;
  lon = -180.0 + lonVal / 190463.0;
  return isfinite(lat) && isfinite(lon) && fabs(lat) <= 90.0 && fabs(lon) <= 180.0;
}

// Unkomprimierte APRS-Position aus dem Infofeld lesen: "!"/"=" gefolgt von "DDMM.mmN/DDDMM.mmE..."
// (Standard-APRS-Positionsformat), bei fehlendem Zahlen-Praefix Weiterleitung an aprsParseCompressed() oben
// (komprimiertes Format faengt NIE mit zwei Ziffern an). Rueckgabe true bei Erfolg, lat/lon in Dezimalgrad.
static bool aprsParsePosition(const char *info, double &lat, double &lon) {
  if (!(info[0] == '!' || info[0] == '=')) return false;
  const char *p = info + 1;
  if (strlen(p) < 2 || !isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1])) return aprsParseCompressed(p, lat, lon);
  if (strlen(p) < 18) return false;
  int dLat = (p[0] - '0') * 10 + (p[1] - '0');
  float mLat = aprsReadMin5(p + 2);
  char ns = p[7];
  if (ns != 'N' && ns != 'S') return false;
  if (!isdigit((unsigned char)p[9]) || !isdigit((unsigned char)p[10]) || !isdigit((unsigned char)p[11])) return false;
  int dLon = (p[9] - '0') * 100 + (p[10] - '0') * 10 + (p[11] - '0');
  float mLon = aprsReadMin5(p + 12);
  char ew = p[17];
  if (ew != 'E' && ew != 'W') return false;
  lat = dLat + mLat / 60.0;
  if (ns == 'S') lat = -lat;
  lon = dLon + mLon / 60.0;
  if (ew == 'W') lon = -lon;
  if (!isfinite(lat) || !isfinite(lon) || fabs(lat) > 90.0 || fabs(lon) > 180.0) return false;
  return true;
}

// APRS-Objektreport lesen: ";OBJEKTNAME(9 Zeichen, rechts mit Leerzeichen aufgefuellt)*/_ZEITSTEMPEL(7 Zeichen)
// DDMM.mmN/DDDMM.mmE..." (APRS-Spec Kap. 11) -- damit verbreiten LoRa-APRS-iGates/Digipeater fremde Positionen
// (z. B. aus APRS-IS oder SondeHub geholte Ballon-/Sondendaten) unter einem eigenen Objektnamen weiter, NICHT
// als eigene Positionsmeldung der sendenden Station (die kommt als "!"/"=", siehe aprsParsePosition()) --
// genau das zeigt das iGate-Display mit "Objekte werden uebertragen" an. Objektname statt Rufzeichen wird in
// der Stationsliste gefuehrt (name-Puffer mind. 10 Byte).
static bool aprsParseObject(const char *info, char *name, size_t nameLen, double &lat, double &lon) {
  if (info[0] != ';') return false;
  if (strlen(info) < 1 + 9 + 1 + 7 + 18) return false;
  char raw[10];
  memcpy(raw, info + 1, 9);
  raw[9] = 0;
  int n = 9;
  while (n > 0 && raw[n - 1] == ' ') n--;   // rechtsbuendig aufgefuellten Namen zurueckschneiden
  if (n == 0) return false;
  // Manche Objektquellen (z. B. SondeHub-/Ballon-Gateways) haengen nach einem Leerzeichen noch einen einzelnen
  // Typbuchstaben an (z. B. "DG8DQ B" fuer Ballon, "DB0BS C" fuer Chase-Car) -- fuer die Anzeige (Karte,
  // Stationsliste, /malt) zaehlt nur das eigentliche Rufzeichen davor. Das Namensfeld ist aber nur 9 Zeichen
  // lang und manche Objekte nutzen es fuer etwas ganz anderes mit Leerzeichen MITTENDRIN (z. B. ein
  // Relais-Objekt "OV 438.225-" mit Frequenzangabe) -- ein simples "beim ERSTEN Leerzeichen abschneiden"
  // zerstoerte das (wurde zu blossem "OV"). Deshalb jetzt nur das ENGE Muster "ein Leerzeichen + genau EIN
  // Grossbuchstabe GENAU am Namensende" erkannt und abgeschnitten, alles andere bleibt wie im Paket.
  if (n >= 3 && raw[n - 2] == ' ' && isupper((unsigned char)raw[n - 1])) {
    int n2 = n - 2;
    while (n2 > 0 && raw[n2 - 1] == ' ') n2--;
    if (n2 > 0) n = n2;
  }
  raw[n] = 0;
  strncpy(name, raw, nameLen - 1);
  name[nameLen - 1] = 0;
  char liveKilled = info[10];
  if (liveKilled != '*' && liveKilled != '_') return false;
  const char *p = info + 18;     // 1 (';') + 9 (Name) + 1 ('*'/'_') + 7 (Zeitstempel) = 18
  if (!isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1])) return false;
  int dLat = (p[0] - '0') * 10 + (p[1] - '0');
  float mLat = aprsReadMin5(p + 2);
  char ns = p[7];
  if (ns != 'N' && ns != 'S') return false;
  if (!isdigit((unsigned char)p[9]) || !isdigit((unsigned char)p[10]) || !isdigit((unsigned char)p[11])) return false;
  int dLon = (p[9] - '0') * 100 + (p[10] - '0') * 10 + (p[11] - '0');
  float mLon = aprsReadMin5(p + 12);
  char ew = p[17];
  if (ew != 'E' && ew != 'W') return false;
  lat = dLat + mLat / 60.0;
  if (ns == 'S') lat = -lat;
  lon = dLon + mLon / 60.0;
  if (ew == 'W') lon = -lon;
  if (!isfinite(lat) || !isfinite(lon) || fabs(lat) > 90.0 || fabs(lon) > 180.0) return false;
  return true;
}

// Rohinhalt eines Pakets als druckbarer Text fuer die Log-Diagnose (nicht druckbare Bytes werden durch
// '.' ersetzt) -- so laesst sich auch ein Paket, das aprsHandlePacket() NICHT als Position erkennt, von
// Auge pruefen (Rohschrott = falscher Demod/Parameter, lesbarer Text = unser Parser passt nicht dazu).
static void aprsRawPreview(const uint8_t *d, size_t len, char *out, size_t outLen) {
  size_t n = (len < outLen - 1) ? len : outLen - 1;
  for (size_t i = 0; i < n; i++) {
    uint8_t c = d[i];
    out[i] = (c >= 32 && c < 127) ? (char)c : '.';
  }
  out[n] = 0;
}

// Aus Rufzeichen + Infofeld eine Position (Stations- oder Objektreport, siehe aprsParsePosition()/
// aprsParseObject()) lesen und in der Stationsliste eintragen. kind: RECV_DIRECT/RECV_DIGI/RECV_IGATE (siehe
// aprsHandlePacket()), bestimmt die Punktfarbe auf der Karte (COL_APRS_*) und den "direkt gehoert"-Fokus.
// viaCall != nullptr (nur bei kind == RECV_IGATE): das Paket kam als Fremdverkehr ueber dieses Rufzeichen
// (z. B. das eigene iGate) an -- nur fuer die Logzeile, nicht fuer die Stationsliste selbst.
// Unterscheidungston fuer eine NEU in der Liste aufgetauchte Station (Menue X/beepSec wie beim normalen
// Empfangston -- "aus" bleibt stumm): RECV_DIRECT hoeher (BEEP_HZ_NEU_DIREKT), RECV_DIGI tiefer
// (BEEP_HZ_NEU_REPEATED). RECV_IGATE (Fremdverkehr ueber APRS-IS/Internet, nicht per Funk gehoert) bekommt
// bewusst keinen eigenen Ton -- hier geht es um per Funk NEU gehoerte Stationen.
static void aprsNewStationBeep(uint8_t kind) {
  if (beepSec == 0) return;
  if (kind == RECV_DIRECT) beepPlayHz(BEEP_HZ_NEU_DIREKT);
  else if (kind == RECV_DIGI) beepPlayHz(BEEP_HZ_NEU_REPEATED);
}

static bool aprsStoreFromInfo(const char *call, const char *info, const char *viaCall, uint8_t kind) {
  static const char *const kindTxt[3] = {"direkt", "digipeated", "Fremdverkehr"};
  double lat, lon;
  if (aprsParsePosition(info, lat, lon)) {
    bool isNew = false;
    int idx = aprsFindOrAdd(call, &isNew);
    aprsLat[idx] = (float)lat;
    aprsLon[idx] = (float)lon;
    aprsSeenMs[idx] = millis();
    aprsRecvKind[idx] = kind;
    aprsTrailAdd(idx, lat, lon);
    aprsExtractComment(info, 20, aprsComment[idx], sizeof(aprsComment[idx]));   // 1 ("!"/"=") + 18 (Position) + 1 (Symbol)
    char m[128];
    if (viaCall) snprintf(m, sizeof(m), "LoRa-APRS: %s bei %.5f %.5f (%s ueber %s)", call, lat, lon, kindTxt[kind], viaCall);
    else snprintf(m, sizeof(m), "LoRa-APRS: %s bei %.5f %.5f (%s)", call, lat, lon, kindTxt[kind]);
    logLine(m);
    if (isNew) aprsNewStationBeep(kind);
    return true;
  }
  char objName[10];
  if (aprsParseObject(info, objName, sizeof(objName), lat, lon)) {
    bool isNew = false;
    int idx = aprsFindOrAdd(objName, &isNew);     // Objektname (z. B. Sonden-ID) statt Rufzeichen des Digi/iGate
    aprsLat[idx] = (float)lat;
    aprsLon[idx] = (float)lon;
    aprsSeenMs[idx] = millis();
    aprsRecvKind[idx] = kind;
    aprsTrailAdd(idx, lat, lon);
    aprsExtractComment(info, 37, aprsComment[idx], sizeof(aprsComment[idx]));   // 18 (Kopf bis Position) + 18 (Position) + 1 (Symbol)
    char m[144];
    if (viaCall) snprintf(m, sizeof(m), "LoRa-APRS-Objekt: %s (ueber %s, %s ueber %s) bei %.5f %.5f", objName, call, kindTxt[kind], viaCall, lat, lon);
    else snprintf(m, sizeof(m), "LoRa-APRS-Objekt: %s (ueber %s, %s) bei %.5f %.5f", objName, call, kindTxt[kind], lat, lon);
    logLine(m);
    if (isNew) aprsNewStationBeep(kind);
    return true;
  }
  return false;
}

// Ein empfangenes LoRa-APRS-Paket auswerten: Header abstreifen, Rufzeichen + Position lesen, Station
// aktualisieren (Position + verkuerzte Spur, siehe aprsTrailAdd()). Rueckgabe true, wenn daraus eine
// Station+Position wurde (fuer die Rohempfangs-Diagnose in aprsPoll()).
// Beginnt das Infofeld mit '}', handelt es sich um APRS-"Fremdverkehr" (Third-Party Traffic, APRS-Spec
// Kap. 13): ein iGate reicht ein ueber APRS-IS/Internet gehoertes Paket unveraendert als eingebettete
// TNC2-Zeile "}URSPRUNGS-RUFZEICHEN>PFAD:INFO" weiter -- so kam z. B. die in der Rohempfangs-Diagnose
// sichtbare Zeile "DL6TJ-10>APLRG1,WIDE1-1:}SP7PGK-15>APNU1..." zustande (eigenes iGate reicht fremde,
// aus dem Internet gehoerte Stationen per LoRa weiter). Die eingebettete Zeile wird dann genauso behandelt
// wie ein direkt empfangenes Paket (kind RECV_IGATE).
// Ohne "}": direkt per Funk empfangen -- aber nur, wenn der Pfad zwischen Rufzeichen und ':' KEINEN per "*"
// als genutzt markierten Digipeater-Hop enthaelt (Standard-AX.25/TNC2-Konvention: ein Digipeater haengt beim
// Weiterreichen ein "*" an das zuletzt benutzte Pfadfeld). Enthaelt der Pfad ein "*", wurde das Paket
// unterwegs per Funk ueber mindestens einen Digipeater geleitet (RECV_DIGI), sonst kam es ohne jeden
// Zwischenschritt direkt von der sendenden Station (RECV_DIRECT).
static bool aprsHandlePacket(uint8_t *d, size_t len) {
  aprsStripHeader(d, len);
  if (len == 0 || len >= 200) return false;
  d[len] = 0;                               // als C-String behandeln (Aufrufer reserviert 1 Byte mehr)
  char *info = strchr((char *)d, ':');
  if (!info) return false;
  char call[12];
  if (!aprsParseCall((char *)d, call, sizeof(call))) return false;
  char *gt = strchr((char *)d, '>');
  bool pathDigi = false;
  if (gt) for (char *p = gt + 1; p < info; p++) if (*p == '*') { pathDigi = true; break; }
  char *payload = info + 1;
  if (payload[0] == '}') {
    char *inner = payload + 1;
    char *innerInfo = strchr(inner, ':');
    if (!innerInfo) return false;
    char innerCall[12];
    if (!aprsParseCall(inner, innerCall, sizeof(innerCall))) return false;
    return aprsStoreFromInfo(innerCall, innerInfo + 1, call, RECV_IGATE);
  }
  return aprsStoreFromInfo(call, payload, nullptr, pathDigi ? RECV_DIGI : RECV_DIRECT);
}

// Wird in jeder loop()-Runde im LoRa-Modus aufgerufen (statt des RS41-Suchlaufs): auf ein Paket pruefen,
// dekodieren, naechsten Empfang anstossen. gpsPoll()/handleInput()/gfxUpdate() macht loop() schon davor.
// Jedes vom Funkmodul mit korrektem CRC abgelieferte Paket wird geloggt (Laenge, RSSI, SNR, Rohvorschau),
// unabhaengig davon, ob aprsHandlePacket() daraus eine Station machen kann -- einzige Moeglichkeit,
// "es kommt ueberhaupt etwas auf dieser Frequenz/mit diesen Parametern an" von "nichts kommt an" zu
// unterscheiden (siehe aprsRxTotal/aprsRxOk, angezeigt in der Stationen-Ansicht und /malt).
static void aprsPoll() {
  {   // Altstationen aussortieren (aprsPrune()) -- unabhaengig davon, ob gerade ein Paket da ist: sonst wuerde
      // eine Station, nach der lange nichts mehr empfangen wird, nie verschwinden. Alle 10 s reicht, aprsN
      // ist mit max. APRS_MAX = 16 Stationen ohnehin winzig.
    static uint32_t lastPrune = 0;
    uint32_t nowP = millis();
    if (nowP - lastPrune >= 10000) { lastPrune = nowP; aprsPrune(); }
  }
  if (!rxFlag) return;
  rxFlag = false;
  static uint8_t buf[256];
  size_t len = radio.getPacketLength();
  if (len > 0 && len < sizeof(buf) - 1) {
    int st = radio.readData(buf, len);
    float rssi = radio.getRSSI(true);
    float snr = radio.getSNR();
    if (st == RADIOLIB_ERR_NONE) {
      aprsRxTotal++;
      aprsLastRxMs = millis();
      aprsLastRssi = rssi;
      aprsLastSnr = snr;
      bool ok = aprsHandlePacket(buf, len);
      if (ok) aprsRxOk++;
      else {
        char prev[101];          // grosszuegiger als zu Beginn (4.19): auch verschachtelter Fremdverkehr
        aprsRawPreview(buf, len, prev, sizeof(prev));   // (siehe aprsHandlePacket()) kann lang werden
        char m[180];
        snprintf(m, sizeof(m), "LoRa-APRS RX #%lu (nicht geparst): %u Byte, RSSI %.1f dBm, SNR %.1f dB: \"%s\"",
                 (unsigned long)aprsRxTotal, (unsigned)len, rssi, snr, prev);
        logLine(m);
      }
    } else {
      char m[64];
      snprintf(m, sizeof(m), "LoRa-APRS RX: CRC-/Lesefehler (%d), RSSI %.1f dBm, SNR %.1f dB", st, rssi, snr);
      logLine(m);
    }
  }
  radio.startReceive();
}

// Dezimalgrad -> KOMPRIMIERTES APRS-Positionsformat (APRS101.pdf Kapitel 9), statt der bisherigen
// unkomprimierten "DDMM.mmN/DDDMM.mmE"-Darstellung: 13 statt 20 Byte fuer Position+Symbol -- bei LoRa
// direkt kuerzere Sendezeit/Kanalbelegung (passt zum Wunsch nach haeufigeren Beacons ohne zu fluten, siehe
// APRS_BEACON_*), und mit ca. 60 cm Aufloesung zudem feiner als die bisherigen zwei Nachkommastellen der
// Bogenminuten (ca. 18 m). Aufbau: Symboltabelle(1) + Breite(4) + Laenge(4) je Base91 + Symbolcode(1) +
// Kurs/Tempo(2, optional -- Leerzeichen wenn kein aktueller Kurs bekannt) + Kompressions-Typ-Byte(1).
// Kurs/Tempo zeigen anderen Stationen jetzt zusaetzlich Fahrtrichtung/Geschwindigkeit an (vorher gar nicht
// gesendet) -- das Kompressions-Typ-Byte selbst ist rein informativ (Quelle/Fix-Alter) und hat keinen
// Einfluss auf die decodierte Position, daher hier fest auf "komprimiert, GGA, aktueller Fix" gesetzt.
static void aprsEncodeCompressed(double lat, double lon, char symCode, bool haveCourse, float courseDeg,
                                  float speedKmh, char *out, size_t n) {
  long latVal = lround((90.0 - lat) * 380926.0);
  if (latVal < 0) latVal = 0; if (latVal > 91L * 91 * 91 * 91 - 1) latVal = 91L * 91 * 91 * 91 - 1;
  long lonVal = lround((180.0 + lon) * 190463.0);
  if (lonVal < 0) lonVal = 0; if (lonVal > 91L * 91 * 91 * 91 - 1) lonVal = 91L * 91 * 91 * 91 - 1;
  char latC[4], lonC[4];
  for (int i = 3; i >= 0; i--) { latC[i] = (char)(33 + latVal % 91); latVal /= 91; }
  for (int i = 3; i >= 0; i--) { lonC[i] = (char)(33 + lonVal % 91); lonVal /= 91; }
  char c = ' ', sp = ' ';
  if (haveCourse) {
    int ci = (int)lroundf(courseDeg / 4.0f);
    if (ci < 0) ci = 0; if (ci > 89) ci = 89;
    float knots = speedKmh / 1.852f;
    int si = (knots > 0.0f) ? (int)lroundf(logf(knots + 1.0f) / logf(1.08f)) : 0;
    if (si < 0) si = 0; if (si > 89) si = 89;
    c = (char)(33 + ci);
    sp = (char)(33 + si);
  }
  snprintf(out, n, "/%c%c%c%c%c%c%c%c%c%c%cQ", latC[0], latC[1], latC[2], latC[3],
           lonC[0], lonC[1], lonC[2], lonC[3], symCode, c, sp);   // 'Q' = Typ-Byte fuer "komprimiert, GGA, aktueller Fix"
}

// Eigene Position per LoRa-APRS senden (Menue A -> "LoRa Upload", nur im LoRa-Modus, siehe loraTxOn) --
// ausschliesslich ueber das Funkmodul, keine Internetverbindung beteiligt (der SondeHub-Upload selbst
// bleibt unberuehrt, siehe menuValue()/menuStep() MI_SHUB). Unterbricht kurz den Empfang, da der SX1262
// nicht gleichzeitig senden und empfangen kann.
static bool aprsSendBeacon() {
  double lat, lon;
  if (!homePosition(lat, lon)) return false;   // kein aktueller GPS-Fix -- lieber nicht mit alter Position senden
  if (!gpsFixGoodEnoughToSend()) return false; // Fix da, aber Guete (Sat-Minimum/HDOP) nicht ausreichend -- lieber warten
                                                 // (aprsBeaconTick() versucht es jetzt bald wieder, siehe dort)
  char pos[16];
  // Kurs/Tempo nur mit hineinkodieren, wenn gerade ein frischer, brauchbarer Fix vorliegt (siehe oben) UND
  // RMC schon einmal einen gueltigen Kurs gemeldet hat (courseDeg-Sentinel -1, siehe GpsState) -- sonst
  // bleiben die beiden Bytes auf Leerzeichen (= "kein Kurs/Tempo"), ganz normal fuer APRS.
  bool haveCourse = gps.courseDeg >= 0.0f;
  aprsEncodeCompressed(lat, lon, aprsSymbolChars[aprsSymbolIdx % NSYMBOL], haveCourse, gps.courseDeg, gps.speedKmh, pos, sizeof(pos));
  char altTxt[16] = "";
  if (gps.fix) snprintf(altTxt, sizeof(altTxt), "/A=%06ld", lround(gps.alt * 3.28084));   // APRS-Standardfeld,
                                                                                            // Hoehe in Fuss direkt
                                                                                            // hinter Position/Symbol
  // Rohpayload braucht denselben 3-Byte-Header 0x3C/0xFF/0x01 vor der TNC2-Textzeile, den aprsStripHeader()
  // beim Empfang abschneidet (verbreitete LoRa-APRS-Konvention, u. a. vom OE5BPA-iGate erwartet) -- ohne ihn
  // wurde bisher zwar erfolgreich GESENDET (radio.transmit() lieferte Status 0), aber vom iGate vermutlich
  // gar nicht als APRS-Paket erkannt und verworfen.
  uint8_t pkt[96];
  pkt[0] = 0x3C; pkt[1] = 0xFF; pkt[2] = 0x01;
  int tlen = snprintf((char *)pkt + 3, sizeof(pkt) - 3, "%s>APRS,WIDE1-1:=%s%sder TJ unterwegs - T-Deck plus v%s",
                       FW_CALLSIGN, pos, altTxt, FW_VERSION);
  int len = 3 + tlen;
  radio.standby();
  radio.setOutputPower(txPowerDbm);           // Menue D (MI_TXPWR) -- radio.begin() setzt bei jedem Moduswechsel
                                                // wieder den RadioLib-Default (10 dBm), deshalb hier jedesmal neu
  int st = radio.transmit(pkt, (size_t)len);
  char m[160];
  // Symbolname lesbar mit ins Log (vorher nur als einzelnes, kodiertes Zeichen im Rohpaket zu erkennen) --
  // damit auf einen Blick klar ist, mit welcher "Fahrzeugkennung" (Stationaer/Fussgaenger/Fahrrad/Bus) gerade
  // gesendet wurde.
  snprintf(m, sizeof(m), "LoRa-APRS gesendet (%s): %s (Status %d)", aprsSymbolNames[aprsSymbolIdx % NSYMBOL], (char *)pkt + 3, st);
  logLine(m);
  if (beepSec != 0) beepPlay();               // gleicher Empfangston/gleiche Einstellung wie beim Sondenfund (Menue B/MI_BEEP) --
                                                // "aus" bleibt stumm, sonst piept jeder gesendete eigene Beacon einmal
  rxFlag = false;
  radio.startReceive();                       // zurueck in Empfangsbereitschaft
  return true;
}

// Sendeintervall dynamisch nach Tempo (SmartBeaconing-Prinzip): im Stand reicht selten (Akku/Kanal schonen),
// schnell unterwegs soll die eigene Position auf der Karte der anderen nicht zu grob hinter der Realitaet
// zurueckbleiben. Zusaetzlich (unabhaengig vom Timer): sobald seit dem letzten Beacon APRS_BEACON_DIST_M
// zurueckgelegt wurden, wird sofort gesendet -- je nachdem, was zuerst zutrifft.
// MED von 2 Min. auf 1 Min. und die 1-km-Wegstrecke-Ausloesung (>10 km/h) auf 500 m verkuerzt: bei Fahrt
// kommen die Positionen jetzt spuerbar oefter, ohne die auf einem gemeinsamen RF-/LoRa-Kanal uebliche
// Untergrenze von ca. 1 Beacon/Minute zu unterschreiten -- FAST (>50 km/h, alle 30 s) und SLOW/Stand
// (alle 30 min) bleiben unveraendert, die sind schon an dieser Untergrenze bzw. bewusst selten.
#define APRS_BEACON_SLOW_MS   (30UL * 60UL * 1000UL)   // Stand/langsam (<=10 km/h): alle 30 min
#define APRS_BEACON_MED_MS    (1UL * 60UL * 1000UL)    // >10 km/h: alle 1 min
#define APRS_BEACON_FAST_MS   (30UL * 1000UL)          // >50 km/h: alle 30 s
#define APRS_BEACON_DIST_WALK_M  100.0                 // <=10 km/h (zu Fuss): zusaetzlich sofort nach 100 m Wegstrecke
#define APRS_BEACON_DIST_FAST_M 500.0                  // >10 km/h: zusaetzlich sofort nach 500 m Wegstrecke
#define APRS_BEACON_RETRY_MS   (5UL * 1000UL)           // siehe aprsBeaconTick(): Fix bei Faelligkeit nicht gut
                                                          // genug -- bald wieder versuchen statt ein volles
                                                          // Intervall (im Stand 30 min!) zu verschenken

// Automatische Wahl des APRS-Symbols nach Tempo (siehe aprsSymbolChars[]/aprsSymbolNames[]/aprsSymbolMinKmh[] oben):
// die hoechste Stufe, deren Schwelle das aktuelle Tempo erreicht, ist die Zielstufe. Umgeschaltet wird aber erst,
// wenn diese Zielstufe APRS_SYMBOL_HYST_MS lang ununterbrochen anliegt -- sonst wuerde z. B. an einer Ampel
// (kurz abgebremst von Bus- auf Fussgaenger-Tempo) staendig das Symbol hin- und herspringen.
#define APRS_SYMBOL_HYST_MS (1UL * 60UL * 1000UL)   // 1 Minute (vorher 2)
static void aprsSymbolTick() {
  float v = effSpeedKmh();
  uint8_t target = 0;
  for (int k = NSYMBOL - 1; k >= 0; k--) {
    if (v > aprsSymbolMinKmh[k]) { target = (uint8_t)k; break; }
  }
  uint32_t now = millis();
  if (target != aprsSymbolCand) { aprsSymbolCand = target; aprsSymbolCandSinceMs = now; }
  if (aprsSymbolCand != aprsSymbolIdx && now - aprsSymbolCandSinceMs >= APRS_SYMBOL_HYST_MS) {
    aprsSymbolIdx = aprsSymbolCand;
  }
}

// Periodischer LoRa-Beacon im Hintergrund -- nur im LoRa-Modus mit eingeschaltetem Upload (Menue A).
static void aprsBeaconTick() {
  if (radioMode != 1 || !loraTxOn) return;
  uint32_t now = millis();
  double lat, lon;
  bool haveFix = homePosition(lat, lon);
  float v = effSpeedKmh();
  uint32_t interval = (v > 50.0f) ? APRS_BEACON_FAST_MS : (v > 10.0f) ? APRS_BEACON_MED_MS : APRS_BEACON_SLOW_MS;
  double distThreshM = (v > 10.0f) ? APRS_BEACON_DIST_FAST_M : APRS_BEACON_DIST_WALK_M;
  bool dueByTime = (int32_t)(now - aprsBeaconNextMs) >= 0;
  bool dueByDist = haveFix && aprsBeaconLastSet &&
                   haversineKm(aprsBeaconLastLat, aprsBeaconLastLon, lat, lon) * 1000.0 >= distThreshM;
  if (!dueByTime && !dueByDist) return;
  // BUG behoben: hier wurde bisher IMMER neu getaktet (aprsBeaconNextMs = now + interval, ggf. 30 min im
  // Stand), sobald Zeit/Wegstrecke "faellig" waren -- auch wenn aprsSendBeacon() mangels gueltigem/gutem
  // Fix in genau diesem Moment gar nichts gesendet hat (das passiert lautlos, ohne Log-Zeile). Ein einzelner
  // knapper HDOP-/Sat-Ausreisser genau im Faelligkeits-Moment hat dadurch den naechsten Versuch um ein
  // ganzes Intervall verschoben, voellig spurlos im Log -- beobachtet als mehrminuetige Sendeluecken beim
  // Radfahren und eine 2,5-stuendige Luecke im Stand. Jetzt nur bei tatsaechlichem Erfolg so weit neu
  // takten, sonst bald (APRS_BEACON_RETRY_MS) erneut versuchen.
  if (aprsSendBeacon()) {
    aprsBeaconNextMs = now + interval;
    if (haveFix) { aprsBeaconLastLat = lat; aprsBeaconLastLon = lon; aprsBeaconLastSet = true; }
  } else {
    aprsBeaconNextMs = now + APRS_BEACON_RETRY_MS;
  }
}

// ---------- Frame auswerten und anzeigen ----------
bool trackSonde(float fMHz) {
  uint32_t lastOk = millis();
  bool everGot = false;
  int unkCount = 0;                               // Frames ohne lesbare ID bei aktivem Sondenfilter
  // Erster Versuch: nur ein Zeitfenster; danach dranbleiben, bis 5 s lang nichts mehr kommt
  while (millis() - lastOk < (everGot ? (uint32_t)LOST_MS : (uint32_t)(ACQ_MS - 50))) {
    gpsPoll();
    handleInput();
    if (restartRx) return everGot;                           // Frequenz/Suchmodus im Menue geaendert (loop() setzt es zurueck)
    line(12, String("Lausche ") + String(fMHz, 3) + " MHz");
    if (!captureHeader(fMHz, everGot ? 2500 : ACQ_MS)) {
      if (mapMode) gfxUpdate();                      // Punktfarbe / eigene Position aktualisieren (Empfang bleibt aus)
      continue;
    }
    everGot = true;
    lastOk = millis();
    lastRxMs = lastOk;
    lastRxFreq = fMHz;                        // fuer die Anzeige im Menue F (letzte empfangene Sonde)
    float pktRssi = radio.getRSSI(true);      // RSSI des gerade empfangenen Pakets
    frameCount++;
    VLOG("\n=== Frame %lu | %.3f MHz | Paket-RSSI %.1f dBm | t=%lu ms ===\n",
                  (unsigned long)frameCount, fMHz, pktRssi, (unsigned long)millis());
    hexDump("RAW Schritt 1 (wie empfangen)", raw1, CAP1_LEN, 8);

    memset(fr, 0, sizeof(fr));
    unscramble(raw1, CAP1_LEN, 8);
    hexDump("ENTWUERFELT Schritt 1", &fr[8], CAP1_LEN, 8);
    VLOG("Frametyp @0x038: %02X\n", fr[0x38]);

    // Bloecke ab 0x39 durchlaufen
    String blocks = "";
    int nFix = 0;                                 // per 1-Bit-Korrektur reparierte Bloecke in diesem Frame
    int pGps = -1;
    int p = 0x39, endPos = 8 + CAP1_LEN;
    while (p + 4 <= endPos) {
      uint8_t id = fr[p], len = fr[p + 1];
      if (id < 0x76 || id > 0x8F || p + len + 4 > endPos) break;
      int r = blockCrcFix(p);
      bool ok = (r > 0);
      if (r == 2) nFix++;
      {
        uint16_t crcSt = fr[p + 2 + len] | ((uint16_t)fr[p + 3 + len] << 8);
        VLOG("Block @0x%03X ID %02X Laenge %3u CRC gespeichert %04X berechnet %04X -> %s\n",
                      p, id, len, crcSt, crc16(&fr[p + 2], len), r == 2 ? "KORRIGIERT" : (ok ? "OK" : "FALSCH"));
      }
      char b[8];
      snprintf(b, sizeof(b), "%02X%c ", id, r == 2 ? '*' : (ok ? '+' : '-'));
      blocks += b;
      if (id == 0x7C && ok) pGps = p;
      p += len + 4;
    }

    if (p + 2 <= endPos)
      VLOG("Naechster Block @0x%03X: ID %02X Laenge %u (Rest ausserhalb des Fensters)\n", p, fr[p], fr[p + 1]);

    bool stOk = (fr[0x39] == 0x79) && blockCrc(0x39);
    char sid[9] = "????????";
    int fnr = -1;
    if (stOk)
      for (int i = 0; i < 8; i++)
        if (!isalnum(fr[0x3D + i])) stOk = false;   // Schutz gegen Fehlkorrekturen: ID besteht nur aus Buchstaben/Ziffern
    if (stOk) {
      fnr = fr[0x3B] | (fr[0x3C] << 8);
      for (int i = 0; i < 8; i++) sid[i] = (fr[0x3D + i] >= 0x20 && fr[0x3D + i] < 0x7F) ? fr[0x3D + i] : '?';
    }
    if (stOk && prevPosId[0] && strncmp(prevPosId, sid, 8) != 0) prevPosValid = false;   // andere Sonde: Sprungtest neu beginnen
    if (stOk) { strncpy(prevPosId, sid, 8); prevPosId[8] = 0; }

    if (filterOn) {                               // nur die gewaehlte Sonden-ID akzeptieren
      if (stOk && strncmp(sid, filterId, 8) != 0) {          // fremde Sonde erkannt
        char m[64];
        snprintf(m, sizeof(m), "Fremde Sonde %s bei %.3f MHz ignoriert (Filter %s)", sid, fMHz, filterId);
        logLine(m);
        foreignF = fMHz;
        foreignUntil = millis() + 90000UL;
        return false;                             // zum naechsten Kandidaten
      }
      if (stOk) {
        okF = fMHz;                               // Filter-ID bestaetigt
        unkCount = 0;
      } else if (!(okF > 0 && fabsf(fMHz - okF) < 0.03f)) {  // ID nicht lesbar und Frequenz nicht bestaetigt
        if (++unkCount >= 8) { foreignF = fMHz; foreignUntil = millis() + 30000UL; return false; }
        continue;                                 // Frame verwerfen, weiter lauschen
      }
    }
    // Kein Beep hier mehr: Schritt 1 (dieser Block) allein bedeutet nur "ein Paket wurde angenommen", nicht
    // "die Sonde wird angezeigt" -- das haengt noch von Schritt 2 (Position, weiter unten) ab. Vorher piepste
    // es schon hier, auch wenn Schritt 2 danach scheiterte -- irritierend ("Empfangston", aber keine Anzeige).
    line(36, String("Frames: ") + frameCount + "  ID: " + sid + "  Nr: " + fnr, stOk ? ST77XX_GREEN : ST77XX_ORANGE);
    line(48, String("Bl: ") + blocks, ST77XX_YELLOW);

    if (pGps >= 0) {
      static const char *tage[7] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
      uint16_t week = fr[pGps + 2] | (fr[pGps + 3] << 8);
      uint32_t s = (uint32_t)le32(pGps + 4) / 1000;
      char t[48];
      snprintf(t, sizeof(t), "GPS-Zeit: Wo %u %s %02u:%02u:%02u", week, tage[(s / 86400) % 7],
               (unsigned)((s % 86400) / 3600), (unsigned)((s % 3600) / 60), (unsigned)(s % 60));
      line(60, t);
    } else {
      line(60, "GPS-Zeit: n/a", ST77XX_ORANGE);
    }

    // ---- Schritt 2: Rest des NAECHSTEN Frames ab Block 0x7D ----
    // Frame N+1 beginnt ca. 687 ms nach dem Ende von Schritt 1, Block 0x7D ca. 300 ms spaeter.
    bool got = false;
    uint32_t sinceEnd = millis() - lastOk;
    if (sinceEnd < 900) {
      while (millis() - lastOk < 700) { gpsPoll(); handleInput(); delay(1); }   // nicht zu frueh scharf stellen (weniger Fehltreffer)
      armBlockPos();
      got = waitPacket(raw2, CAP2_LEN, 600);
    }
    VLOG("Schritt 2 (Sync %02X %02X fuer Block @0x%03X im naechsten Frame): empfangen: %s (Start %lu ms nach Schritt 1)\n",
                  syncPos[0], syncPos[1], SYNC2_POS, got ? "ja" : "nein", (unsigned long)sinceEnd);

    bool havePos = false;
    double lat = 0, lon = 0, alt = 0;
    int32_t ex = 0, ey = 0, ez = 0;
    int nsv = -1;              // Satellitenzahl der Sonde (Byte 0x126 im Positionsblock), -1 = kein Block
    bool posBlock = false;     // Positionsblock 0x7B mit gueltiger CRC empfangen
    bool posRejected = false;  // Position war unplausibel (Sprung, Hoehe) und wurde verworfen
    String blocks2 = "";
    if (got) {
      fr[SYNC2_POS] = SYNC2_ID;
      fr[SYNC2_POS + 1] = SYNC2_LEN;
      hexDump("RAW Schritt 2 (wie empfangen)", raw2, CAP2_LEN, SYNC2_POS + 2);
      unscramble(raw2, CAP2_LEN, SYNC2_POS + 2);
      hexDump("ENTWUERFELT Schritt 2 (ab Block-ID)", &fr[SYNC2_POS], CAP2_LEN + 2, SYNC2_POS);
      int q = SYNC2_POS, pPos = -1;
      while (q + 4 <= FRAME_LEN) {
        uint8_t id = fr[q], len = fr[q + 1];
        if (id < 0x76 || id > 0x8F || q + len + 4 > FRAME_LEN) {
          VLOG("Blockkette endet @0x%03X (ID %02X Laenge %u)\n", q, id, len);
          break;
        }
        int r = blockCrcFix(q);
        bool ok = (r > 0);
        if (r == 2) nFix++;
        uint16_t crcSt = fr[q + 2 + len] | ((uint16_t)fr[q + 3 + len] << 8);
        VLOG("Block @0x%03X ID %02X Laenge %3u CRC gespeichert %04X berechnet %04X -> %s\n",
                      q, id, len, crcSt, crc16(&fr[q + 2], len), r == 2 ? "KORRIGIERT" : (ok ? "OK" : "FALSCH"));
        char b[8];
        snprintf(b, sizeof(b), "%02X%c ", id, r == 2 ? '*' : (ok ? '+' : '-'));
        blocks2 += b;
        if (id == 0x7B && ok) pPos = q;
        q += len + 4;
      }
      if (pPos >= 0) {
        posBlock = true;
        ex = le32(pPos + 2); ey = le32(pPos + 6); ez = le32(pPos + 10);
        nsv = fr[pPos + 20];                        // Datenbyte 18 = Satellitenzahl
        if (nsv < SV_MIN || (ex == 0 && ey == 0 && ez == 0)) {
          VLOG("Positionsblock CRC ok, aber Sonde ohne GPS-Fix (Satelliten %d)\n", nsv);
        } else {
          double la2, lo2, al2;
          ecef2lla(ex / 100.0, ey / 100.0, ez / 100.0, la2, lo2, al2);
          bool okPos = (al2 > -500.0 && al2 < 45000.0 && nsv <= 32);
          uint32_t nowP = millis();
          if (okPos && prevPosValid && nowP - prevPosMs < 120000UL) {      // Sprungtest gegen die letzte gueltige Position
            double dtS = (nowP - prevPosMs) / 1000.0;
            double dm = haversineKm(prevPosLat, prevPosLon, la2, lo2) * 1000.0;
            if (dm > 100.0 * dtS + 500.0) okPos = false;                   // mehr als ca. 360 km/h + 500 m: unplausibel
          }
          if (!okPos) {
            posRejected = true;
            VLOG("Position verworfen (unplausibel): %.5f %.5f %.0f m\n", la2, lo2, al2);
          } else {
            havePos = true;
            lat = la2; lon = lo2; alt = al2;
            prevPosValid = true; prevPosLat = lat; prevPosLon = lon; prevPosMs = nowP;
            {   // ECEF-Geschwindigkeit (cm/s, am Flug bestaetigt) -> Ost/Nord/Oben
              double ve, vn, vu;
              ecefVelToEnu(lat, lon, le16(pPos + 14) / 100.0, le16(pPos + 16) / 100.0, le16(pPos + 18) / 100.0, ve, vn, vu);
              sondeVh = (float)sqrt(ve * ve + vn * vn);
              sondeDir = (float)fmod(atan2(ve, vn) * 180.0 / M_PI + 360.0, 360.0);
              sondeVz = (float)vu;
              haveVel = true;
              if (!predVelInit) {                     // erster Fix dieser Sonde: ungeglaettet uebernehmen (kein Anlaufen)
                predVe = (float)ve; predVn = (float)vn; predVz = (float)vu;
                predVelInit = true;
              } else {                                // danach: Tiefpass, nur fuer die Landevorhersage
                predVe += PRED_SMOOTH_IN_ALPHA * ((float)ve - predVe);
                predVn += PRED_SMOOTH_IN_ALPHA * ((float)vn - predVn);
                predVz += PRED_SMOOTH_IN_ALPHA * ((float)vu - predVz);
              }
            }
          }
        }
      } else {
        VLOGLN("Kein gueltiger Block 0x7B in Schritt 2 gefunden");
      }
    }
    line(48, String("Bl: ") + blocks + blocks2, ST77XX_YELLOW);
    if (havePos) {
      lastPosOkMs = millis();                        // fuer die Punktfarbe: nur eine ECHTE Position gilt als "Empfang ok"
      if (beepSec == 1) beepPlay();                 // Empfangston erst hier: Schritt 1 alleine zeigt noch keine
                                                      // Sonde an, das haengt von Schritt 2 (Position) ab -- vorher
                                                      // piepste es schon nach Schritt 1, auch ohne Anzeige danach
      line(72, String("Pos: ") + String(lat, 5) + "  " + String(lon, 5), ST77XX_GREEN);
      line(84, String("H ") + String(alt, 0) + " m  Steig " + String(sondeVz, 1) + " m/s  Tempo " + String(sondeVh * 3.6, 0) + " km/h " + String(sondeDir, 0) + " Grad", ST77XX_GREEN);
    } else {
      const char *reason = !got ? "Schritt 2 nicht empfangen" : (posRejected ? "unplausibel verworfen" : (posBlock ? "Sonde ohne GPS-Fix" : "kein Block 0x7B"));
      line(72, String("Pos: ") + reason, ST77XX_ORANGE);
      if (posBlock) line(84, String("Satelliten der Sonde: ") + nsv);
      else line(84, "");
      // Rohdaten mitloggen: Schritt 1 wurde angenommen (sonst waeren wir nicht hier), aber es gibt keine
      // Sondenanzeige -- genau der Fall, der bisher trotzdem den Empfangston ausgeloest hat. Damit sich im
      // Nachhinein pruefen laesst, ob das am Funkempfang liegt oder noch ein Decodierfehler steckt.
      {
        String m = String("RX ohne Anzeige (") + reason + ") f=" + String(fMHz, 3) + " ID=" + sid + " raw1=";
        char hb[3];
        for (int i = 0; i < CAP1_LEN; i++) { snprintf(hb, sizeof(hb), "%02X", raw1[i]); m += hb; }
        if (got) {
          m += " raw2=";
          for (int i = 0; i < CAP2_LEN; i++) { snprintf(hb, sizeof(hb), "%02X", raw2[i]); m += hb; }
        }
        logLine(m.c_str());
      }
    }
    {
      double hl, ho;
      bool gf = homePosition(hl, ho);
      line(96, String("Ich: ") + String(hl, 5) + " " + String(ho, 5) + (gf ? "  GPS " : "  fest ") + gps.sats + " Sat");
    }
    if (havePos) {                                // Sondendaten fuer die Kartenansicht merken
      if (fabsf(fMHz - freqMHz) > 0.0005f) {       // Paket vollstaendig decodiert: Frequenz merken, aber nur bei
        freqMHz = fMHz;                             // Aenderung in den Flash schreiben (schont NVS, meist dieselbe
        saveSettings();                             // Sonde/Frequenz ueber die ganze Sitzung)
      }
      bool firstPos = !haveSondePos;
      sondeLat = lat; sondeLon = lon; sondeAlt = alt;
      altTrackAdd(alt);                           // Hoehentrack-Ansicht, siehe altTrackUpdate()
      sondeRssi = pktRssi;
      if (stOk) {                                 // ID/Framenummer nur uebernehmen, wenn der Statusblock gueltig war (schwaches Signal)
        sondeFnr = fnr;
        strncpy(sondeId, sid, 8);
        sondeId[8] = 0;
      } else if (sondeId[0] == 0) {
        strcpy(sondeId, "????????");
      }
      haveSondePos = true;
      if (firstPos || (stOk && strncmp(viewIdSeen, sid, 8) != 0)) {   // erste bzw. neue Sonde: Karte wieder automatisch (Sonde+Ich)
        sondeFoundSeq++;                            // fuer den Ton auf der Weboberflaeche, siehe /mstat
        altTrackReset();                            // Hoehentrack beginnt mit der neuen Sonde neu
        if (mapManual) {
          char m[64];
          snprintf(m, sizeof(m), "Karte: automatisch (%s)", firstPos ? "erste Sondenposition" : "neue Sonden-ID");
          logLine(m);
        }
        if (stOk) { strncpy(viewIdSeen, sid, 8); viewIdSeen[8] = 0; }
        predVelInit = false;                        // neue Sonde: Glaettung fuer die Landevorhersage neu anlaufen lassen
        predPtInit = false;
        onlinePredValid = false;                    // ... und eine evtl. vorhandene Online-Vorhersage verwerfen
        onlinePredNextMs = 0;                       // (gleich neu abrufbar, statt bis zum naechsten Takt zu warten)
        mapManual = false;
        rFocus = 0;
        viewZ = -1;
        gpxDrain(gpxSonde, gpxBufSonde, gpxCountSonde, gpxSondeId, gpxSondeName);   // Rest der alten Sonde-Datei erst sichern ...
        gpxCloseTrack(gpxSonde);                // ... dann: naechster Punkt oeffnet die Datei der neuen Sonde
        gpxSafeId(sondeId, gpxSondeId, sizeof(gpxSondeId));
        strncpy(gpxSondeName, sondeId, sizeof(gpxSondeName) - 1);
        gpxSondeName[sizeof(gpxSondeName) - 1] = 0;
        findPickRecentNote(gpxSondeId);                // merken fuer Taste Q (schnelle Auswahl ohne SD-Scan)
      }
      if (strcmp(sondeId, "????????") != 0) {    // unbekannte Sonde (ID noch nie per Statusblock bestaetigt):
        gpxQueue(gpxSonde, gpxBufSonde, gpxCountSonde, gpxSondeId, gpxSondeName, lat, lon, (float)alt);   // kein GPX-Log, erst nach erfolgreicher ID-Erkennung (jede Position, ungekuerzt, im PSRAM gepuffert)
      }
      trailAdd(lat, lon, sondeId);
      predictLanding();
      // Kein SondeHub-Sonden-Upload mehr -- siehe Begruendung bei der SondeHub-Live-Upload-Definition weiter oben.
    }
    lastMapMs = 0;
    if (mapMode) gfxUpdate();

    {   // Eine Zeile pro Frame (seriell und auf SD): alles Wichtige zum Kopieren
      double hl, ho;
      bool gf = homePosition(hl, ho);
      uint16_t wk = 0;
      uint32_t tw = 0;
      if (pGps >= 0) { wk = fr[pGps + 2] | (fr[pGps + 3] << 8); tw = (uint32_t)le32(pGps + 4); }
      char ln[300], idl[12], tstr[12];
      timeText(tstr, sizeof(tstr));
      if (stOk) snprintf(idl, sizeof(idl), "%s", sid);
      else snprintf(idl, sizeof(idl), "%s?", sondeId[0] ? sondeId : "????????");
      int n = snprintf(ln, sizeof(ln), "t%lu %s F%lu %s Nr%d %.0fdBm f%.3f bw%.1f GPS%u/%lu Bl:%s%s", (unsigned long)(millis() / 1000),
                       tstr, (unsigned long)frameCount, idl, fnr, pktRssi, fMHz, rxBw, wk, (unsigned long)(tw / 1000), blocks.c_str(), blocks2.c_str());
      if (nFix) n += snprintf(ln + n, sizeof(ln) - n, "fx%d ", nFix);
      if (havePos) n += snprintf(ln + n, sizeof(ln) - n, "Pos %.6f %.6f %.0fm SV%d v%.0fkmh/%03.0f/%+.1fms ", lat, lon, alt, nsv,
                                 sondeVh * 3.6f, sondeDir, sondeVz);
      else if (posRejected) n += snprintf(ln + n, sizeof(ln) - n, "Pos verworfen ");
      else if (posBlock) n += snprintf(ln + n, sizeof(ln) - n, "Pos kein Fix (SV%d) ", nsv);
      else n += snprintf(ln + n, sizeof(ln) - n, "Pos -- (S2 %s) ", got ? "ja" : "nein");
      if (havePos && predValid) n += snprintf(ln + n, sizeof(ln) - n, "Land %lu:%02lu %.5f %.5f ", (unsigned long)(predEtaS / 60),
                                              (unsigned long)(predEtaS % 60), predLat, predLon);
      if (havePos && onlinePredValid) n += snprintf(ln + n, sizeof(ln) - n, "LandOnline %.5f %.5f ", onlinePredLat, onlinePredLon);
      n += snprintf(ln + n, sizeof(ln) - n, "Ich %.5f %.5f %s S%d", hl, ho, gf ? "GPS" : "fest", gps.sats);
      if (havePos) n += snprintf(ln + n, sizeof(ln) - n, " %.2fkm", haversineKm(hl, ho, lat, lon));
      n += snprintf(ln + n, sizeof(ln) - n, " Bat%.2fV/%d%%%s", batV, batPct, batChg ? "+" : "");
      if (lastMapMs) snprintf(ln + n, sizeof(ln) - n, " map%lums", (unsigned long)lastMapMs);
      logLine(ln);
    }
    VLOG("--- Zusammenfassung Frame %lu ---\n", (unsigned long)frameCount);
    VLOG("Sonden-ID: %s  Framenr: %d  Statusblock: %s\n", sid, fnr, stOk ? "OK" : "FALSCH");
    VLOG("Bloecke: %s%s\n", blocks.c_str(), blocks2.c_str());
    if (pGps >= 0) {
      uint16_t week = fr[pGps + 2] | (fr[pGps + 3] << 8);
      VLOG("GPS: Woche %u, TOW %lu ms\n", week, (unsigned long)le32(pGps + 4));
    } else {
      VLOGLN("GPS: kein gueltiger 0x7C-Block");
    }
    {
      double hl, ho;
      bool gf = homePosition(hl, ho);
      VLOG("Ich: %.6f %.6f (%s), GPS-Sat %d, %lu Bytes NMEA empfangen\n", hl, ho, gf ? "GPS-Fix" : "feste Ersatzposition",
                    gps.sats, (unsigned long)gps.bytes);
    }
    if (havePos) {
      VLOG("ECEF [cm]: X %ld Y %ld Z %ld\n", (long)ex, (long)ey, (long)ez);
      VLOG("Position: lat %.6f lon %.6f alt %.1f m\n", lat, lon, alt);
    } else {
      VLOGLN("Position: nicht verfuegbar");
    }
  }
  return everGot;
}

// Zeigt kurz nach dem Einschalten Rufzeichen, Versionsnummer und Compile-Datum/-Uhrzeit (__DATE__/__TIME__,
// vom Compiler automatisch bei jedem Build gesetzt -- so sieht man am Geraet, welcher Stand gerade laeuft,
// ohne den Sketch oeffnen zu muessen) gross und zentriert auf dem Display, bevor der normale Boot-Status
// (Radio/SD/GPS usw.) darunter durchlaeuft. Reine Information, keine Eingabe moeglich, blockiert kurz.
#define BOOT_SPLASH_MS 2500
static void bootCenter(Adafruit_GFX *g, const char *s, int y, uint16_t color, int size, bool bold) {
  g->setTextSize(size);
  int16_t x1, y1; uint16_t w, h;
  g->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  int cx = (320 - (int)w) / 2;
  g->setTextColor(color);
  g->setCursor(cx, y);
  g->print(s);
  if (bold) {               // Adafruit_GFX kennt kein Fett -- zweiter Druck 1 Px versetzt simuliert es
    g->setCursor(cx + 1, y);
    g->print(s);
  }
}
static void bootSplash() {
  Adafruit_GFX *g = scr();
  g->fillScreen(ST77XX_BLACK);
  g->setTextWrap(false);
  bootCenter(g, FW_CALLSIGN, 56, ST77XX_CYAN, 3, true);
  char title[24];
  snprintf(title, sizeof(title), "T-Deck RS41 v%s", FW_VERSION);
  bootCenter(g, title, 112, ST77XX_WHITE, 2, true);
  // __DATE__ kommt vom Compiler im Format "Mon dd yyyy" (engl. Monatsname, Tag ggf. mit
  // Leerzeichen statt 0 gepolstert) -- hier auf dd.mm.yyyy umgerechnet; __TIME__ ist bereits
  // "hh:mm:ss" und wird unverändert angehängt.
  static const char *const monNames[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char monStr[4]; int day = 1, year = 2026;
  sscanf(__DATE__, "%3s %d %d", monStr, &day, &year);
  int mon = 1;
  for (int i = 0; i < 12; i++) if (strncmp(monStr, monNames[i], 3) == 0) { mon = i + 1; break; }
  char dt[32];
  snprintf(dt, sizeof(dt), "%02d.%02d.%04d %s", day, mon, year, __TIME__);
  bootCenter(g, dt, 150, 0x8410, 2, false);
  if (frameCanvas) blit(frameCanvas);
  delay(BOOT_SPLASH_MS);
}

// Blockierende J/N-Abfrage beim Start, nur wenn radioMode == 1 (LoRa-APRS) und der LoRa-Beacon (loraTxWasOn,
// siehe loadSettings()) beim letzten Mal aktiv war: loraTxOn selbst steht nach jedem Neustart erst mal auf aus
// (s. Deklaration oben), damit nicht unbeabsichtigt gesendet wird -- hier kann man es aber bewusst wieder
// anschalten, statt jedesmal von Hand ins Menue zu muessen. J = wieder aktivieren (mit der eingestellten
// Sendeleistung, siehe txPowerDbm/MI_TXPWR -- aprsSendBeacon() wendet die ohnehin vor jedem Senden neu an),
// N = aus (bzw. ausgeschaltet lassen).
static void confirmLoraTxResume() {
  Adafruit_GFX *g = scr();
  g->fillScreen(ST77XX_BLACK);
  g->setTextWrap(false);
  g->setTextSize(2);
  g->setTextColor(ST77XX_YELLOW);
  g->setCursor(6, 14);
  g->print("LoRa senden war");
  g->setCursor(6, 36);
  g->print("aktiv. Wieder an?");
  g->setTextSize(1);
  g->setTextColor(ST77XX_CYAN);
  char pw[32];
  snprintf(pw, sizeof(pw), "Sendeleistung: %d dBm", txPowerDbm);
  g->setCursor(6, 62);
  g->print(pw);
  // Gross und nebeneinander, statt klein untereinander -- J (gruener Rahmen) links, N (roter Rahmen) rechts,
  // mit der Erklaerung direkt darunter in derselben Spalte.
  g->drawRoundRect(8, 78, 148, 110, 8, ST77XX_GREEN);
  g->setTextSize(5);
  g->setTextColor(ST77XX_GREEN);
  g->setCursor(58, 95);
  g->print("J");
  g->setTextSize(1);
  g->setTextColor(ST77XX_WHITE);
  g->setCursor(16, 160);
  g->print("ja, wieder an");
  g->drawRoundRect(164, 78, 148, 110, 8, ST77XX_RED);
  g->setTextSize(5);
  g->setTextColor(ST77XX_RED);
  g->setCursor(214, 95);
  g->print("N");
  g->setTextSize(1);
  g->setTextColor(ST77XX_WHITE);
  g->setCursor(178, 160);
  g->print("nein, aus lassen");
  if (frameCanvas) blit(frameCanvas);
  while (true) {
    int e;
    do { gpsPoll(); e = pollInput(); if (e == EV_NONE) delay(5); } while (e == EV_NONE);
    int c = tolower(e);
    if (c == 'j') {
      loraTxOn = true;
      aprsBeaconNextMs = 0; aprsBeaconLastSet = false;    // gleich die erste eigene Position senden, Wegstrecke-Anker neu
      saveSettings();
      return;
    }
    if (c == 'n') {
      loraTxOn = false;
      loraTxWasOn = false;                                // bewusst abgelehnt -- beim naechsten Start nicht wieder fragen
      saveSettings();
      return;
    }
  }
}

// ---------- Setup / Loop ----------
void setup() {
  // GPIO10 (PIN_POWER_ON) schaltet laut LilyGO die Stromversorgung der Peripherie (u. a. die I2C-Tastatur/
  // den Trackball, siehe weiter unten bei PIN_POWER_ON = LOW zum Abschalten). Bei einem echten Hardware-
  // Reset (EN-Pin, z. B. nach seriellem Upload) ist diese Leitung dabei kurz stromlos -- die Peripherie
  // bekommt also immer einen echten Power-Cycle. Bei einem reinen Software-Reset (ArduinoOTA/esp_restart(),
  // z. B. nach einem WLAN-Upload) kann GPIO10 dagegen durchgehend HIGH bleiben, die Peripherie wird NICHT
  // neu eingeschaltet -- siehe die Haenger-Diagnose vom 13.10. (Absturz in inputBegin()/I2C, reproduzierbar
  // nur nach WLAN-Upload). Darum hier ausdruecklich erst LOW, kurz warten, dann HIGH: erzwingt einen echten
  // Power-Cycle dieser Schiene bei JEDEM Start, unabhaengig von der Reset-Art.
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, LOW);
  delay(50);
  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_SD_CS, OUTPUT);     digitalWrite(PIN_SD_CS, HIGH);
  pinMode(PIN_LORA_CS, OUTPUT);   digitalWrite(PIN_LORA_CS, HIGH);
  pinMode(PIN_TFT_CS, OUTPUT);    digitalWrite(PIN_TFT_CS, HIGH);
  pinMode(PIN_TFT_BL, OUTPUT);    digitalWrite(PIN_TFT_BL, LOW);   // Beleuchtung ZU, siehe Begruendung unten
  delay(500);

  // Beleuchtung bleibt bewusst aus, bis der Bildschirm geloescht ist: das TFT-Panel haelt seinen zuletzt
  // gezeigten Inhalt (z. B. "Fertig, schalte ab ...") ueber einen Reset/Neustart hinweg selbst, und die
  // obige delay(500) + das spaetere Warten auf den Serial Monitor brauchen ihre Zeit. Waere die Beleuchtung
  // (wie vorher) schon vorher an, waere der alte Bildschirm so lange sichtbar. Reihenfolge jetzt: Display
  // initialisieren + loeschen + Splash zeichnen, und ERST DANACH die Beleuchtung einschalten.
  loadSettings();                               // gespeicherte Einstellungen (Menue) -- u. a. fuer die Helligkeit gleich
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
  tft.init(240, 320);
  tft.setRotation(3);
  tft.invertDisplay(TFT_INVERT);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  applyBrightness();                            // jetzt erst Beleuchtung an -- der Bildschirm ist schon schwarz
  bootSplash();                                 // Rufzeichen/Version/Compile-Zeit kurz gross anzeigen

  setenv("TZ", TZ_RULE, 1);                     // Ortszeit (fuer Dateizeiten und Logzeilen); die Uhr selbst stellt das GPS
  tzset();
  beepInit();                                    // Empfangston (Menue X)
  Serial.begin(115200);
  {                                   // bis zu 3 s auf den Serial Monitor warten, damit die Startmeldungen ankommen
    uint32_t tw = millis();
    while (!Serial && millis() - tw < 3000) delay(10);
  }
  delay(200);
  Serial.printf("\n##### T-Deck RS41 Empfang v%s gestartet #####\n", FW_VERSION);
  Serial.printf("Scan %.3f-%.3f MHz: grob %.3f MHz/%.0f kHz-Filter, fein %.0f kHz Schritt/%.1f kHz-Filter\n",
                SCAN_START, SCAN_END, COARSE_STEP, COARSE_BW, FINE_STEP * 1000, rxBw);
  Serial.printf("BR %.1f kbit/s, Hub %.1f kHz, TCXO %.1f V, Signal ab Median+%.1f dB\n",
                BITRATE, FREQ_DEV, TCXO_V, REL_MARGIN);

  bgCanvas = new GFXcanvas16(320, 240);         // Bildpuffer zuerst: alle Bildschirme laufen darueber (Screenshots)
  frameCanvas = new GFXcanvas16(320, 240);
  if (!bgCanvas->getBuffer() || !frameCanvas->getBuffer()) {
    Serial.println("Bildpuffer (2 x 150 kB) nicht allozierbar -> Karte, Hoehentrack/Stationen und Screenshots aus (PSRAM = OPI PSRAM eingestellt?)");
    delete bgCanvas; delete frameCanvas;
    bgCanvas = nullptr; frameCanvas = nullptr;
  }
  // Hoehentrack-Puffer (siehe altTrackAdd()), unabhaengig von bgCanvas/frameCanvas. Bewusst ueber
  // heap_caps_malloc(..., MALLOC_CAP_SPIRAM) statt "new[]": "new[]" bricht auf dem ESP32-Arduino-Kern bei
  // einem Fehlschlag per abort() ab (C++-Exceptions sind aus), statt wie erwartet nullptr zurueckzugeben --
  // das fuehrte zu einem sofortigen, logfreien Absturz. malloc/heap_caps_malloc geben bei Fehlschlag immer
  // nullptr zurueck, und MALLOC_CAP_SPIRAM erzwingt die Zuteilung im PSRAM statt im knappen internen RAM.
  altTrackAlt = (float *)heap_caps_malloc(ALT_TRACK_MAX * sizeof(float), MALLOC_CAP_SPIRAM);
  altTrackMs = (uint32_t *)heap_caps_malloc(ALT_TRACK_MAX * sizeof(uint32_t), MALLOC_CAP_SPIRAM);
  if (!altTrackAlt || !altTrackMs) {
    Serial.println("Hoehentrack-Puffer nicht allozierbar -> Hoehentrack-Ansicht bleibt leer");
    free(altTrackAlt); free(altTrackMs);
    altTrackAlt = nullptr; altTrackMs = nullptr;
  }
  if (frameCanvas) {
    for (bigMarginTiles = 2; bigMarginTiles >= 1; bigMarginTiles--) {   // grosser Kartenhintergrund mit Rand (schnelles
      int w = 320 + 2 * bigMarginTiles * 256, h = 240 + 2 * bigMarginTiles * 256;   // Trackball-Scrollen, siehe prepareMapBackground()):
      bigCanvas = new GFXcanvas16(w, h);                                            // so gross wie moeglich probieren, bei
      if (bigCanvas && bigCanvas->getBuffer()) { bigW = w; bigH = h; break; }        // Fehlschlag kleiner, zuletzt ganz aus
      delete bigCanvas; bigCanvas = nullptr;
    }
    if (!bigCanvas) {
      bigW = 0; bigH = 0;
      Serial.println("Grosser Kartenhintergrund nicht allozierbar (auch nicht mit 1 Kachel Rand) -> jeder Pan-Schritt "
                      "rendert wieder komplett neu (alter Weg ueber bgCanvas).");
    } else {
      Serial.printf("Grosser Kartenhintergrund: %d x %d Px (%d Kachel(n) Rand, %lu kB)\n", bigW, bigH, bigMarginTiles,
                    (unsigned long)((size_t)bigW * (size_t)bigH * 2UL / 1024UL));
    }
    bundleTileBuf = (uint8_t *)ps_malloc(BUNDLE_MAX_TILE);
    if (!bundleTileBuf) Serial.println("Bundle-Kachelpuffer (400 kB) nicht allozierbar -> gepackte Kartenkacheln "
                                        "werden uebersprungen, es wird direkt aus einzelnen Dateien gelesen.");
    gpxBufIch = (GpxPoint *)ps_malloc(GPX_BUF_CAP * sizeof(GpxPoint));
    gpxBufSonde = (GpxPoint *)ps_malloc(GPX_BUF_CAP * sizeof(GpxPoint));
    if (!gpxBufIch || !gpxBufSonde) { Serial.println("GPX-Zwischenspeicher nicht allozierbar -> GPX-Aufzeichnung aus."); gpxOn = false; }
  }
  scrClear();
  line(0, TITLE_TEXT, ST77XX_CYAN);

#if TILE_TEST
  tileSelfTest();
  scrClear();
  line(0, TITLE_TEXT, ST77XX_CYAN);
#endif

  int st = (radioMode == 1) ? aprsRadioInit() : sondeRadioInit();   // Menue I (radioMode), siehe loadSettings()
  Serial.printf("Radio-Init (%s) -> %d\n", radioMode == 1 ? "LoRa-APRS" : "RS41", st);
  if (st != RADIOLIB_ERR_NONE) haltWithError("Radio-Init fehlgeschlagen", st);

  if (radioMode == 0) {
    syncPos[0] = rev8(SYNC2_ID ^ MASK[SYNC2_POS & 63]);
    syncPos[1] = rev8(SYNC2_LEN ^ MASK[(SYNC2_POS + 1) & 63]);
    Serial.printf("Sync fuer Schritt 2: %02X %02X (Block @0x%03X, ID %02X, Laenge %02X)\n",
                  syncPos[0], syncPos[1], SYNC2_POS, SYNC2_ID, SYNC2_LEN);
  }
  line(12, "Radio OK", ST77XX_GREEN);
  batteryTick();

  line(24, sdInit() ? "SD-Karte OK" : "SD-Karte nicht lesbar (Karte aus)", sdReady ? ST77XX_GREEN : ST77XX_ORANGE);
  if (sdLogOn && sdReady) startLog();           // frisches Log fuer diesen Start, zunaechst unter Platzhaltername
                                                 // (Datum/Zeit folgt per onTimeSet(), sobald GPS/NTP eine Uhrzeit liefert)
  if (sdReady) {                                // welche Kartenarten liegen auf der Karte?
    for (int k = 1; k < NSTYLE; k++) styleAvail[k] = SD.exists(styleDir[k]);
    if (!styleAvail[mapStyle % NSTYLE]) mapStyle = 0;
    Serial.printf("Kartenarten: Rad ja, Luftbild %s, Topo %s, Relief %s, TopPlusOpen %s (aktiv: %d)\n", styleAvail[1] ? "ja" : "nein", styleAvail[2] ? "ja" : "nein", styleAvail[3] ? "ja" : "nein", styleAvail[4] ? "ja" : "nein", mapStyle);
  }
  line(36, "GPS: suche NMEA ...");
  gpsBegin();
  if (gps.bytes > 0) line(36, "");                                   // Zustand zeigt der Statusblock oben rechts
  else line(36, "GPS: keine NMEA-Daten", ST77XX_RED);               // nur bei Fehler bleibt eine Meldung stehen
  inputBegin();                                 // Tastatur (I2C) und Trackball
  // WiFi-Verbindungsaufbau bewusst erst HIER, nach GPS/Tastatur-I2C (siehe Fehlersuche vom 13.10. in
  // inputBegin()/gpio_reset_pin() im Changelog oben): WiFi.begin() zieht beim Einschalten des Radios kurz
  // deutlich mehr Strom, ein moeglicher Brownout-/Buskollisions-Kandidat bei Akku + Display + LoRa + SD + I2C
  // gleichzeitig aktiv. WiFi startet daher erst, nachdem die I2C-Tastatur/der Trackball sauber durch sind.
  if (wifiSsid[0]) {                                    // Heimnetz bekannt: gleich verbinden -- Fundmeldungen absetzen und/oder
    bool haveRecovery = sdReady && recoveryCount() > 0; // per NTP die Uhrzeit holen, bis das GPS selbst einen Fix hat; bleibt an,
    char m[80];                                         // solange die Verbindung steht (sonst mit am Strom, keine Akkusorge)
    if (haveRecovery) snprintf(m, sizeof(m), "%d Fundmeldung(en) in der Warteliste -> verbinde Heimnetz", recoveryCount());
    else snprintf(m, sizeof(m), "Verbinde mit Heimnetz (Fundmeldungen/Uhrzeit)");
    logLine(m);
    wifiSet(1);                                   // nur die Verbindung anstossen, laeuft nicht-blockierend weiter (wifiTick())
    ntpWanted = true;
    ntpConfigured = false;
  }
  // LoRa-Beacon war beim letzten Mal aktiv -- J/N abfragen, AUSSER der Neustart kam von einem Firmware-Upload
  // (ESP_RST_SW: Software-Neustart, z. B. durch ArduinoOTA/USB-Flash direkt vor diesem Boot). In dem Fall hat
  // sich an der Antennenkonstellation nichts geaendert (kein echter Stromzyklus/Hardware-Eingriff) -- also
  // direkt wieder einschalten wie bei "J", ohne nachzufragen.
  if (radioMode == 1 && loraTxWasOn) {
    if (esp_reset_reason() == ESP_RST_SW) {
      logLine("Neustart durch Firmware-Upload erkannt -- LoRa-Senden automatisch wieder aktiviert (J)");
      loraTxOn = true;
      aprsBeaconNextMs = 0; aprsBeaconLastSet = false;
      saveSettings();
    } else {
      confirmLoraTxResume();
    }
  }
  if (savedView != 0) {                           // zuletzt benutzte Ansicht wiederherstellen
    mapMode = true;
    gfxView = savedView - 1;
    showView();
  }
}

void loop() {
  restartRx = false;
  if (radioModeChanged) { radioModeChanged = false; radioApplyMode(); }
  gpsPoll();
  handleInput();
  if (mapMode) gfxUpdate();                          // Sonde ohne Empfang: Punkt wird gelb/rot; ohne Sonde: Karte um die eigene Position
  if (radioMode == 1) { aprsPoll(); return; }         // Menue I: LoRa-APRS statt RS41-Suchlauf (siehe radioApplyMode())
  if (fixedFreq) {                                   // feste Frequenz (Menue): kein Suchlauf
    line(12, String("Feste Frequenz ") + String(freqMHz, 3) + " MHz");
    if (!mapMode) drawFixedPanel(radio.getRSSI(false));   // Pegel/Statistik statt leerer Flaeche, siehe drawFixedPanel()
    trackSonde(freqMHz);
    return;
  }
  if (lastRxFreq > 0 && millis() - lastRxMs < STICKY_MS) {   // kuerzlich Sonde gehabt: erst auf ihrer Frequenz
    line(12, String("Lausche weiter ") + String(lastRxFreq, 3) + " MHz (zuletzt empfangene Sonde)");
    trackSonde(lastRxFreq);
    return;                                                   // naechste Runde entscheidet neu (weiter sticky, oder Suchlauf)
  }
  line(12, "Grobscan 400-406 MHz ...");
  uint32_t t0 = millis();
  coarseScan();
  uint32_t tCoarse = millis() - t0;
  float median;
  int n = pickCandidates(median);
  if (!mapMode) drawSpectrum();
  Serial.printf("\nGrobscan (%d Durchlaeufe gemittelt): %lu ms, Median %.1f dBm, %d Kandidat(en)\n", SCAN_SWEEPS, (unsigned long)tCoarse, median, n);
  line(24, String("Scan ") + SCAN_SWEEPS + "x " + tCoarse + " ms, Median " + String(median, 0) + " dBm, Kand.: " + n);

  if (n == 0) {
    line(36, "Kein Signal ueber Rauschen", ST77XX_ORANGE);
    Serial.println("Kein Kandidat ueber Median + Marge");
    return;                                   // sofort neuer Scan
  }

  for (int c = 0; c < n; c++) {
    uint32_t t1 = millis();
    float r, margin;
    float f = fineScan(candF[c], r, margin);
    lastScanFreq = f;                         // fuer die Anzeige im Menue F (letzte gescannte Kandidatenfrequenz)
    Serial.printf("Kandidat %d: grob %.3f MHz (%.1f dBm) -> fein %.3f MHz (%.1f dBm, %.1f dB ueber Median), Feinscan %lu ms\n",
                  c + 1, candF[c], candR[c], f, r, margin, (unsigned long)(millis() - t1));
    line(36, String("Kand. ") + (c + 1) + ": " + String(f, 3) + " MHz " + String(r, 1) + " dBm");
    if (margin < FINE_MARGIN) {
      Serial.printf("Kandidat %d: kein schmales Signal -> kein Decodierversuch\n", c + 1);
      continue;
    }
    if (millis() < foreignUntil && fabsf(f - foreignF) < 0.05f) {          // Sondenfilter: fremde Sonde uebergehen
      Serial.printf("Kandidat %d: fremde Sonde (Filter %s) -> uebersprungen\n", c + 1, filterId);
      continue;
    }
    if (trackSonde(f) || restartRx) return;   // Sonde gefunden und wieder verloren (oder Menue-Aenderung) -> neu beginnen
    Serial.printf("Kandidat %d: kein RS41-Frame\n", c + 1);
  }
}
