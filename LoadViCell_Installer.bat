@echo off
set ERRORS_FOUND=
set ERR_MSG=
set SRC_DRV=
set TGT_DRV=
set TGT_DRV_FLAG=
set BASE_PATH=
set TGT_PATH_FLAG=
set INSTALLER_PN=
set INST_TYPE_PN=
set UI_NAME=
set INSTALLER_NAME=
set LOADER_DBG=
set FIND_DBG=
set FIND_DBG_FLAG=
set INSTALLER_DBG_FLAG=
set INSTALLER_ERRORS=
set FORCE_FLAG=
set PRESERVE_FLAG=
set SIM_INSTALL=
set SIM_INSTALL_FLAG=
set DO_REMOVE=
set DISPLAY_TIMEOUT=
set CHK_DRV=
set CHK_PARAM=

set TGT_INST_TYPE_PN=C90104

set LOADER_NAME=LoadViCell_Installer.bat

rem - all valid instrument OS versions SHOULD have whitelisting installed; missing components should generate a message
if exist \Cell-Health-OS-v*.txt goto white_chk
if exist \Instrument\Cell-Health-OS-v*.txt goto white_chk
@echo Non-standard installation OS!

:white_chk
if exist "C:\Program Files\McAfee\Solidcore\sadmin.exe" goto start_ok
@echo Missing application control component expected in instrument OS configuration!
goto param_loop

:start_ok
@sadmin begin-update
if errorlevel == 0 goto param_loop
@echo Error entering update mode! Non-admin environment!
set DISPLAY_TIMEOUT=300
goto xit

:param_loop
if "%1x" == "x" goto start_load
if "%1x" == "-Dx" goto set_debug
if "%1x" == "-dx" goto set_debug
if "%1x" == "-DFx" goto set_find_debug
if "%1x" == "-dfx" goto set_find_debug
if "%1x" == "-DIx" goto set_install_debug
if "%1x" == "-dix" goto set_install_debug
if "%1x" == "-Kx" goto set_preserve
if "%1x" == "-kx" goto set_preserve
if "%1x" == "-Fx" goto set_force
if "%1x" == "-fx" goto set_force
if "%1x" == "-Sx" goto set_sim_install
if "%1x" == "-sx" goto set_sim_install
if "%1x" == "-Tx" goto set_install_drv
if "%1x" == "-tx" goto set_install_drv
if "%1x" == "-Px" goto set_install_path
if "%1x" == "-px" goto set_install_path
if "%1x" == "-Ux" goto set_uninstall
if "%1x" == "-ux" goto set_uninstall
if "%1x" == "-?x" goto show_help
if "%1x" == "/?x" goto show_help
if "%1x" == "?x" goto show_help
if "%1x" == "-helpx" goto show_help
if "%1x" == "/helpx" goto show_help

set CHK_PARAM=%1
goto bad_param

:set_force
rem - force and preserve are mutually exclusive
set FORCE_FLAG= -f
set PRESERVE_FLAG=
goto next_param

:set_preserve
rem - force and preserve are mutually exclusive
set PRESERVE_FLAG= -p
set FORCE_FLAG=
goto next_param

:set_debug
@echo on
set LOADER_DBG=debug
:set INSTALLER_DBG_FLAG= -d
goto next_param

:set_find_debug
set FIND_DBG_FLAG= -d
goto next_param

:set_install_debug
set INSTALLER_DBG_FLAG= -d
goto next_param

:set_sim_install
set SIM_INSTALL=true
set SIM_INSTALL_FLAG= -s
goto next_param

:set_install_drv
if "%2x" == "x" goto missing_drv_param
shift

:chk_drv_param
rem - @echo off
set CHK_DRV=
for %%i in (C D E F G H I J K L M N O P Q R S T U V W X Y Z) do if "%CHK_DRV%x" == "x" ( if "%1" == "%%i:" (set CHK_DRV=%1) else ( if "%1" == "%%i" (set CHK_DRV=%1:)))
if not "%LOADER_DBG%x" == "x" @echo on
if not "%CHK_DRV%x" == "x" goto drv_set
set CHK_DRV=%1
goto bad_drv_param

:drv_set
set TGT_DRV=%CHK_DRV%
set TGT_DRV_FLAG= -t %TGT_DRV%
goto next_param

