@echo off

set folder_path=path\to\folder\

echo %folder_path%

echo.
echo ========================================
echo.

Extract_ProbeDownUpTime.py %folder_path%

echo.
echo ========================================
echo.

Extract_SetValveTime.py %folder_path%

echo.
echo ========================================
echo.

Extract_SetPositionTime.py %folder_path%

echo.
echo ========================================
echo.

Extract_StateChangeTime.py %folder_path%

PAUSE