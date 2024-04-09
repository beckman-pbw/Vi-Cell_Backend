@echo off
set QUIET_FLAGS=/quiet /promptrestart
set DBG_MODE=
set PATCH_FILE=
set SET_MCAFEE=
set OS_VER=

:param_chk
if %1x == x goto chk_OS
if %1x == -dx goto set_debug_on
if %1x == -d-x goto set_debug_off
if %1x == -qx goto set_quiet_on
if %1x == -q-x goto set_quiet_off
goto bad_param

:set_debug_on
@echo on
set DBG_MODE=TRUE
goto next_param

:set_debug_off
@echo off
set DBG_MODE=
goto next_param

:set_quiet_on
set QUIET_FLAGS=/quiet /promptrestart
goto next_param

:set_quiet_off
set QUIET_FLAGS=
goto next_param

:bad_param
echo unrecognized option! ignoring...

:next_param
shift
goto param_chk

:chk_OS
rem - check for an instrument OS indicator; missing components should generate a message
if exist C:\Cell-Health-OS-v1.*.txt goto mcafee_chk
if exist C:\Instrument\Cell-Health-OS-v1.*.txt goto mcafee_chk
echo "No OS patch or shortcut updates required"
goto fix_chk

:mcafee_chk
if exist "C:\Program Files\McAfee\Solidcore\sadmin.exe" goto mcafee_update_begin
@echo Application control component not found in OS configuration!

:mcafee_update_begin
@sadmin begin-update
if errorlevel == 0 goto mcafee_update_ok
@echo Error entering update mode! Non-admin environment!
goto xit

:mcafee_update_ok
set SET_MCAFEE=T

:patch_chk
if not "%DBG_MODE%x"== "x" pause
rem do check for OS image version-specific patch
@echo "No OS patch or shortcut updates required"
goto fix_chk

:fix_chk
if not "%DBG_MODE%x"== "x" pause
if exist C:\Instrument\ViCell_install.exe del /f /q C:\Instrument\ViCell_install.exe

rem remove all previous install remnants
if not exist C:\Instrument\ViCellBLU_UI.msi goto fix_chk1
msiexec /x C:\Instrument\ViCellBLU_UI.msi /quiet /passive
del /f /q C:\Instrument\ViCellBLU_UI.msi

:fix_chk1
if not exist C:\Instrument\Software\ViCellBLU_UI.exe goto fix_chk2
del /f /q C:\Instrument\Software\ViCellBLU_UI.exe

:fix_chk2
if not exist C:\Instrument\Software\ViCellBLU_UI.ex~ goto start_patch
del /f /q C:\Instrument\Software\ViCellBLU_UI.ex~

if "%SET_MCAFEE%x" == "x" goto xit
@sadmin end-update

:xit
if not "%DBG_MODE%x"== "x" pause
set QUIET_FLAGS=
set DBG_MODE=
set PATCH_FILE=
set SET_MCAFEE=
set OS_VER=
