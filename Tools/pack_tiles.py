#!/usr/bin/env python3
"""
pack_tiles.py — packt Kartenkacheln (z/x/y/<name>.png|jpg) pro Zoomstufe in
wenige Bundle-Dateien plus eine sortierte Index-Datei.

Warum: FAT32 verwaltet jeden Ordner als lineare Liste, und jede Datei belegt
mindestens einen ganzen Cluster (meist 32 KB), egal wie klein sie ist. Bei
Millionen einzelner Kachel-Dateien wird das Suchen langsam und der Speicher-
verbrauch explodiert. Bundles pro Zoomstufe machen daraus eine Handvoll
Dateien.

WICHTIG -- FAT32-Limit: eine einzelne Datei darf auf FAT32 nie groesser als
4 GiB (genau 4.294.967.295 Byte) werden, das ist eine Grenze des Dateisystems
selbst, keine dieses Skripts. Deshalb wird jede Zoomstufe bei Bedarf in
mehrere "Teile" (Parts) aufgeteilt, jeder deutlich unter dieser Grenze
(--max-part-mb, Default 3072 MB = 3 GiB Sicherheitsabstand).

Format je Zoomstufe <Z>:
  <out>/bundles/z<Z>_p<N>.tb  Ein Bundle-Teil: Kachel-Bytes hintereinander,
                              ohne Trennzeichen. N beginnt bei 0; meist gibt
                              es nur Teil 0, bei sehr grossen Zoomstufen
                              mehrere.
  <out>/bundles/z<Z>.tbi      Index: 16-Byte-Header, danach pro Kachel 20
                              Byte (x, y, part, offset -- alle vier als
                              uint32, offset zaehlt INNERHALB des jeweiligen
                              Teils, bleibt also immer < 4 GiB --, length als
                              uint32; little endian), sortiert aufsteigend
                              nach (x, dann y). Der ESP32 sucht darin per
                              Binaersuche (Sonde und "Ich" sind jeweils nur
                              eine Handvoll Kacheln, das kostet pro Kachel
                              rund log2(Anzahl) kleine Lesezugriffe).

  Header (16 Byte): Magic "TBI3" (4 Byte), uint32 Anzahl Eintraege,
                     uint32 Zoomstufe, uint32 Dateityp (0 = PNG, 1 = JPG).

Kacheln, die kleiner als --min-size sind, werden NICHT ins Bundle
aufgenommen (fuer Luftbild/Topo/Relief-JPGs: leere Kacheln ausserhalb der
Abdeckung sind sehr klein, siehe TILE_BLANK_BYTES im Sketch — meist reicht
derselbe Schwellwert). Fehlt eine Kachel im Bundle, faellt der Sketch auf
die Basiskarte (/tiles) zurueck, genau wie bisher bei fehlenden Dateien.

Eine Zoomstufe muss durchgehend eine Dateiendung haben (png ODER jpg), weil
der Header nur EINEN Dateityp fuer die ganze Zoomstufe traegt. Kommen beide
vor (sollte bei diesen Downloads nicht passieren), wird die haeufigere
Endung gepackt und die andere übersprungen (mit Warnung).

Aufruf (ein Lauf pro Kartenart, mehrere Ordner gehen auch in einem Aufruf):
python3 Tools/pack_tiles.py ~/kartenkacheln/tiles --verify
python3 Tools/pack_tiles.py ~/kartenkacheln/tiles_sat ~/kartenkacheln/tiles_topo \
    ~/kartenkacheln/tiles_relief ~/kartenkacheln/tiles_topplus \
    --min-size 2048 --verify

Nach erfolgreichem Packen (und --verify, falls genutzt) koennen die
urspruenglichen z/x/y-Ordner geloescht werden, entweder von Hand oder mit
--delete-source (packt zuerst ALLE Zoomstufen fertig, loescht danach).

Nur Python-Standardbibliothek, kein pip-Install noetig.
"""

import argparse
import collections
import os
import random
import re
import struct
import sys
import time

MAGIC = b"TBI3"               # v3: Bundles koennen in mehrere <4-GiB-Teile aufgeteilt sein (FAT32-Limit)
REC_FMT = "<IIIII"         # x, y, part, offset (innerhalb des Teils), length -- alle uint32, little endian
REC_LEN = struct.calcsize(REC_FMT)
HDR_FMT = "<4sIII"         # magic, count, zoom, ext (0=png, 1=jpg)
HDR_LEN = struct.calcsize(HDR_FMT)
EXT_NAMES = {0: "png", 1: "jpg"}
EXT_CODES = {"png": 0, "jpg": 1, "jpeg": 1}

