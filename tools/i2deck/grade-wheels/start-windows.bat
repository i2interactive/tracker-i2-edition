@echo off
rem i2 GRADE WHEELS for Windows: double-click to start. First run sets up
rem a Python environment next to this file (needs Python 3 from python.org).
cd /d "%~dp0"
if not exist .venv\Scripts\python.exe (
  echo Setting up (first run only)...
  py -3 -m venv .venv || python -m venv .venv || (echo Python 3 is needed. Install it from python.org, then try again. & pause & exit /b 1)
  .venv\Scripts\python -m pip install --quiet --upgrade pip
  .venv\Scripts\python -m pip install --quiet -r requirements.txt || (echo Setup failed. & pause & exit /b 1)
)
.venv\Scripts\python i2_grade_wheels.py %*
pause