:set_install_path
if "%2x" == "x" goto missing_path_param
shift
set BASE_PATH=%1
set TGT_PATH_FLAG= -p %BASE_PATH%
goto next_param

:set_uninstall
set DO_REMOVE=true

:next_param
shift
goto param_loop

:show_help
@echo "                                                                                                 "
@echo "%LOADER_NAME%:                                                                                   "
@echo "                                                                                                 "
@echo "    -d                 : enable debug mode; script operation is displayed to a console window    "
@echo "    -df                : installation package locator sub-process process debug                  "
@echo "    -di                : installation package process debug                                      "
@echo "    -k                 : preserve (keep) existing configuration files                            "
@echo "    -f                 : force overwrite of all configuration files with new files               "
@echo "    -s                 : install for simulation mode                                             "
@echo "    -tx [drive-param]  : set installation destination drive                                      "
@echo "                       : drive may be specified with or without a colon                          "
@echo "                       : (e.g. D: and D are both valid); script converts to expected format      "
@echo "    -px [path-param]   : set installation path;                                                  "
@echo "                       : path specified must compatible with any target drive specifier given    "
@echo "                       : path specified MAY include a drive, but in this case, no additional     "
@echo "                       : drive specifier should be used                                          "
@echo "                                                                                                 "
goto xit

:start_load
if not "%LOADER_DBG%x" == "x" @echo on
if "%LOADER_DBG%x" == "x" cls
if not exist %TGT_DRV%%BASE_PATH%\Instrument goto no_tgt_path

pushd %TGT_DRV%%BASE_PATH%\Instrument

@echo Starting ViCell-BLU Install loader > %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
@echo. >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Install.log.last del /F /Q \Instrument\Install.log.last
if exist %TGT_DRV%%BASE_PATH%\Instrument\Install.log ren \Instrument\Install.log Install.log.last

rem - check if the software has already been installed for a specific instrument type
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.ex~ goto blu_inst
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellBLU_UI.exe goto blu_inst

:blu_inst
set UI_NAME=ViCellBLU_UI
if not "%DO_REMOVE%x" == "x" goto remove_old
set INST_TYPE_PN=%TGT_INST_TYPE_PN%
set INSTALLER_NAME=InstallViCellBLU

:findloop
rem - installer P/N is specified; check for the installer source drive
if not "%DO_REMOVE%x" == "x" goto old_not_found
for %%i in (D E F G H I J K L M N O P Q R S T U V W X Y Z) do call .\find_installer.bat%FIND_DBG_FLAG% %%i
if not "%LOADER_DBG%x" == "x" @echo on
if "%SRC_DRV%x" == "x" goto no_src
rem - the finder needs to do magnitude comparisons, and so can't add the colon until after...
set SRC_DRV=%SRC_DRV%:
if "%INSTALLER_PN%x" == "x" goto no_inst
if "%INST_TYPE_PN%x" == "x" goto strt_pn_chk
if not "%INST_TYPE_PN%x" == "%INSTALLER_PN%x" goto bad_inst

:strt_pn_chk
if "%INSTALLER_NAME%x" == "x" goto pn_chk2
if not "%UI_NAME%x" == "x" goto tgt_drv_chk

:pn_chk2
if "%INSTALLER_PN%" == %TGT_INST_TYPE_PN% goto set_blu_pn
if "%UI_NAME%x" == "x" goto no_uinam
goto tgt_drv_chk

:set_blu_pn
set INST_TYPE_PN=%TGT_INST_TYPE_PN%
set UI_NAME=ViCellBLU_UI
set INSTALLER_NAME=InstallViCellBLU

:tgt_drv_chk
rem - check for the drive target for the install; it may be needed for the hash check...
rem - start by changing to the source installer drive to test the target drive and path as a valid specifier
pushd %SRC_DRV%\
if not exist %TGT_DRV%%BASE_PATH%\Instrument\%LOADER_NAME% goto tgt_drv_chk1
rem - TGT_DRV is specified OR BASE_PATH contains a drive specifier in the path; use as-is
goto start_hash_chk