DIGIT_RE = re.compile(r"^\d+$")
TILE_RE = re.compile(r"^(\d+)\.(png|jpg|jpeg)$", re.IGNORECASE)

DEFAULT_MAX_PART_BYTES = 3072 * 1024 * 1024   # 3 GiB: deutlich unter dem FAT32-Limit von 4 GiB - 1 Byte


def find_zoom_dirs(tiles_dir, out_subdir):
    """Liefert (zoom:int, pfad) fuer jeden rein numerisch benannten Unterordner
    von tiles_dir, ausser dem Ausgabeordner selbst."""
    zdirs = []
    with os.scandir(tiles_dir) as it:
        for e in it:
            if not e.is_dir():
                continue
            if e.name == out_subdir:
                continue
            if DIGIT_RE.match(e.name):
                zdirs.append((int(e.name), e.path))
    zdirs.sort(key=lambda t: t[0])
    return zdirs


def iter_tiles(zoom_dir):
    """Liefert (x:int, y:int, path, size, ext:str) fuer jede Kachel-Datei unter zoom_dir/x/y.ext."""
    with os.scandir(zoom_dir) as xs:
        for xe in xs:
            if not xe.is_dir() or not DIGIT_RE.match(xe.name):
                continue
            x = int(xe.name)
            with os.scandir(xe.path) as ys:
                for ye in ys:
                    if not ye.is_file():
                        continue
                    m = TILE_RE.match(ye.name)
                    if not m:
                        continue
                    y = int(m.group(1))
                    try:
                        size = ye.stat().st_size
                    except OSError:
                        continue
                    ext = m.group(2).lower()
                    yield x, y, ye.path, size, ("jpg" if ext == "jpeg" else ext)


def pack_zoom(zoom, zoom_dir, out_dir, min_size, max_part_bytes, verify, verify_samples, progress_every):
    print(f"z{zoom}: sammle Kacheln in {zoom_dir} ...")
    entries = []            # (x, y, path, size) -- landen im Bundle (nur die Mehrheits-Dateiendung)
    skipped_paths = []      # zu kleine ("leere") Kacheln -- werden bei --delete-source ebenfalls entfernt
    ext_counts = collections.Counter()
    by_ext = collections.defaultdict(list)
    scanned = 0
    t0 = time.time()
    for x, y, path, size, ext in iter_tiles(zoom_dir):
        scanned += 1
        if progress_every and scanned % progress_every == 0:
            print(f"  ... {scanned} Dateien durchsucht ({time.time() - t0:.0f} s)")
        if min_size and size < min_size:
            skipped_paths.append(path)
            continue
        ext_counts[ext] += 1
        by_ext[ext].append((x, y, path, size))

    if not ext_counts:
        print(f"z{zoom}: keine Kacheln >= {min_size} Byte gefunden, ueberspringe.")
        return None, skipped_paths

    ext = ext_counts.most_common(1)[0][0]
    entries = by_ext[ext]
    for other_ext, items in by_ext.items():          # Kacheln mit der "falschen" Endung: wie zu klein behandeln
        if other_ext != ext:
            print(f"z{zoom}: WARNUNG -- {len(items)} Kacheln mit Endung .{other_ext} neben "
                  f"{len(entries)} mit .{ext}; nur .{ext} wird gepackt, der Rest wird uebersprungen.")
            skipped_paths.extend(p for _, _, p, _ in items)

    entries.sort(key=lambda e: (e[0], e[1]))   # nach x, dann y: passt zur Binaersuche im Sketch

    os.makedirs(out_dir, exist_ok=True)
    tbi_path = os.path.join(out_dir, f"z{zoom}.tbi")

    print(f"z{zoom}: schreibe {len(entries)} Kacheln ({len(skipped_paths)} zu klein/falscher Typ uebersprungen), "
          f"Dateityp .{ext}, max. {max_part_bytes / 1e9:.2f} GB je Teil")
    index_records = []
    part = -1
    tb = None
    part_offset = 0
    parts_written = []
    try:
        for i, (x, y, path, size) in enumerate(entries):
            with open(path, "rb") as f:
                data = f.read()
            n = len(data)
            if tb is None or part_offset + n > max_part_bytes:
                if tb:
                    tb.close()
                part += 1
                part_path = os.path.join(out_dir, f"z{zoom}_p{part}.tb")
                tb = open(part_path, "wb")
                parts_written.append(part_path)
                part_offset = 0
            tb.write(data)
            index_records.append((x, y, part, part_offset, n))
            part_offset += n
            if progress_every and (i + 1) % progress_every == 0:
                print(f"  ... {i + 1}/{len(entries)} geschrieben, Teil {part} ({time.time() - t0:.0f} s)")
    finally:
        if tb:
            tb.close()

    with open(tbi_path, "wb") as fi:
        fi.write(struct.pack(HDR_FMT, MAGIC, len(index_records), zoom, EXT_CODES[ext]))
        for rec in index_records:
            fi.write(struct.pack(REC_FMT, *rec))

    total_bytes = sum(os.path.getsize(p) for p in parts_written)
    tbi_size = os.path.getsize(tbi_path)
    print(f"z{zoom}: fertig. {len(parts_written)} Teil(e), zusammen {total_bytes / 1e6:.1f} MB, "
          f"Index {tbi_size / 1e6:.2f} MB, {len(index_records)} Kacheln, {time.time() - t0:.0f} s")

    if verify:
        verify_bundle(out_dir, zoom, tbi_path, entries, verify_samples)

    return [e[2] for e in entries], skipped_paths   # Quellpfade: gepackt / uebersprungen (beide fuer --delete-source)


