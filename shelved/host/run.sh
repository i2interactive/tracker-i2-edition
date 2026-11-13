#!/bin/sh
# Build the host harness and run a scenario: run.sh <quiz|medal> <sdroot> <script> [outdir]
set -e
here=$(cd "$(dirname "$0")" && pwd)
root="$here/../../.."
out=${4:-"$here/out/$1"}
mkdir -p "$out"
g++ -std=gnu++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined \
  -I"$root/core/interface/i2Arcade" \
  "$here/host.cpp" "$root/core/interface/i2Arcade/i2QuizGame.cpp" "$root/core/interface/i2Arcade/i2MedalGame.cpp" \
  -o "$here/host_bin"
"$here/host_bin" "$1" "$2" "$out" "$3"
python3 "$here/render.py" "$out"/*.txt
