@echo off
cls
set REBOOT_ME="F"
if not exist "C:\Program Files\McAfee\Solidcore\sadmin.exe" goto skip_mcafee

rem We need to delay a short time to allow all of windows services to start
echo Delay before solidify, please wait ...
SET /A XCOUNT=0
:loop1
SET /A XCOUNT+=1
echo Delay before check status : %XCOUNT%
timeout /t 1 /nobreak > nul
IF "%XCOUNT%" == "15" (
  GOTO end_loop1
) ELSE (
  GOTO loop1
)
:end_loop1


SET /A XCOUNT=0
:loop2
SET /A XCOUNT+=1
echo Delay before solidify, please wait : %XCOUNT%
start /wait /b sadmin status
if %ERRORLEVEL% EQU 0 goto end_loop2
timeout /t 5 /nobreak > nul
IF "%XCOUNT%" == "12" (
  GOTO ERROR_OUT
) ELSE (
  GOTO loop2
)
:end_loop2
timeout /t 5 /nobreak > nul


cls
echo .
echo . ----------------------------------------------------------------
echo .
echo . Re-solidifing the system
echo .
echo . This will take around 20+ minutes depending on amount of data
echo .
echo . The system will automatically reboot when this is done
echo .
echo ----------------------------------------------------------------
echo .


start /wait /b sadmin so 
if not %ERRORLEVEL% EQU 0 goto ERROR_OUT
start /wait /b sadmin enable
if not %ERRORLEVEL% EQU 0 goto ERROR_OUT
set REBOOT_ME="T"

set STARTUP_DIR="C:\Users\ViCellInstrumentUser\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\"
set PROGS_DIR="C:\Users\ViCellInstrumentUser\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\"

:skip_mcafee
if not exist %STARTUP_DIR%\LogoutUser.bat goto skip1
del /f %STARTUP_DIR%\LogoutUser.bat
:skip1

set START_SHORT="C:\Users\ViCellInstrumentUser\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\Start ViCell UI.lnk"
set TOOLS_SHORT="C:\Instrument\Tools\Start ViCell UI.lnk"
copy /Y %TOOLS_SHORT% %START_SHORT%

if not %REBOOT_ME% == "T" goto skip_reboot
echo !!!!!!!!!!!!!!!!!!!!
echo System will reboot shortly 
echo 
shutdown /r /t 15
:skip_reboot

call :deleteSelf
:deleteSelf
   start /B "" cmd /C del "%~f0"&exit /B 0

exit 1

:ERROR_OUT
echo sadmin - solidify errors
@echo on
sadmin status

@timeout /t 600
exit 1