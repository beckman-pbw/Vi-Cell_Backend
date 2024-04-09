@echo off
set NEW_INSTALLER=
set NEW_EXE=
set NEW_UI_CFG=
set OLD_INSTALLER=
set OLD_EXE=
set OLD_UI_CFG=
set ERROR_MSG=
set INSTALLER_DBG=
set INSTALLER_ERROR=
set FORCE_UPDATE=
set PRESERVE_ALL=
set TGT_DRV=
set BASE_PATH=
set TOOL_DIR=
set REBOOT_RQD=
set ERRORS_FOUND=
set STARTUP_DIR=
set OS_IMG_VER=
set DATA_DIR=

:param_loop
if "%1x" == "x" goto start_install
if "%1x" == "-Dx" goto set_debug
if "%1x" == "-dx" goto set_debug
if "%1x" == "-Kx" goto set_preserve
if "%1x" == "-kx" goto set_preserve
if "%1x" == "-Fx" goto force_update
if "%1x" == "-fx" goto force_update
if "%1x" == "-Tx" goto set_install_drv
if "%1x" == "-tx" goto set_install_drv
if "%1x" == "-Px" goto set_install_path
if "%1x" == "-px" goto set_install_path
if "%1x" == "-?x" goto show_help
if "%1x" == "/?x" goto show_help
if "%1x" == "?x" goto show_help
if "%1x" == "-helpx" goto show_help
if "%1x" == "/helpx" goto show_help
goto next_param

:force_update
set FORCE_UPDATE=true
goto next_param

:set_preserve
set PRESERVE_ALL=true
goto next_param

:set_install_drv
if "%2x" == "x" goto missing_drv_param
shift
set TGT_DRV=%1
goto next_param

:set_install_path
if "%2x" == "x" goto missing_path_param
shift
set BASE_PATH=%1
goto next_param

:set_debug
set INSTALLER_DBG=debug
@echo on

:next_param
shift
goto param_loop

:show_help
@echo "                                                                                                 "
@echo "InstallViCellBLU:                                                                                "
@echo "                                                                                                 "
@echo "    -d                 : enable debug mode; script operation is displayed to a console window    "
@echo "    -k                 : preserve (keep) existing configuration files                            "
@echo "    -f                 : force overwrite of all configuration files with new files               "
@echo "    -t [drive-param]   : set installation destination drive                                      "
@echo "                       : drive should be specified with a colon                                  "
@echo "                       : (e.g. "D:" - without the quotes)                                        "
@echo "    -p [path-param]    : set installation path;                                                  "
@echo "                       : path specified must compatible with any target drive specifier given    "
@echo "                       : path specified MAY include a drive, but in this case, no additional     "
@echo "                       : drive specifier should be used                                          "
@echo "                                                                                                 "
goto folder_cleanup_xit

rem :========================================================================================================
rem :
rem :========================================================================================================

:start_install
if not "%INSTALLER_DBG%x" == "x" @echo on

rem  To check for a user folder which should ONLY exist on the instrument; if not present, assume this is an offline installation
set STARTUP_DIR="C:\Users\ViCellInstrumentUser\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\"

set "REBOOT_RQD=F"

echo ........................................... > %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo Starting ViCell-BLU Installer >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo %DATE%   %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2%  >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if not "%TGT_DRV%x" == "x" goto start_os_chk
pushd \
set TGT_DRV=%CD:\=%
popd

:start_os_chk
rem  do check for known OS image versions to apply version-specific patches
if exist %TGT_DRV%%BASE_PATH%\Instrument\restart_install.bat del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\restart_install.bat
rem Reference for future use...
rem set PATCH_FILE=%TGT_DRV%%BASE_PATH%\Instrument\Tools\windows10.0-kb4566424-x64_3d5bfb3e572029861cfb02c69de6b909153f5856.msu
if exist C:\Cell-Health-OS-v*.*.txt goto do_os_chk
if exist %TGT_DRV%%BASE_PATH%\Instrument\Cell-Health-OS-v*.*.txt goto do_os_chk
rem no OS indicator detected; assume this is an offline that we don't touch...
echo "No OS indicator found. No patches will be applied" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto install_start

:do_os_chk
if exist C:\Cell-Health-OS-v1.0.txt goto os_10
if exist %TGT_DRV%%BASE_PATH%\Instrument\Cell-Health-OS-v1.0.txt goto os_10
echo "No OS patch or shortcut updates required" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem  1.4 and beyond, for pgAdmin check
set OS_IMG_VER=1.x
goto spooler_chk

:os_10
set OS_IMG_VER=1.0


:spooler_chk
rem  don't modify print spooler on offline systems or unknown OS systems
if not exist %STARTUP_DIR% goto install_start
if "%OS_IMG_VER%x" == "x" goto install_start
@powershell Stop-Service -Name Spooler -Force
@powershell Set-Service -Name Spooler -StartupType Disabled