def read_index(tbi_path):
    with open(tbi_path, "rb") as f:
        magic, count, zoom, ext_code = struct.unpack(HDR_FMT, f.read(HDR_LEN))
        if magic != MAGIC:
            raise ValueError(f"{tbi_path}: falsches oder veraltetes Format (Magic {magic!r}, erwartet {MAGIC!r})")
        data = f.read(count * REC_LEN)
    ext = EXT_NAMES.get(ext_code, "?")
    return zoom, ext, [struct.unpack_from(REC_FMT, data, i * REC_LEN) for i in range(count)]


def bsearch(records, x, y):
    """Binaersuche wie im Sketch: sortiert nach (x, y). Gibt (part, offset, length) oder None."""
    lo, hi = 0, len(records) - 1
    key = (x, y)
    while lo <= hi:
        mid = (lo + hi) // 2
        rx, ry, rpart, roff, rlen = records[mid]
        rk = (rx, ry)
        if rk == key:
            return rpart, roff, rlen
        if rk < key:
            lo = mid + 1
        else:
            hi = mid - 1
    return None


def verify_bundle(out_dir, zoom, tbi_path, entries, samples):
    """Liest ein paar zufaellige Kacheln ueber Index + Bundle-Teile zurueck und vergleicht sie
    byteweise mit der Originaldatei. Bricht mit AssertionError ab, wenn etwas nicht passt."""
    zoom2, ext, records = read_index(tbi_path)
    n = min(samples, len(entries))
    picks = random.sample(entries, n) if n < len(entries) else entries
    open_parts = {}   # part-Nummer -> offenes File-Objekt (mehrere Teile moeglich)
    try:
        for x, y, path, _size in picks:
            hit = bsearch(records, x, y)
            assert hit is not None, f"z{zoom} ({x},{y}): nicht im Index gefunden"
            part, offset, length = hit
            if part not in open_parts:
                open_parts[part] = open(os.path.join(out_dir, f"z{zoom}_p{part}.tb"), "rb")
            tb = open_parts[part]
            tb.seek(offset)
            got = tb.read(length)
            with open(path, "rb") as f:
                want = f.read()
            assert got == want, f"z{zoom} ({x},{y}) Teil {part}: Bytes im Bundle stimmen nicht mit der Quelle ueberein"
    finally:
        for f in open_parts.values():
            f.close()
    print(f"z{zoom}: Verify OK ({n} von {len(entries)} Kacheln stichprobenartig geprueft, Typ .{ext}, "
          f"{len(open_parts)} Teil(e) beruehrt)")