:tgt_drv_chk1
rem - the full combination of target drive and base path is not valid; check each for individual validity
if "%TGT_DRV%x" == "x" goto tgt_drv_chk2
rem - target drive is not empty; check base path for a valid drive/path specifier...
if not exist %BASE_PATH%\Instrument\%LOADER_NAME% goto tgt_drv_chk2
rem - base path points at a valid target path, possibly including a drive; clear the specified target drive...
set TGT_DRV=
goto start_hash_chk

:tgt_drv_chk2
rem - need to determine the current drive letter; base path may or may not be valid...
set CHK_DRV=
for %%i in (C D E F G H I J K L M N O P Q R S T U V W X Y Z) do if "%CHK_DRV%x" == "x" ( if exist "%%i:%BASE_PATH%\Instrument\%LOADER_NAME%" set CHK_DRV=%%i)
if not "%LOADER_DBG%x" == "x" @echo on
if "%CHK_DRV%x" == "x" goto tgt_drv_chk3
set TGT_DRV=%CHK_DRV%:
goto start_hash_chk

:tgt_drv_chk3
rem - no installer found; shouldn't happen, but may be a combination of pad path and drive specifiers...
popd
goto no_loader_found

:start_hash_chk
popd
if not "%LOADER_DBG%x" == "x" pause
if not exist "%SRC_DRV%\%INSTALLER_PN%*.exe" goto no_installer_found
if exist "%SRC_DRV%\hash.xml" goto chk_hash_util
goto no_hash_key

:chk_hash_util
rem - installer source drive and P/N are specified and found; check the installer hash
if exist %WINDIR%\fciv.exe goto hash_path1_ok
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv\fciv.exe goto hash_path2_ok
if exist %SRC_DRV%\fciv.exe goto copy_src_hash
goto no_hash_tool

:copy_src_hash
rem - first, try copying the tool from the source drive to the windows folder
copy /Y /V %SRC_DRV%\fciv.exe %WINDIR% >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel == 0 goto hash_path1_ok
@echo Error copying hash tool to expected folder (%WINDIR%)! Possibly non-admin user!
@echo Error copying hash tool to expected folder (%WINDIR%)! Possibly non-admin user! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
set DISPLAY_TIMEOUT=300

rem - next, try copying the tool from the source drive to the instrument tool folder
rem - first, check for the expected tool folder and create if necessary
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv goto copy_hash
mkdir %TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel == 0 goto copy_hash2
@echo Error creating the instrument hash tool folder (%TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv)! Possibly non-admin user!
@echo Error creating the instrument hash tool folder (%TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv)! Possibly non-admin user! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
set DISPLAY_TIMEOUT=300

:copy_hash
rem - if necessary, copying the tool from the source drive to the instrument tool folder
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv\fciv.exe goto hash_path2_ok
copy /Y /V %SRC_DRV%\fciv.exe %TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel == 0 goto hash_path2_ok
@echo Error copying hash tool to the instrument hash tool folder (%TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv)! Possibly non-admin user!
@echo Error copying hash tool to the instrument hash tool folder (%TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv)! Possibly non-admin user! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
set DISPLAY_TIMEOUT=300

rem - if all else fails, copy the tool from the source drive to the instrument root folder
if exist %TGT_DRV%%BASE_PATH%\Instrument\fciv.exe goto hash_copy3_ok
copy /Y /V %SRC_DRV%\fciv.exe %TGT_DRV%%BASE_PATH%\Instrument >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel == 0 goto hash_path3_ok
@echo Error copying the hash tool from the instrument root folder(%TGT_DRV%%BASE_PATH%\Instrument) to the Windows folder (%WINDIR%)! Possibly non-admin user!
@echo Error copying the hash tool to the instrument root folder! Possibly non-admin user to the Windows folder (%WINDIR%)! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto no_hash_tool

:hash_path1_ok
set HASH_PATH=%WINDIR%\
goto do_hash_chk

:hash_path2_ok
set HASH_PATH=%TGT_DRV%%BASE_PATH%\Instrument\Tools\fciv\
goto do_hash_chk

:hash_path3_ok
set HASH_PATH=%TGT_DRV%%BASE_PATH%\Instrument\