:install_start
if not "%INSTALLER_DBG%x" == "x" pause

rem off line installer creates the required instrument folder structure prior to this point, so only the install-related folders need to be created...
pushd %TGT_DRV%%BASE_PATH%\Instrument

echo "Create directories if needed" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp mkdir %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info mkdir %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\config mkdir %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\config >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\backup mkdir %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\backup >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

echo "Delete old files if needed" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\backup\* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\backup\* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\config\* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\config\* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info\* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info\* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

echo "Backup bin files" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem : Check for extraneous .bin files in the installer Software folder; remove if any are found
rem : NOTE: these next 2 specific file types belong in the %TGT_DRV%%BASE_PATH%\Instrument\Software folder, so they need to be
rem : moved to allow detection of the extraneous files, then replaced in the expected installation folder
if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software\C06*.bin goto setup_skip_bin_move
move /Y %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software\C06*.bin %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:setup_skip_bin_move

if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software\passcode.bin goto setup_skip_passcode
move /Y %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software\passcode.bin %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:setup_skip_passcode

rem : now check for any leftover/extraneous bin files and remove them
if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software\*.bin goto setup_no_extra_bins
rem : these shouldn't exist here; delete them
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software\*.bin >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:setup_no_extra_bins

if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info\*.bin goto done_with_bins
rem : move the expected files back to the expected location
move /Y %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info\*.bin %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:done_with_bins

echo "Create firmware directory if needed" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if not "%INSTALLER_DBG%x" == "x" pause
rem : check firmware folder presence and copy any controller firmware binary files to working folder
if exist %TGT_DRV%%BASE_PATH%\Instrument\bin goto chk_fw_archive_folder
mkdir %TGT_DRV%%BASE_PATH%\Instrument\bin >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:chk_fw_archive_folder
if exist %TGT_DRV%%BASE_PATH%\Instrument\bin\Archive goto chk_new_fw_files
mkdir %TGT_DRV%%BASE_PATH%\Instrument\bin\Archive >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:chk_new_fw_files
rem : Are there firmware files to install? If so check for any bin files leftover in the target folder
if exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\bin\*.txt goto chk_old_fw_files
goto install_config_files

:chk_old_fw_files
echo "Backup old firmware if needed" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem : If new files but no old files, just start the copy
if not exist %TGT_DRV%%BASE_PATH%\Instrument\bin\*.txt goto copy_new_fw_files
rem : prevent firmware installation confusion due to multiple sets in the folder
rem : delete the files in the archive folder to prevent the move from failing
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\bin\Archive\*
move /Y %TGT_DRV%%BASE_PATH%\Instrument\bin\* %TGT_DRV%%BASE_PATH%\Instrument\bin\Archive >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

echo "Install new firmware files" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:copy_new_fw_files
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\bin\* %TGT_DRV%%BASE_PATH%\Instrument\bin >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:install_config_files
echo "Install config files" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if not "%INSTALLER_DBG%x" == "x" pause
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Config\* %TGT_DRV%%BASE_PATH%\Instrument\Config >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if exist %TGT_DRV%%BASE_PATH%\Instrument\Software goto install_backend
echo Missing "Software" folder! >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
mkdir %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:install_backend
echo "Install all backend files" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem : copy all new backend package files to working folder
xcopy /E /I /Y /V /C /R /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software\* %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem : restore any special Software folder configuration bin files
if exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info\*.bin move /Y %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp\info\*.bin %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:clean1
rem : clean up unnecessary installed info files and files leftover from old arrangement
echo "Clean up unnecessary files" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if not "%INSTALLER_DBG%x" == "x" pause
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\*.bin goto clean2
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp goto skip_copy_install_temp
rmdir /S /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto skip_copy_install_temp

:clean2
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp goto clean3
mkdir %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto clean4

:clean3
rem : if the folder exists, ensure any leftover contents are removed
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp\*.bin del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp\*.bin >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:clean4
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\C06*.bin move /Y %TGT_DRV%%BASE_PATH%\Instrument\Software\C06*.bin %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\passcode.bin move /Y %TGT_DRV%%BASE_PATH%\Instrument\Software\passcode.bin %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem : delete any unknown/leftover bin files in the instrument software folder....
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\*.bin del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\*.bin >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if not exist %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp\*.bin goto skip_copy_install_temp
rem : do a copy to allow restoration of these installed files in the final cleanup...
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp\*.bin %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:skip_copy_install_temp
rem : leave the %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp folder for the final cleanup check...

if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\focuscontrollersim.* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\focuscontrollersim.* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem #############################################################################################################
rem      Start the UI installation checks
rem #############################################################################################################
rem : check for existing UI exe
cd %TGT_DRV%%BASE_PATH%\Instrument
if not "%INSTALLER_DBG%x" == "x" pause
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.ex~ set OLD_EXE=ViCellBLU_UI
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe set OLD_EXE=ViCellBLU_UI