def delete_sources(paths, zoom_dirs):
    """Loescht alle urspruenglichen Kachel-Dateien (gepackt + zu klein/falscher Typ uebersprungen) und
    danach leere x-Ordner sowie die z-Ordner selbst, falls sie dadurch leer werden."""
    print(f"Loesche {len(paths)} urspruengliche Kachel-Dateien ...")
    x_dirs = set()
    for p in paths:
        os.remove(p)
        x_dirs.add(os.path.dirname(p))
    for d in sorted(x_dirs, key=len, reverse=True):   # tiefste Ordner zuerst
        try:
            os.rmdir(d)
        except OSError:
            pass  # nicht leer (z. B. eine Kachel dort war doch >= min-size und ist noch da)
    for d in zoom_dirs:
        try:
            os.rmdir(d)
        except OSError:
            pass  # z-Ordner nicht leer
    print("Fertig.")


def process_dir(tiles_dir, args):
    """Verarbeitet einen Kartenordner (z. B. tiles_sat) komplett: packen, verifizieren, ggf. loeschen."""
    tiles_dir = os.path.abspath(tiles_dir)
    if not os.path.isdir(tiles_dir):
        print(f"Kein Ordner, ueberspringe: {tiles_dir}")
        return

    out_dir = os.path.join(tiles_dir, args.out_subdir)
    zdirs = find_zoom_dirs(tiles_dir, args.out_subdir)
    if args.zoom:
        zdirs = [(z, p) for z, p in zdirs if z in args.zoom]
    if not zdirs:
        print(f"{tiles_dir}: keine passenden Zoom-Ordner gefunden, ueberspringe.")
        return

    max_part_bytes = args.max_part_mb * 1024 * 1024
    print(f"\n=== {tiles_dir} ===")
    print(f"Gefundene Zoomstufen: {[z for z, _ in zdirs]}")
    all_source_paths = []
    for zoom, zpath in zdirs:
        packed, skipped = pack_zoom(zoom, zpath, out_dir, args.min_size, max_part_bytes,
                                     args.verify, args.verify_samples, args.progress_every)
        if packed:
            all_source_paths.extend(packed)
        all_source_paths.extend(skipped)

    if args.delete_source:
        if not all_source_paths:
            print("Nichts zu loeschen (kein Bundle erfolgreich erzeugt).")
        else:
            delete_sources(all_source_paths, [p for _, p in zdirs])
    else:
        print(f"Hinweis: die urspruenglichen z/x/y-Dateien unter {tiles_dir} liegen noch da. "
              "Erst pruefen (z. B. --verify), dann von Hand loeschen oder das Skript "
              "mit --delete-source erneut laufen lassen.")


def main():
    ap = argparse.ArgumentParser(description="Packt z/x/y-Kartenkacheln pro Zoomstufe in Bundle(s) + Index.")
    ap.add_argument("tiles_dir", nargs="+",
                     help="Ein oder mehrere Kartenordner mit den Zoom-Unterordnern, "
                          "z. B. tiles tiles_sat tiles_topo tiles_relief tiles_topplus")
    ap.add_argument("--zoom", type=int, action="append", help="Nur diese Zoomstufe(n) packen (mehrfach angebbar); ohne Angabe: alle")
    ap.add_argument("--min-size", type=int, default=0,
                     help="Kacheln kleiner als N Byte werden ausgelassen (leere/Rand-Kacheln, z. B. 2048 fuer JPG-Luftbilder/Topo/Relief)")
    ap.add_argument("--max-part-mb", type=int, default=3072,
                     help="Maximale Groesse je Bundle-Teil in MB, FAT32 erlaubt keine Datei ueber 4096 MB (Default: 3072)")
    ap.add_argument("--out-subdir", default="bundles", help="Unterordner fuer die Bundle-/Index-Dateien (Default: bundles)")
    ap.add_argument("--verify", action="store_true", help="Nach dem Packen Stichproben aus dem Bundle zurücklesen und mit dem Original vergleichen")
    ap.add_argument("--verify-samples", type=int, default=200, help="Anzahl Stichproben je Zoomstufe fuer --verify (Default: 200)")
    ap.add_argument("--delete-source", action="store_true",
                     help="Nach erfolgreichem Packen (und --verify, falls gesetzt) die urspruenglichen z/x/y-Dateien loeschen")
    ap.add_argument("--progress-every", type=int, default=20000, help="Fortschrittsmeldung alle N Dateien (0 = aus)")
    args = ap.parse_args()

    if args.max_part_mb * 1024 * 1024 >= 4 * 1024 * 1024 * 1024:
        sys.exit("--max-part-mb muss unter 4096 liegen (FAT32-Limit pro Datei).")

    for d in args.tiles_dir:
        process_dir(d, args)


if __name__ == "__main__":
    main()
