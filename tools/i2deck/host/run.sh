#!/bin/sh
# Build the host harness, run a script, render the screens:
#   run.sh [script] [outdir]      (default: tour.txt -> out/)
set -e
here=$(cd "$(dirname "$0")" && pwd)
d="$here/../../../core/interface/i2Deck"
script=${1:-"$here/tour.txt"}
out=${2:-"$here/out"}
rm -rf "$out" && mkdir -p "$out/sd"
g++ -std=gnu++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined -I"$d" \
  "$here/host.cpp" "$d/i2DeckApp.cpp" "$d/i2DeckDecks.cpp" "$d/i2Keys.cpp" "$d/i2DotFont.cpp" \
  -o "$here/host_bin"
"$here/host_bin" "$out/sd" "$out" "$script"
python3 "$here/render.py" "$out"/*.txt