rem : look at the file names to be installed, and go to the appropriate cleanup/install config section
if exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\UI_Installer\ViCellBLU_UI.msi goto cfg_chk
goto unk_tgt

:cfg_chk
rem : determine the current version to be installed; preserve it's config file, then prepare to uninstall a previous, differently-named version
set NEW_INSTALLER=ViCellBLU_UI.msi
set NEW_EXE=ViCellBLU_UI
set NEW_UI_CFG=%NEW_EXE%.exe.config
rem : CURRENT OPTION: current installer is the same as the old one; do a remove before running new UI installer
if exist %TGT_DRV%%BASE_PATH%\Instrument\ViCellBLU_UI.msi goto old_msi
rem : No known old installer found; check if there was an exe file...
rem : no prior exe is consistent with no known old installer; treat as new install
if "%OLD_EXE%x" == "x" goto skip_old_install

:old_msi
rem : if an old installation was detected, try to uninstall the previous version
if "%OLD_EXE%x" == "ViCellBLU_UIx" goto set_old_msi
if "%OLD_EXE%x" == "x" goto skip_old_install
goto unk_msi

:set_old_msi
rem :set OLD_EXE=ViCellBLU_UI
set OLD_INSTALLER=ViCellBLU_UI.msi

if not "%INSTALLER_DBG%x" == "x" pause
if "%OLD_INSTALLER%x" == 'x' goto skip_old_install
if not exist %TGT_DRV%%BASE_PATH%\Instrument\%OLD_INSTALLER% goto skip_old_install
set OLD_UI_CFG=%OLD_EXE%.exe.config
if "%NEW_INSTALLER%" == "ViCellBLU_UI.msi" goto cfg_upg1
goto no_installer

:cfg_upg1
if "%OLD_UI_CFG%x" == "x" goto unk_old_cfg
if "%NEW_UI_CFG%x" == "x" goto unk_new_cfg

echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Remove old UI: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem #############################################################################################################
rem Remove the old UI install - run un-install
rem suppress the 'removing' msiexec dialog... No user interaction of UAC approval needed
msiexec /x %OLD_INSTALLER% /qn /Lwemo %TGT_DRV%%BASE_PATH%\Instrument\UnInstaller.log
echo "Uninstall log" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
type %TGT_DRV%%BASE_PATH%\Instrument\UnInstaller.log >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\UnInstaller.log
echo "Old UI Un-Install return code: " %ERRORLEVEL% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if ERRORLEVEL 1 goto msi_uninstall_failed
rem after a successful uninstall, remove the old installer msi file and the UI exe file and indicator
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\ViCellBLU_UI.msi
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.ex~ del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.ex~ >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:skip_old_install

echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Install new UI: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if not "%INSTALLER_DBG%x" == "x" pause
rem #############################################################################################################
rem  install new ui
rem : move the installer to instrument root folder for installation
if "%NEW_INSTALLER%x" == "x" goto unk_new_msi
if not exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\UI_Installer\%NEW_INSTALLER% goto no_tgt
move /Y %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\UI_Installer\%NEW_INSTALLER% %TGT_DRV%%BASE_PATH%\Instrument >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem : if installing the same output target version; Install the UI.  Force all files to be re-installed to prevent unintentional uninstall, and to allow overwrite installation

echo "Install new UI - calling MSI" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem DO NOT USE the /qn option as this prevents ALL UI interaction, even OS UAC change approval notifications!
msiexec /i %NEW_INSTALLER% /quiet /passive /Lwemo %TGT_DRV%%BASE_PATH%\Instrument\MsiInstaller.log
echo "MSI Install log" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
type %TGT_DRV%%BASE_PATH%\Instrument\MsiInstaller.log >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\MsiInstaller.log
echo "New UI Install return code: " %ERRORLEVEL% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if ERRORLEVEL 1 goto msi_install_failed

if not "%INSTALLER_DBG%x" == "x" pause
echo "Update exe name and config files" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem : update the executable name to the expected instrument executable name, if necessary
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%NEW_EXE%.exe goto no_exe
if not "%NEW_EXE%x" == "ViCell_UIx" goto skip_ui_1
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

ren %TGT_DRV%%BASE_PATH%\Instrument\Software\%NEW_EXE%.exe ViCellBLU_UI.exe
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\%NEW_EXE%.exe %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%NEW_UI_CFG% copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\%NEW_UI_CFG% %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe.config >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto skip_ui_2

:skip_ui_1
if not "%NEW_EXE%x" == "ViCellBLU_UIx" goto unk_exe_tgt
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCell_UI.exe del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCell_UI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:skip_ui_2
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\%NEW_EXE%.exe %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%NEW_UI_CFG% copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\%NEW_UI_CFG% %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe.config >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem  Should not open ports for an offline installation on the customer's workstation!
if not exist %STARTUP_DIR% goto install_3rd_party

