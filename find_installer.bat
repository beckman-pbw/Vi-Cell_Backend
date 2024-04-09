@echo off
set FOUND_PN=

:param_loop
if %1x == x goto no_param
if %1x == -Dx goto dbg_set
if not %1x == -dx goto start_find

:dbg_set
set FIND_DBG=debug

:next_param
shift
goto param_loop

:start_find
if not "%FIND_DBG%x" == "x" @echo on
if not exist "%1:\C90104*.exe" goto xit
set FOUND_PN=C90104
goto pn_chk

:pn_chk
if "%INSTALLER_PN%x" == "x" goto pn_chk2
if not "%INSTALLER_PN%x" == "%FOUND_PN%x" goto xit
if "%SRC_DRV%x" == "x" goto drv_set
if %SRC_DRV% LEQ %1 goto xit
goto drv_set

:pn_chk2
if "%INST_TYPE_PN%x" == "x" goto pn_chk3
if not "%INST_TYPE_PN%x" == "%FOUND_PN%x" goto xit

:pn_chk3
if not "%SRC_DRV%x" == "x" goto pn_chk4
set INSTALLER_PN=%FOUND_PN%
goto drv_set

:pn_chk4
if not "%SRC_DRV%x" == "%1x" goto xit
set INSTALLER_PN=%FOUND_PN%
goto xit

:drv_set
set SRC_DRV=%1
goto xit

:no_param
echo No drive parameter supplied!

:xit
set FOUND_PN=