:do_hash_chk
rem - verify the hash values on the detected target install package
pushd %SRC_DRV%\
if not exist %HASH_PATH%fciv.exe goto no_hash_tgt
%HASH_PATH%fciv.exe -v -both -xml %SRC_DRV%\hash.xml > nul
if errorlevel 0 goto hash_ok
rem - restore the previous path AFTER checking the errorlevel value!
popd
goto hash_chk_failed

:hash_ok
rem - restore the previous path AFTER checking the errorlevel value!
popd
@echo Installer package file integrity verification succeeded! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

:copy_installer
rem - installer archive drive and P/N are specified; hash checks OK; copy it into the system to execute it
copy /Y /V "%SRC_DRV%\%INSTALLER_PN%*.exe" . >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel == 1 goto copy_error

rem - rename the installer archive to a common name
if not exist "%INSTALLER_PN%*.exe" goto installer_rename_failed
if exist ViCell_install.exe del /F /Q ViCell_install.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
ren "%INSTALLER_PN%*.exe" ViCell_install.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

rem - unpack the found self-extracting installer archive
if not exist ViCell_install.exe goto no_local_installer
ViCell_install.exe -y >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel 1 goto unpack_failed

if exist ViCell_install.exe del /F /Q ViCell_install.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if not "%LOADER_DBG%x" == "x" pause

rem - run the installation batch, including the instrument-specific UI installer
if exist %INSTALLER_NAME%.bat goto run_batch
if not exist InstallViCell.bat goto no_batch
set INSTALLER_NAME=InstallViCell

:run_batch
call %INSTALLER_NAME%.bat%INSTALLER_DBG_FLAG%%FORCE_FLAG%%PRESERVE_FLAG%%SIM_INSTALL_FLAG%%TGT_DRV_FLAG%%TGT_PATH_FLAG%
if not "%LOADER_DBG%x" == "x" @echo on
if exist %INSTALLER_NAME%.bat del /F /Q %INSTALLER_NAME%.bat >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if "%INSTALLER_ERROR%x" == "x" goto installer_ok
goto installer_failed

:installer_ok
if not "%LOADER_DBG%x" == "x" pause
rem - if UI install succeeded, it will leave the instrument-specific-name executable
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.ex~ del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.ex~ >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe goto install_ok >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe.config del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe.config >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto ui_fail

:install_ok
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe.config %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe.config >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
:ren %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe %UI_NAME%.ex~ >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.ex~ >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
rem - currently, the UI needs it's native-named file to run...
rem - del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

if "%SIM_INSTALL%x" == "x" goto set_hdwr
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\environment.config.sim goto copy_sim
@echo Missing simulation environment configuration file! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto chk_env_default

:copy_sim
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\environment.config.sim %TGT_DRV%%BASE_PATH%\Instrument\Software\UIConfiguration\environment.config >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel == 0 goto chk_ndp
@echo Error copying the simulation environment configuration file! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto chk_env_default

:set_hdwr
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\environment.config.hdwr goto copy_hdwr
@echo Missing hardware environment configuration file! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto chk_env_default

:copy_hdwr
copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Software\environment.config.hdwr %TGT_DRV%%BASE_PATH%\Instrument\Software\UIConfiguration\environment.config >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel == 0 goto chk_ndp
@echo Error copying the hardware environment configuration file! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

:chk_env_default
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\UIConfiguration\environment.config goto run_with_default
goto no_env

:run_with_default
@echo System will run with the default setting! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
set DISPLAY_TIMEOUT=300

:chk_ndp
rem - on an instrument, cleanup the .NET install file installed indication if the installer file exists
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\.NET_4.7.2 goto chk_logs >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\.NET_4.7.2\NDP472-KB054530-x86-x64-AllOS-ENU.exe goto chk_logs
rem - if on an instrument, just cleanup the filename indicator; if not on an instrument, make no assumptions about the installation and don't change the filename
if exist %TGT_DRV%\Cell-Health-OS-*.txt goto chk_ndp2
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Cell-Health-OS-*.txt goto chk_logs

:chk_ndp2
ren %TGT_DRV%%BASE_PATH%\Instrument\Tools\.NET_4.7.2\NDP472-KB054530-x86-x64-AllOS-ENU.exe NDP472-KB054530-x86-x64-AllOS-ENU.ex~ >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto chk_logs

