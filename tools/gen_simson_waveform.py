#!/usr/bin/env python3
"""Konwersja przebiegu z oscyloskopu (CSV) na tablice symboli RMT do odtwarzania
na GPIO. Sygnal jest progowany z histereza (analog -> cyfra), bo tor pomiarowy
(MCPWM capture) patrzy tylko na zbocza. Wynik laduje w src/simson_waveform.h.

Uzycie:
    python tools/gen_simson_waveform.py [csv] [--hi 2.0] [--lo 1.0] [--res-us 1]
"""
import csv, sys, os, argparse

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

ap = argparse.ArgumentParser()
ap.add_argument("csv", nargs="?",
                default=os.path.join(ROOT, "csv", "processed_Simson_wolne_ok1250_FULL.csv"))
ap.add_argument("--hi", type=float, default=2.0, help="prog gorny histerezy [V]")
ap.add_argument("--lo", type=float, default=1.0, help="prog dolny histerezy [V]")
ap.add_argument("--res-us", type=float, default=1.0, help="rozdzielczosc RMT [us/tick]")
ap.add_argument("--out", default=os.path.join(ROOT, "src", "simson_waveform.h"))
args = ap.parse_args()

# --- wczytaj probki + okres probkowania ---
with open(args.csv) as f:
    r = csv.reader(f)
    next(r)                       # naglowek 1
    h2 = next(r)                  # ['Volt', start, increment, ...]
    inc = float(h2[2])           # sekundy / probke
    vals = [float(row[0]) for row in r if row and row[0] != ""]

n = len(vals)
fs = 1.0 / inc
print(f"plik: {os.path.basename(args.csv)}")
print(f"probki: {n}, fs: {fs/1e6:.3f} MHz, czas: {n*inc*1000:.2f} ms")

# --- digitalizacja z histereza ---
state = 1 if vals[0] > args.hi else 0
edges = []                        # indeksy zboczy
for i, v in enumerate(vals):
    if state == 0 and v > args.hi:
        state = 1; edges.append(i)
    elif state == 1 and v < args.lo:
        state = 0; edges.append(i)

# --- odcinki (poziom, czas trwania w probkach) ---
segs = []                         # (level, dur_samples)
prev_idx = 0
level = 1 if vals[0] > args.hi else 0
for idx in edges:
    segs.append((level, idx - prev_idx))
    level ^= 1
    prev_idx = idx
segs.append((level, n - prev_idx))   # ogon do konca przechwytu

# czas trwania w tickach RMT (1 tick = res_us mikrosekund), min 1 tick
def to_ticks(dur_samples):
    us = dur_samples * inc * 1e6
    return max(1, round(us / args.res_us))

segs_t = [(lvl, to_ticks(d)) for lvl, d in segs]

# --- aby petla byla bezszwowa i liczba odcinkow parzysta: scal pierwszy z
#     ostatnim, jesli maja ten sam poziom (oba to zwykle "idle high") ---
if len(segs_t) % 2 == 1 and segs_t[0][0] == segs_t[-1][0]:
    lvl0, d0 = segs_t[0]
    lvll, dl = segs_t.pop()
    segs_t[0] = (lvl0, d0 + dl)
# jesli nadal nieparzyste, dolóz minimalny odcinek przeciwnego poziomu
if len(segs_t) % 2 == 1:
    last_lvl = segs_t[-1][0]
    segs_t.append((last_lvl ^ 1, 1))

# RMT: jedno pole duration to 15 bitow (max 32767 tickow). Rozbij dluzsze.
MAXT = 32767
flat = []
for lvl, d in segs_t:
    while d > MAXT:
        flat.append((lvl, MAXT)); d -= MAXT
    flat.append((lvl, d))
if len(flat) % 2 == 1:
    flat.append((flat[-1][0] ^ 1, 1))

# --- pakuj po 2 odcinki na symbol rmt_symbol_word_t ---
symbols = []
for i in range(0, len(flat), 2):
    (l0, d0) = flat[i]
    (l1, d1) = flat[i + 1]
    symbols.append((d0, l0, d1, l1))

total_ticks = sum(d for _, d in flat)
print(f"zbocza: {len(edges)}, odcinki: {len(flat)}, symbole RMT: {len(symbols)}")
print(f"czas petli: {total_ticks * args.res_us / 1000:.2f} ms")

res_hz = int(round(1e6 / args.res_us))
with open(args.out, "w") as f:
    f.write("// AUTOGENEROWANE przez tools/gen_simson_waveform.py - nie edytowac recznie.\n")
    f.write(f"// Zrodlo: {os.path.basename(args.csv)} (histereza {args.lo}/{args.hi} V)\n")
    f.write(f"// {len(symbols)} symboli, czas petli {total_ticks*args.res_us/1000:.2f} ms,"
            f" rozdzielczosc {args.res_us} us/tick.\n")
    f.write("#ifndef SIMSON_WAVEFORM_H\n#define SIMSON_WAVEFORM_H\n\n")
    f.write('#include "driver/rmt_tx.h"\n\n')
    f.write(f"#define SIMSON_WAVEFORM_RESOLUTION_HZ {res_hz}u  // {args.res_us} us/tick\n\n")
    f.write("static const rmt_symbol_word_t SIMSON_WAVEFORM[] = {\n")
    for (d0, l0, d1, l1) in symbols:
        f.write(f"    {{ .duration0 = {d0}, .level0 = {l0}, .duration1 = {d1}, .level1 = {l1} }},\n")
    f.write("};\n\n")
    f.write("#define SIMSON_WAVEFORM_LEN (sizeof(SIMSON_WAVEFORM) / sizeof(SIMSON_WAVEFORM[0]))\n\n")
    f.write("#endif // SIMSON_WAVEFORM_H\n")

print(f"zapisano: {os.path.relpath(args.out, ROOT)}")