rem ####################################################################################
rem                          open the appropriate firewall rules
rem ####################################################################################
rem  Remove all existing old OpcUa Server rules before adding the updated executable and/or port rules
netsh advfirewall firewall delete rule name="OpcUa Server" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
netsh advfirewall firewall delete rule name="OpcUa Server Port" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
netsh advfirewall firewall delete rule name="OpcUa Server Exe" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem  If using ports, open the firewall ports required by OpcUa
netsh advfirewall firewall add rule name="OpcUa Server Port" dir=in action=allow enable=yes protocol=tcp interfacetype=lan localport=62641 profile=domain,private,public >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
netsh advfirewall firewall add rule name="OpcUa Server Port" dir=out action=allow enable=yes protocol=tcp interfacetype=lan localport=62641 profile=domain,private,public >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:install_3rd_party
rem ####################################################################################
rem                           install 3rd party software
rem ####################################################################################

if not "%INSTALLER_DBG%x" == "x" pause
rem *************** Check   Postgres and DB previously installed   ********************
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Start postgres install/update: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem save any PostgreSQL environment user name
set PGUSEROLD=%PGUSER%
set PGUSER=
set PGPASSWORD=
set PGDATABASEOLD=%PGDATABASE%
set PGDATABASE=
set PGPATH=
set DB_SCHEMA_FILE=

rem first check if the DB engine has been installed
rem Check for the postgresql installation
if exist "C:\Program Files\PostgreSQL\10\bin\psql.exe" goto pg_path_set
rem PostgreSQL does not appear to be in the expected location or the current folder. do complete install from install package
goto install_postgres

:pg_path_set
set PGPATH=C:\Program Files\PostgreSQL\10\bin\

rem first test to see if the db engine, schema, and roles have been installed
echo "Check if DB already exists" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem use the expected ViCell user, password, and DB name
set PGUSER=BCIViCellAdmin
@set PGPASSWORD=nimd@1leCiV1CB
set PGDATABASE=ViCellDB
@"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto check_db_exists2
echo "ViCell DB Found" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto check_db_version

:check_db_exists2
rem install of ViCell schema and role did not occur; check for default maintenance database using default superuser info
set PGUSER=postgres
@set PGPASSWORD=postgres
set PGDATABASE=postgres
@"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet > nul
if errorlevel 1 goto install_postgres
echo "DB engine and maintenance DB found" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo.
goto check_schema_install

:check_db_version
rem Save for future reference...
rem  Check the PostgreSQL version; currently, only two versions are possible, 10.13 and 10.20, and 10.20 is the 'updated' version
rem  This logic will need to be updated to check for any newer versions that might have been installed if other versions are used
echo "Check if DB needs to be updated" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
@"%PGPATH%psql.exe" --version > %TGT_DRV%%BASE_PATH%\Instrument\pgversion.txt
find /c /i "10.20" %TGT_DRV%%BASE_PATH%\Instrument\pgversion.txt > %TGT_DRV%%BASE_PATH%\Instrument\pgver.chk
if %ERRORLEVEL% GEQ 1 goto update_db_version
echo "No DB version update required" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto check_schema_install

:update_db_version
echo "Old DB version found.  DB version needs to be updated" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\EDB\postgresql-10.20-2-windows-x64.exe goto do_db_version_update
echo "DB version update file not found.  Continuing using old PostgreSQL version." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "DB version update file not found.  Continuing using old PostgreSQL version."
goto check_schema_install

:do_db_version_update
echo "Updating DB version..." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
%TGT_DRV%%BASE_PATH%\Instrument\Tools\EDB\postgresql-10.20-2-windows-x64.exe --mode unattended
echo "postgres update done - errorlevel: " %errorlevel% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem **** end postgres update

echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "PostgreSQL update - Done: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

echo "Check if PostgreSQL installed correctly" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem The PostgreSQL installation told us everything is OK but let's check now. 
rem use the default installation superuser, password, and maintainance DB name
@"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto postgres_update_failed
echo "DB version update install succeeded." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

@"%PGPATH%psql.exe" --version > %TGT_DRV%%BASE_PATH%\Instrument\pgversion.txt
find /c /i "10.20" %TGT_DRV%%BASE_PATH%\Instrument\pgversion.txt > %TGT_DRV%%BASE_PATH%\Instrument\pgver.chk
if %ERRORLEVEL% GEQ 1 goto postgres_version_find_failed
echo "DB updated OK.  Updated DB version found" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto check_schema_install

:postgres_version_find_failed
echo "Updated DB version not found." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto check_schema_install

:install_postgres
if not "%INSTALLER_DBG%x" == "x" pause
if not exist "%TGT_DRV%%BASE_PATH%\Instrument\DB_Data" goto set_dflt_db_data_dir
rmdir /S /Q "%TGT_DRV%%BASE_PATH%\Instrument\DB_Data" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if %ERRORLEVEL% GEQ 1 goto db_data_dir_error
if exist "%TGT_DRV%%BASE_PATH%\Instrument\DB_Data" goto db_data_dir_error