if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\.NET_4.7.2 goto chk_logs
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\.NET_4.7.2\NDP472-KB054530-x86-x64-AllOS-ENU.exe goto chk_logs
ren %TGT_DRV%%BASE_PATH%\Instrument\Tools\.NET_4.7.2\NDP472-KB054530-x86-x64-AllOS-ENU.exe NDP472-KB054530-x86-x64-AllOS-ENU.ex~ >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if errorlevel 0 goto chk_logs
set DISPLAY_TIMEOUT=300
set ERR_MSG=Error in post .NET installation! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto chk_logs

:remove_old
if not exist %TGT_DRV%%BASE_PATH%\Instrument\%UI_NAME%.msi goto rm_old1
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.ex~ goto do_remove
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe goto do_remove
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\%UI_NAME%.msi >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto rm_old2

:do_remove
msiexec /x %OLD_INSTALLER% /passive >> %TGT_DRV%%BASE_PATH%\Instrument\Installer.log >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\%UI_NAME%.msi >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

:rm_old1
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.ex~ del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.ex~ >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

:rm_old2
if exist %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe.config del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Software\%UI_NAME%.exe.config >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\ViCell_install.exe del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\ViCell_install.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Software\* goto rm_old3
rmdir /S /Q %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Software mkdir %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Tools\Empty_ViCellUI.exe copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Tools\Empty_ViCellUI.exe %TGT_DRV%%BASE_PATH%\Instrument\Software\ViCellUI.exe >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Carousel.ico copy /Y /V %TGT_DRV%%BASE_PATH%\Instrument\Carousel.ico %TGT_DRV%%BASE_PATH%\Instrument\Software >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

:rm_old3
if exist %TGT_DRV%%BASE_PATH%\Instrument\bin\* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\bin\* >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\bin\Archive\* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\bin\Archive\* >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Config\* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Config\* >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Config\Backup\* del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Config\Backup\* >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if not exist %TGT_DRV%%BASE_PATH%\Instrument\Config\Installed\* goto chk_logs
attrib -r %TGT_DRV%%BASE_PATH%\Instrument\Config\Installed\* >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Config\Installed\* >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\*.manifest.txt del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\*.manifest.txt >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
goto chk_logs

:bad_param
set ERR_MSG=Illegal or unrecognized installation parameter: '%CHK_PARAM%'!
goto setup_error

:bad_drv_param
set ERR_MSG=Unrecognized installation drive parameter: '%CHK_DRV%'!
goto setup_error

:missing_drv_param
set ERR_MSG=Missing installation target drive parameter!
goto setup_error

:missing_path_param
set ERR_MSG=Missing installation target path parameter!
goto setup_error

:no_src
set ERR_MSG=Instrument type target installer not found on accessible drives.
goto set_error

:no_inst
set ERR_MSG=Instrument type target installer not found.
goto set_error

:bad_inst
set ERR_MSG=Wrong Instrument type installer.
goto set_error

:no_installer_found
set ERR_MSG=Specified source installer archive file '%SRC_DRV%\%INSTALLER_PN%*.exe' not found.
goto set_error

:no_loader_found
set ERR_MSG=Installer-loader not found at specified path '%TGT_DRV%%BASE_PATH%\Instrument'.
goto set_error

:installer_rename_failed
set ERR_MSG=Source installer archive file rename failed.
goto set_error

:no_local_archive
set ERR_MSG=Local installer archive file not found.
goto set_error

:no_tgt_path
set ERR_MSG=Install path not found or invalid.
goto set_error

:no_uinam
set ERR_MSG=Unknown UI name.
goto set_error

:no_hash_key
set ERR_MSG=No installer package file integrity verification key found!
goto set_error

:no_hash_tgt
set ERR_MSG=File integrity verification tool not found on expected drive!
goto set_error

:no_hash_tool
set ERR_MSG=File integrity verification tool not found!
goto set_error

:hash_tool_copy_err
set ERR_MSG=Error copying file integrity verification tool to expected folder! Possibly non-admin user!
goto set_error

:hash_chk_failed
set ERR_MSG=File integrity verification failure on target update package.
goto set_error

:unpack_failed
set ERR_MSG=Installer unpack failure on target update package.
goto set_error

