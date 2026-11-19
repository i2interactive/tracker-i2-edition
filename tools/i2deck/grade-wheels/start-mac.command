#!/bin/bash
# i2 GRADE WHEELS for Mac: double-click to start. First run sets up a Python
# environment next to this file (needs python3; macOS offers to install it).
cd "$(dirname "$0")" || exit 1
if [ ! -x .venv/bin/python ]; then
  echo "Setting up (first run only)..."
  python3 -m venv .venv || { echo "python3 is needed. Install it, then try again."; read -r; exit 1; }
  .venv/bin/python -m pip install --quiet --upgrade pip
  .venv/bin/python -m pip install --quiet -r requirements.txt || { echo "Setup failed."; read -r; exit 1; }
fi
exec .venv/bin/python i2_grade_wheels.py "$@"