:set_dflt_db_data_dir
echo "Setting DB data folder to '%TGT_DRV%%BASE_PATH%\Instrument\DB_Data'." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
set DATA_DIR= --datadir "%TGT_DRV%%BASE_PATH%\Instrument\DB_Data"
goto start_postgres_install

:db_data_dir_error
echo "Can't remove empty DB_Data folder." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Using PostgreSQL default data folder 'C:\Program Files\PostgreSQL\10\data'." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:start_postgres_install
if not "%INSTALLER_DBG%x" == "x" pause
rem *************************************************************************
rem                 Install all PostgreSQL-related components
rem *************************************************************************
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Start OBDC driver install: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem *************** Install   ODBC driver for postgres   ********************
if not "%INSTALLER_DBG%x" == "x" pause
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\EDB\edb_psqlodbc.exe goto ODBC_missing
%TGT_DRV%%BASE_PATH%\Instrument\Tools\EDB\edb_psqlodbc.exe --mode unattended
rem echo "ODBC install done errorlevel" %errorlevel% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem **** end ODBC install

echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Start PostgreSQL install: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem *************** Install   Postgres   ********************
if not "%INSTALLER_DBG%x" == "x" pause
rem set the default installation superuser, password, and maintainance DB name
set PGUSER=postgres
@set PGPASSWORD=postgres
set PGDATABASE=postgres
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\EDB\postgresql-10.20-2-windows-x64.exe goto postgres_installer_missing
%TGT_DRV%%BASE_PATH%\Instrument\Tools\EDB\postgresql-10.20-2-windows-x64.exe --mode unattended %DATA_DIR%
rem echo "postgres install done errorlevel: " %errorlevel% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem  OS version 1.0 (and beyond) came with the compatible standalone pgAdmin installed; don't delete shortcuts

:skip_install_lnk_delete
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "PostgreSQL install - Done: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

echo "Check if PostgreSQL installed correctly" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem The PostgreSQL installation told us everything is OK but let's check now. 
rem use the default installation superuser, password, and maintainance DB name
if not exist "C:\Program Files\PostgreSQL\10\bin\psql.exe" goto postgres_install_failed
rem PostgreSQL appears to be in the expected location.
set PGPATH=C:\Program Files\PostgreSQL\10\bin\

@"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto postgres_install_failed

:check_schema_install
if not "%INSTALLER_DBG%x" == "x" pause
rem test to see if the ViCell schema and roles have been installed
echo "Checking if ViCell DB schema and roles already exist..." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo.
rem use the expected ViCell user, password, and DB name
set PGUSER=BCIViCellAdmin
@set PGPASSWORD=nimd@1leCiV1CB
set PGDATABASE=ViCellDB
if not "%INSTALLER_DBG%x" == "x" pause
@"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto start_schema_install
echo "ViCell DB Found" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:start_schema_update
rem *************************************************************************
rem                 Update   Postgres Schema
rem *************************************************************************
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Start Schema Update: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem the SchemaAlter.sql file is setup to perform the updates on both schemas
set "DB_SCHEMA_FILE=%TGT_DRV%%BASE_PATH%\Instrument\Tools\SchemaAlter.sql"
rem - remove quotes from the filename
set DB_SCHEMA_FILE=%DB_SCHEMA_FILE:"=%
if not exist %DB_SCHEMA_FILE% goto DB_schema_update_missing

if not "%INSTALLER_DBG%x" == "x" pause
@"%PGPATH%psql.exe" --file=%DB_SCHEMA_FILE% --quiet 
if errorlevel 1 goto DB_schema_update_failed

rem The update told us everything is OK but let's check now. 
@"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto DB_schema_update_failed
echo DB schema updated OK >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo.

echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Update Schema Done: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo.
goto db_done

:start_schema_install
rem *************************************************************************
rem                      Create DB User Roles and schema
rem *************************************************************************
if not "%INSTALLER_DBG%x" == "x" pause
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Start Create Schema: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem the DBCreate.sql file is setup to create both schemas and all required/expected users
set "DB_SCHEMA_FILE=%TGT_DRV%%BASE_PATH%\Instrument\Tools\DBCreate.sql"
rem - remove quotes from the filename
set DB_SCHEMA_FILE=%DB_SCHEMA_FILE:"=%
if not exist %DB_SCHEMA_FILE% goto DB_schema_create_missing

if not "%INSTALLER_DBG%x" == "x" pause
rem set the default installation superuser, password, and maintainance DB name
set PGUSER=postgres
@set PGPASSWORD=postgres
set PGDATABASE=postgres
@"%PGPATH%psql.exe" --file=%DB_SCHEMA_FILE% --quiet 
if errorlevel 1 goto DB_schema_create_failed

