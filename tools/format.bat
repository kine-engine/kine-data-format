@echo off
setlocal enabledelayedexpansion

:: project root directory
cd /d "%~dp0.."

:: find and format all C and H files, skip build directories
for /r %%f in (*.c *.h) do (
  echo %%f | findstr /i "\\build\\" >nul
  if errorlevel 1 (
    clang-format -i "%%f"
  )
)

echo formatting complete.