:installer_failed
set ERR_MSG=Application install failure on target update package.
goto set_error

:no_env
set ERR_MSG=No environment configuration! System may not run in the intended mode!
goto set_error

:ui_fail
set ERR_MSG=ViCell UI install failure. UI file not found.
goto set_error

:no_batch
set ERR_MSG=ViCell install script not found.
goto set_error

:copy_error
set ERR_MSG=Installer copy verification failed!
goto set_error

:old_not_found
set ERR_MSG=Specified source installer archive file '%SRC_DRV%\%INSTALLER_PN%*.exe' not found.
@echo No previous installation found in folder (%TGT_DRV%%BASE_PATH%\Instrument)!
@echo No previous installation found in folder (%TGT_DRV%%BASE_PATH%\Instrument)! >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

:setup_error
set DISPLAY_TIMEOUT=300
goto set_error1

:set_error
rem - errors cause the installation to pause until the operator dismisses it; no need to do a timeout pause...
set DISPLAY_TIMEOUT=

:set_error1
@echo. >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
@echo %ERR_MSG% >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
@echo %ERR_MSG%

:chk_logs
if not "%LOADER_DBG%x" == "x" pause
if exist %TGT_DRV%%BASE_PATH%\Instrument\Installer.log goto combine_logs
@echo. >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
@echo "No Installer log found!" >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
ren %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log Install.log
goto finish_log

:combine_logs
@echo. >> %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log
copy /a %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log+%TGT_DRV%%BASE_PATH%\Instrument\Installer.log %TGT_DRV%%BASE_PATH%\Instrument\Install.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\Installer.log del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\Installer.log
if exist %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log del /F /Q %TGT_DRV%%BASE_PATH%\Instrument\InstallLoader.log

:finish_log
if "%ERR_MSG%x" == "x" goto no_errors
set ERRORS_FOUND=  Errors encountered during installation/upgrade!
if "%SIM_INSTALL%x" == "x" goto done
set INSTALLER_ERRORS=%ERRORS_FOUND%: %ERR_MSG%
goto done

:no_errors
set ERRORS_FOUND=  Installation/upgrade completed with no detected errors.

:done
@echo. >> %TGT_DRV%%BASE_PATH%\Instrument\Install.log
@echo Finished ViCell-BLU Installation.%ERRORS_FOUND% >> %TGT_DRV%%BASE_PATH%\Instrument\Install.log
@echo. >> %TGT_DRV%%BASE_PATH%\Instrument\Install.log

@echo.
@echo Finished ViCell-BLU Installation.%ERRORS_FOUND%
@echo.
@echo Inspect the installer log file '%TGT_DRV%%BASE_PATH%\Instrument\Install.log' for details.
@echo.
if not "%ERR_MSG%x" == "x" pause

set ERRORS_FOUND=
set ERR_MSG=
set SRC_DRV=
set INSTALLER_PN=
set INST_TYPE_PN=
set UI_NAME=
set INSTALLER_NAME=

popd

:xit
rem - all valid instrument OS versions SHOULD have whitelisting installed; missing components should generate a message
if exist \Cell-Health-OS-v*.txt goto xit1
if exist \Instrument\Cell-Health-OS-v*.txt goto xit1
@echo Non-standard installation OS!

:xit1
if exist "C:\Program Files\McAfee\Solidcore\sadmin.exe" goto xit2
@echo Missing application control component expected in instrument OS configuration!
goto xit3

:xit2
@sadmin end-update

:xit3
if not "%LOADER_DBG%x" == "x" pause

set TGT_DRV=
set TGT_DRV_FLAG=
set BASE_PATH=
set TGT_PATH_FLAG=
set LOADER_NAME=
set LOADER_DBG=
set FIND_DBG=
set FIND_DBG_FLAG=
set INSTALLER_DBG_FLAG=
set INSTALLER_ERRORS=
set FORCE_FLAG=
set PRESERVE_FLAG=
set SIM_INSTALL=
set SIM_INSTALL_FLAG=
set DO_REMOVE=
set CHK_DRV=
set CHK_PARAM=

if not "%DISPLAY_TIMEOUT%x" == "x" timeout /T %DISPLAY_TIMEOUT%

set DISPLAY_TIMEOUT=