rem The schema creation told us everything is OK but let's check now. 
rem Now use the expected admin user, password, and DB name
set PGUSER=BCIViCellAdmin
@set PGPASSWORD=nimd@1leCiV1CB
set PGDATABASE=ViCellDB
@"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto DB_schema_create_failed
echo DB schema created OK >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Create Schema Done: " %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ........................................... >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:db_done
set PGUSER=
set PGUSEROLD=
set PGPASSWORD=
set PGDATABASE=
set PGDATABASEOLD=

rem **********
rem **********   End of DB related work
rem ***********************************

:chk_mcafee_update
if not "%INSTALLER_DBG%x" == "x" pause
if not exist "C:\Program Files\McAfee\Solidcore\sadmin.exe" goto reboot_chk
rem tell McAfee about new trusted files 
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\bin"
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\lib"
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\scripts"
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\share"
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\pgAdmin 4"
@sadmin trusted -i "C:\Program Files (x86)\pgAdmin 4\v4"

@sadmin features enable pkg-ctrl

rem add BeckmanConnect certificate
@sadmin cert add -u "%TGT_DRV%%BASE_PATH%\Instrument\Tools\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer"
rem add TeamViewer certificate
@sadmin cert add -u "%TGT_DRV%%BASE_PATH%\Instrument\Tools\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer"
rem add the new 10 year BeckmanConnect certificate
@sadmin cert add -u "%TGT_DRV%%BASE_PATH%\Instrument\Tools\BeckmanConnect Certificate.cer"

:reboot_chk
if "%REBOOT_RQD%x" == "Tx" goto check_mcafee_reboot
rem - .NET reboot is NOT required - we know we don't need, nor want, certain files anymore so delete them
echo ".NET Reboot is NOT required" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto no_reboot_cleanup

:check_mcafee_reboot
echo ******************************************* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "!!!!!!  McAfee Reboot IS required  !!!!!!" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ******************************************* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if not "%INSTALLER_DBG%x" == "x" pause
rem *************************************************************************
rem  Only disable McAfee if we installed SW that requires reboot to complete
rem  NOTE that systems built with the OS accompanying the 1.0 software (v0.10f) DID NOT have McAfee installed!
rem  Make no assumptions about a system based on the absence of McAfee!
rem 
if exist "C:\Program Files\McAfee\Solidcore\sadmin.exe" goto disable_mcafee
rem  A reboot is required but this system doesn't have McAfee so remove files we don't want or need anymore 
echo "No McAfee reboot required" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:no_reboot_cleanup
if not "%INSTALLER_DBG%x" == "x" pause
echo "Removing files used for restart" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\FinishInstall.bat del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Tools\FinishInstall.bat >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\LogoutUser.bat del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Tools\LogoutUser.bat >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\WhitelistingUpdate.bat del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Tools\WhitelistingUpdate.bat >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto done_mcafee

:disable_mcafee
echo "McAfee - is installed - disable" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem sadmin status  >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rem the 'start /wait' format appears ot be necessary to allow the sadmin utility to properly handlle the 'disable' command in the scope of this script...
start /wait /B sadmin disable 
timeout /t 2 /nobreak > nul
rem sadmin status  >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if not exist %STARTUP_DIR% goto done_mcafee
rem  Appears to be an instrument installation; copy the installation continuation script files
move /Y %TGT_DRV%%BASE_PATH%\Instrument\Tools\LogoutUser.bat %STARTUP_DIR% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem only move the startup link for instruments, not offline configurations
set LNK_NAME="Start ViCell UI.lnk"
set START_SHORT="%STARTUP_DIR:"=%%LNK_NAME:"=%"
set TOOLS_SHORT="%TGT_DRV%%BASE_PATH%\Instrument\Tools\%LNK_NAME:"=%"
set TOOLS_SHORT="%TOOLS_SHORT:"=%"
if exist %START_SHORT% move /Y %START_SHORT% %TOOLS_SHORT% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:done_mcafee
rem **********   End of McAfee related work 

rem **********
if not "%INSTALLER_DBG%x" == "x" pause
rem : handle (re)install of missing, newer, or pre-configured files
if exist %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\UI_Installer\* move /Y %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\UI_Installer\* %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Software\Libraries\*.dll goto skip_move_dlls
move /Y %TGT_DRV%%BASE_PATH%\Instrument\Software\Libraries\*.dll %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rmdir /S /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\Libraries

:skip_move_dlls
rem ################################################################################
rem                       Cleanup 
rem ################################################################################
rem 
cd %TGT_DRV%%BASE_PATH%\Instrument
if not "%INSTALLER_DBG%x" == "x" pause
rem : move all new package files to working folder to ensure no overwrite by UI installer
xcopy /E /I /Y /V /C /R /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\Software\* %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem : Cleanup install_temp directory 
if not "%INSTALLER_DBG%x" == "x" pause
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp goto skip_remove_install_temp
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp\*.bin goto delete_install_temp_dir
rem : restore any files installed prior to the UI install that may have been overwritten or deleted by the UI installaation
move /Y %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp\*.bin %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:delete_install_temp_dir
rmdir /S /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\install_temp >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:skip_remove_install_temp
cd %TGT_DRV%%BASE_PATH%\Instrument
if not "%INSTALLER_DBG%x" == "x" pause
attrib -r %TGT_DRV%%BASE_PATH%\Instrument\Software\* /S >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
attrib -r %TGT_DRV%%BASE_PATH%\Instrument\Config\* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if not "%INSTALLER_DBG%x" == "x" pause
goto done_pass_xit

:DB_schema_create_missing
set ERROR_MSG="DB schema creation script is missing"
goto db_error_xit

:DB_schema_create_failed
set ERROR_MSG="DB schema creation failed"
goto db_error_xit

:DB_schema_update_missing
set ERROR_MSG="DB schema update script is missing"
goto db_error_xit

:DB_schema_update_failed
set ERROR_MSG="DB schema update failed"
goto db_error_xit

:postgres_install_failed
set ERROR_MSG="PostgreSQL installation failed"
goto db_error_xit

:postgres_update_failed
set ERROR_MSG="PostgreSQL version update failed"
goto db_error_xit

:postgres_installer_missing
set ERROR_MSG="Postgres installer is missing"

:db_error_xit
set PGUSER=
set PGUSEROLD=
set PGPASSWORD=
set PGDATABASE=
set PGDATABASEOLD=

goto error_xit

:ODBC_missing
set ERROR_MSG="ODBC installer is missing"
goto error_xit

:vc_redist_missing
set ERROR_MSG="VC Redist installer is missing"
goto error_xit

:dotnet_missing
set ERROR_MSG="dot NET installer is missing"
goto error_xit

:dotnet_install_failed
set ERROR_MSG="Failure installing dot NET"
goto error_xit

:msi_uninstall_failed
set ERROR_MSG="Failure uninstalling UI application!"
goto msi_failure

:msi_install_failed
set ERROR_MSG="Failure installing UI application!"

:msi_failure
rem after a UI msi install or uninstall failure, remove the old installer msi file and the UI exe file and indicator to cleanup
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\ViCellBLU_UI.msi >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.ex~ del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.ex~ >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
goto error_xit

:unk_tgt
set ERROR_MSG="No known UI installer file found!"
goto error_xit

:unk_old_cfg
set ERROR_MSG="Old UI configuration file not defined!"
goto error_xit

:unk_new_cfg
set ERROR_MSG="New UI configuration file not defined!"
goto error_xit

:unk_new_msi
set ERROR_MSG="New installer file not defined!"
goto error_xit

:unk_old_msi
set ERROR_MSG="Old installer file not defined!"
goto error_xit

:missing_old_msi
set ERROR_MSG="Old installer file not found!"
goto error_xit

:missing_old_exe
set ERROR_MSG="Old installer 'exe' file not found!"
goto error_xit

:bad_drv_param
set ERROR_MSG="Unrecognized installation drive parameter: '%CHK_DRV%'!"
goto set_error

:missing_drv_param
set ERROR_MSG="Missing installation target drive parameter!"
goto set_error

:missing_path_param
set ERROR_MSG="Missing installation target path parameter!"
goto set_error

:unk_msi
set ERROR_MSG="Unknown/Unrecognized installer file!"
goto error_xit

:no_installer
set ERROR_MSG="Installer file not defined!"
goto error_xit

:no_tgt
set ERROR_MSG="Installer file not found!"
goto error_xit

:unk_exe_tgt
set ERROR_MSG="Unknown/Unrecognized installer output 'exe' file!"
goto error_xit

:no_exe
set ERROR_MSG="Installer output 'exe' file not found!"

:error_xit
echo ":error_xit -- " >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\FinishInstall.bat del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Tools\FinishInstall.bat >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\LogoutUser.bat del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Tools\LogoutUser.bat >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\WhitelistingUpdate.bat del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Tools\WhitelistingUpdate.bat >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
popd

:set_error
echo ":set_error -- " >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo. >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo %ERROR_MSG% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo %ERROR_MSG%
set ERROR_MSG= : errors found!
set INSTALLER_ERROR=%ERROR_MSG%
set "ERRORS_FOUND=T"

if not "%INSTALLER_DBG%x" == "x" pause
goto folder_cleanup_xit

rem : do self-clean and exit
:done_pass_xit
popd

:folder_cleanup_xit
rem do common install/update file cleanup
rem remove cert files now
if exist "%TGT_DRV%%BASE_PATH%\Instrument\Tools\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer" del /F /Q "%TGT_DRV%%BASE_PATH%\Instrument\Tools\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist "%TGT_DRV%%BASE_PATH%\Instrument\Tools\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer" del /F /Q "%TGT_DRV%%BASE_PATH%\Instrument\Tools\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist "%TGT_DRV%%BASE_PATH%\Instrument\Tools\BeckmanConnect Certificate.cer" del /F /Q "%TGT_DRV%%BASE_PATH%\Instrument\Tools\BeckmanConnect Certificate.cer" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

rem do registry script removal
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\UTF8.reg del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Tools\UTF8.reg

rem do DB-related file cleanup
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\EDB\postgresql-10.20-2-windows-x64.exe del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Tools\EDB\postgresql-10.20-2-windows-x64.exe
REM if exist %TGT_DRV%%BASE_PATH%\Instrument\pgversion.txt del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\pgversion.txt
REM if exist %TGT_DRV%%BASE_PATH%\Instrument\pgver.chk del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\pgver.chk

rem Always remove the DB creation script if present
rem recreate the schema name value to account for all sql scripts that may be present
set "DB_SCHEMA_FILE=%TGT_DRV%%BASE_PATH%\Instrument\Tools\*.sql"
rem - remove quotes from the filename
set DB_SCHEMA_FILE=%DB_SCHEMA_FILE:"=%
if "%DB_SCHEMA_FILE%x" == "x" goto folder_cleanup2
if exist %DB_SCHEMA_FILE% del /F /Q %DB_SCHEMA_FILE% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
set DB_SCHEMA_FILE=

:folder_cleanup2
rem : do installer folder cleanup
echo Installer folder cleanup >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
cd %TGT_DRV%%BASE_PATH%\Instrument
attrib -r -s -h %TGT_DRV%%BASE_PATH%\Instrument\SW_Install /D /S >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rmdir /S /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install\temp >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
rmdir /S /Q %TGT_DRV%%BASE_PATH%\Instrument\SW_Install >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Config\HawkeyeConfig.einfo del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Config\HawkeyeConfig.einfo >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if not "%INSTALLER_DBG%x" == "x" pause

echo ******************************************* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo     Finished ViCell-BLU Installer  %ERROR_MSG% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ******************************************* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo     %DATE%    %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2% >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ******************************************* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if not "%ERRORS_FOUND%x" == "Tx" goto skip_error_msg
echo "!!!!!!!  ERROR(s) during install  !!!!!!!" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ******************************************* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo .
echo . ----------------------------------------------------------------
echo .
echo . ERROR   ERROR   ERROR   ERROR   ERROR   ERROR   ERROR   ERROR
echo . 
echo . Finished ViCell-BLU Installer:  %ERROR_MSG%
echo . 
echo . ----------------------------------------------------------------
echo .
rem - wait for the user to continue
pause
rem - don't want to reboot if there was an error
goto log_cleanup

:skip_error_msg
echo ">>>>>>>>>>>>>>>  PASSED  <<<<<<<<<<<<<<<<" >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ******************************************* >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

if "%REBOOT_RQD%x" == "Tx" goto offline_reboot_chk
if "%REBOOT_RQD%x" == "Yx" goto offline_reboot_chk
goto log_cleanup

:offline_reboot_chk
if not exist %STARTUP_DIR% goto offline_reboot
echo ............................................ >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Reboot required - auto reboot starting ..." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ............................................ >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
@echo Finishing install and preparing to restart the system
goto do_reboot

:offline_reboot
echo .............................................. >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Offline Reboot required - prompting user ..." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo .............................................. >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
@echo Finishing install.
echo.
@echo The system must be restarted to apply all changes.

:reboot_prompt
echo.
set /p DO_RESTART="Do you want to restart now(Y) or later(L)? (default is later):"
if "%DO_RESTART%x" == "x" goto log_cleanup
if "%DO_RESTART%x" == "yx" goto reboot_now
if "%DO_RESTART%x" == "Yx" goto reboot_now
if "%DO_RESTART%x" == "lx" goto log_cleanup
if "%DO_RESTART%x" == "Lx" goto log_cleanup
echo.
echo unrecognized/illegal response!
goto reboot_prompt

:reboot_now
echo ................................ >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo "Offline Reboot now selected..." >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
echo ................................ >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log

:do_reboot
rem - give a few seconds in the pre-timeout to allow the log file to get copied and to complete the outer installer...
shutdown /r /t 15 /d p:0:0 /c "Restarting to apply changes"

:log_cleanup
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Installer.log %TGT_DRV%%BASE_PATH%\Instrument\Logs\

:xit
set NEW_INSTALLER=
set NEW_EXE=
set NEW_UI_CFG=
set OLD_INSTALLER=
set OLD_EXE=
set OLD_UI_CFG=
set ERROR_MSG=
set INSTALLER_DBG=
set FORCE_UPDATE=
set PRESERVE_ALL=
set TGT_DRV=
set BASE_PATH=
set TOOL_DIR=
set REBOOT_RQD=
set ERRORS_FOUND=
set STARTUP_DIR=
set OS_IMG_VER=
set DATA_DIR=

