@echo off
cls
set ADD_FLAG=
set TGT_FILE=
set TGT_PATH=
set TGT_DRV=
set TGT_DRV_CHK=
set SET_PATH=
set SET_FILE=
set SET_FLAG=
set CHK_PATH=
set HASH_FILE=
set HASH_FLAG=
set HASH_MODE=
set HASH_PATH=
set NEXT_FLAG=
set FULL_PATH_FLAG=
set START_DRV=
set START_PATH=
set NEW_PATH=

set START_PATH=%CD%
set START_DRV=%CD:~0,2%

: preserve the starting location
pushd %START_PATH%

: check for either a dash or slash as the parameter indicator
set PFLAG_START=
set PFLAG=-

:param_loop
if %1x == x goto start_chk
if %1x == %PFLAG%Ax goto set_add_flag
if %1x == %PFLAG%ax goto set_add_flag
if %1x == %PFLAG%Cx goto set_cmode_param
if %1x == %PFLAG%cx goto set_cmode_param
if %1x == %PFLAG%Dx goto set_dbg
if %1x == %PFLAG%dx goto set_dbg
if %1x == %PFLAG%Fx goto set_tgt_file
if %1x == %PFLAG%fx goto set_tgt_file
if %1x == %PFLAG%Hx goto set_hash_file
if %1x == %PFLAG%hx goto set_hash_file
if %1x == %PFLAG%HPx goto set_hash_path
if %1x == %PFLAG%hpx goto set_hash_path
if %1x == %PFLAG%Lx goto set_lmode_param
if %1x == %PFLAG%lx goto set_lmode_param
if %1x == %PFLAG%Px goto set_tgt_path
if %1x == %PFLAG%px goto set_tgt_path
if %1x == %PFLAG%Rx goto set_rmode_param
if %1x == %PFLAG%rx goto set_rmode_param
if %1x == %PFLAG%Vx goto set_vmode_param
if %1x == %PFLAG%vx goto set_vmode_param
if %1x == %PFLAG%VFx goto set_vfmode_param
if %1x == %PFLAG%vfx goto set_vfmode_param
if %1x == %PFLAG%Qx goto xit
if %1x == %PFLAG%qx goto xit
if %1x == %PFLAG%Xx goto xit
if %1x == %PFLAG%xx goto xit
if %1x == %PFLAG%?x goto usage
if %1x == ?x goto usage

if %PFLAG%x == /x goto dash_param_flag
set NEXT_FLAG=/
goto chk_next_param_flag

:dash_param_flag
set NEXT_FLAG=-

:chk_next_param_flag
if not %PFLAG_START%x == x goto chk_end_param_flag
set PFLAG_START=%PFLAG%
goto set_next_param_flag

:chk_end_param_flag
if %PFLAG_START%x == %NEXT_FLAG%x goto bad_param

:set_next_param_flag
set PFLAG=%NEXT_FLAG%
goto param_loop

:set_dbg
@echo on
goto next_param

:set_add_flag
set ADD_FLAG= -add
goto next_param

:set_cmode_param
set HASH_FLAG=-c
goto next_param

:set_tgt_file
if "%2x" == "x" goto no_file_param
shift
set TGT_FILE=%1
goto next_param

:set_hash_file
if "%2x" == "x" goto no_file_param
shift
set HASH_FILE=%1
if %HASH_FILE%x == folder.xmlx set HASH_FILE=folder
if %HASH_FILE%x == f set HASH_FILE=folder
if %HASH_FILE%x == hash.xmlx set HASH_FILE=hash
if %HASH_FILE%x == h set HASH_FILE=hash
goto next_param

:set_hash_path
if "%2x" == "x" goto no_path_param
shift
if not exist "%1" goto bad_path_param
set HASH_PATH=%1
goto next_param

:set_lmode_param
set HASH_FLAG=-l
goto next_param

:set_vmode_param
set HASH_FLAG=-v
goto next_param

:set_rmode_param
set HASH_FLAG=-r
: recursive mode requies a path parameter be supplied
goto mode_path_chk

:set_vfmode_param
set HASH_FLAG=-vf
: folder verify mode requies a path parameter be supplied

:mode_path_chk
if not "%2x" == "x" goto set_tgt_path2
if "%TGT_PATH%x" == "x" goto no_path_param
goto next_param

:set_tgt_path
if "%2x" == "x" goto no_path_param

:set_tgt_path2
shift
if not exist "%1" goto bad_path_param
set TGT_PATH=%1

:next_param
set PFLAG_START=
shift
goto param_loop

:start_chk
if not %HASH_FLAG%x == x goto set_mode

:get_mode
@echo.
@echo "Enter the hash mode flag to use:                                    "
@echo "    -a to add a hash value to an existing hash file                 "
@echo "    -c to create the hashes for a single file                       "
@echo "    -l to list available hashs in the hash file                     "
@echo "    -r to create the hashes for all contents of a folder structure  "
@echo "    -v for single file verify                                       "
@echo "    -vf for folder verify                                           "
@echo "    -z clear all mode flags and restart                             "
@echo "    -q or -x to quit                                                "
@echo "    -? or ? for help                                                "
@echo.
set /p SET_FLAG=Enter mode flag: 
if %SET_FLAG%x == x goto no_mode_entered
if %SET_FLAG%x == -Qx goto xit
if %SET_FLAG%x == -qx goto xit
if %SET_FLAG%x == -Xx goto xit
if %SET_FLAG%x == -xx goto xit
if %SET_FLAG%x == -?x goto usage
if %SET_FLAG%x == ?x goto usage
if %SET_FLAG%x == -Ax goto set_amode
if %SET_FLAG%x == -ax goto set_amode
set HASH_FLAG=%SET_FLAG%
goto set_mode

:no_mode_entered
@echo.
@echo No hash operation mode entered!
goto get_mode

:set_mode
if %HASH_FLAG%x == -Cx goto set_cmode
if %HASH_FLAG%x == -cx goto set_cmode
if %HASH_FLAG%x == -Lx goto set_lmode
if %HASH_FLAG%x == -lx goto set_lmode
if %HASH_FLAG%x == -Rx goto set_rmode
if %HASH_FLAG%x == -rx goto set_rmode
if %HASH_FLAG%x == -Vx goto set_vmode
if %HASH_FLAG%x == -vx goto set_vmode
if %HASH_FLAG%x == -VFx goto set_vfmode
if %HASH_FLAG%x == -vfx goto set_vfmode
if %HASH_FLAG%x == -Zx goto clr_mode
if %HASH_FLAG%x == -zx goto clr_mode
if %HASH_FLAG%x == x goto no_mode
@echo.
echo Unrecognized mode entered! "%HASH_FLAG%"
goto usage

:set_amode
set ADD_FLAG= -add
if "%HASH_MODE%x" == "x" goto set_amode1
if "%HASH_MODE%x" == "chkx" goto reset_mode
if "%HASH_MODE%x" == "chkfx" goto reset_mode
if "%HASH_MODE%x" == "listx" goto reset_mode
goto tgt_path_chk

:reset_mode
@echo.
set /p SET_FLAG='-a' and %HASH_FLAG% are mutually exclusive.  Continue with the '-a' mode? 
if %SET_FLAG%x == Nx goto clr_add
if %SET_FLAG%x == nx goto clr_add
if %SET_FLAG%x == Yx goto reset_mode1
if %SET_FLAG%x == yx goto reset_mode1
if %SET_FLAG%x == -Qx goto xit
if %SET_FLAG%x == -qx goto xit
if %SET_FLAG%x == -Xx goto xit
if %SET_FLAG%x == -xx goto xit
if %SET_FLAG%x == x goto no_reset_entry
@echo.
echo Unrecognized entry "%SET_FLAG%"
goto reset_mode

:no_reset_entry
@echo.
echo No response entered!
goto reset_mode

:reset_mode1
@echo.
@echo Mode will be reset to '-c' by default.
set HASH_MODE=
goto set_amode2

:set_amode1
@echo.
@echo Mode will be set to '-c' by default.

:set_amode2
@echo.
set /p SET_FLAG=Do you want to change the mode ('-a' flag will be kept if appropriate, otherwise cleared)? 
if %SET_FLAG%x == Nx goto set_cmode
if %SET_FLAG%x == nx goto set_cmode
if %SET_FLAG%x == Yx goto set_make_mode
if %SET_FLAG%x == yx goto set_make_mode
if %SET_FLAG%x == -Qx goto xit
if %SET_FLAG%x == -qx goto xit
if %SET_FLAG%x == -Xx goto xit
if %SET_FLAG%x == -xx goto xit
if %SET_FLAG%x == x goto no_amode_entry
@echo.
echo Unrecognized entry "%SET_FLAG%"
goto usage

:no_amode_entry
@echo.
echo No response entered!
goto set_amode2

:set_make_mode
if "%HASH_MODE%x" == "x" set HASH_MODE=make
set HASH_FLAG=-c
@echo "Current hash mode flags: %HASH_FLAG% -add                                    "
@echo "                                                                    "
goto get_mode

:clr_mode
set HASH_MODE=
set HASH_FLAG=
set ADD_FLAG=
goto get_mode

:set_cmode
set HASH_MODE=make
if "%HASH_FILE%x" == "x" set HASH_FILE=hash
goto tgt_path_chk

:set_lmode
set HASH_MODE=list
if "%HASH_PATH%x" == "x" goto clr_add
set TGT_PATH=%HASH_PATH%
goto clr_add

:set_rmode
set HASH_MODE=folder
if "%HASH_FILE%x" == "x" set HASH_FILE=folder
if "%HASH_PATH%x" == "x" set HASH_PATH=..
goto tgt_path_chk

:set_vmode
set HASH_MODE=chk
if "%HASH_PATH%x" == "x" goto clr_add
set TGT_PATH=%HASH_PATH%
goto clr_add

:set_vfmode
set HASH_MODE=chkf
if "%HASH_PATH%x" == "x" goto clr_add
set TGT_PATH=%HASH_PATH%

:clr_add
set ADD_FLAG=

:tgt_path_chk
@echo.
echo Current path value is "%TGT_PATH%"
@echo.
set SET_PATH=
set /p SET_PATH=Do you want to set or change the path? 
if %SET_PATH%x == Qx goto xit
if %SET_PATH%x == qx goto xit
if %SET_PATH%x == Xx goto xit
if %SET_PATH%x == xx goto xit
if %SET_PATH%x == Yx goto get_tgt_path
if %SET_PATH%x == yx goto get_tgt_path
if %SET_PATH%x == Nx goto hash_folder_chk
if %SET_PATH%x == nx goto hash_folder_chk
if %SET_PATH%x == x goto no_tgt_path_response
@echo.
echo Unrecognized entry! "%SET_PATH%"
goto tgt_path_chk

:no_tgt_path_response
@echo.
echo No response entered!
goto tgt_path_chk

:get_tgt_path
@echo.
set /p TGT_PATH=Enter the path to the file folder (without terminating '\'), or 'q' or 'x' to quit: 
if "%TGT_PATH%x" == "Qx" goto xit
if "%TGT_PATH%x" == "qx" goto xit
if "%TGT_PATH%x" == "Xx" goto xit
if "%TGT_PATH%x" == "xx" goto xit
if "%TGT_PATH%x" == "x" echo no path entered!
if exist "%TGT_PATH%" goto hash_folder_chk
echo Path "%TGT_PATH%" not found!
goto tgt_path_chk

:no_tgt_path_entered
@echo.
echo No path entered!
goto get_tgt_path

:hash_folder_chk
if %HASH_MODE% == folder goto chk_hash_folder1
if %HASH_MODE% == chkf goto chk_hash_folder1
if %HASH_MODE% == chk goto chk_hash_folder2
if %HASH_MODE% == list goto chk_hash_folder2
goto tgt_file_chk

:chk_hash_folder1
if not "%HASH_PATH%x" == "x" goto set_folder_files
set HASH_PATH=..
if %HASH_MODE% == folder goto set_folder_files
set HASH_FILE=folder
if exist "%TGT_PATH%\%HASH_PATH%\%HASH_FILE%" goto set_folder_files
set HASH_PATH=%TGT_PATH%
if not exist "%HASH_PATH%\%HASH_FILE%.xml" goto no_hash_file
goto set_folder_files

:chk_hash_folder2
if not "%HASH_PATH%x" == "x" goto chk_hash_folder3
set HASH_PATH=%TGT_PATH%

:chk_hash_folder3
set HASH_FILE=hash
if exist "%HASH_PATH%\%HASH_FILE%.xml" goto chk_hash_folder4
set HASH_FILE=folder
if exist "%HASH_PATH%\%HASH_FILE%.xml" goto chk_hash_folder4
set HASH_PATH=..
if exist "%TGT_PATH%\%HASH_PATH%\%HASH_FILE%.xml" chk_hash_folder5
set HASH_FILE=hash
if not exist "%TGT_PATH%\%HASH_PATH%\%HASH_FILE%.xml" goto no_hash_file

:chk_hash_folder4
if %HASH_MODE% == chk goto tgt_file_chk
goto start

:tgt_file_chk
@echo.
echo Current filename is "%TGT_FILE%"
set SET_FILE=
set /p SET_FILE=Do you want to set or change the filename? 
if %SET_FILE%x == Qx goto xit
if %SET_FILE%x == qx goto xit
if %SET_FILE%x == Xx goto xit
if %SET_FILE%x == xx goto xit
if %SET_FILE%x == Yx goto get_file
if %SET_FILE%x == yx goto get_file
if %SET_FILE%x == Nx goto tgt_file_chk2
if %SET_FILE%x == nx goto tgt_file_chk2
if %SET_FILE%x == x goto no_tgt_file_response
@echo.
echo Unrecognized entry! "%SET_FILE%"
goto tgt_file_chk

:no_tgt_file_response
@echo.
echo No response entered!
goto tgt_file_chk

:tgt_file_chk2
if not "%TGT_FILE%x" == "x" goto tgt_file_ok
if %HASH_MODE% == chk goto set_std_files
if %HASH_MODE% == chkf goto set_folder_files
goto start

:get_file
@echo.
set /p TGT_FILE=Enter the file name, or 'q' or 'x' to quit: 
if "%TGT_FILE%x" == "Qx" goto xit
if "%TGT_FILE%x" == "qx" goto xit
if "%TGT_FILE%x" == "Xx" goto xit
if "%TGT_FILE%x" == "xx" goto xit
if "%TGT_FILE%x" == "x" echo no filename entered!

:tgt_file_ok
set CHK_PATH=%TGT_PATH%
if "%TGT_PATH%x" == "x" set CHK_PATH=%CD%
:if "%TGT_PATH%x" == "x" set CHK_PATH=.
if not exist "%CHK_PATH%\%TGT_FILE%" goto no_tgt_file
if "%TGT_PATH%x" == "x" set TGT_PATH=%CD%
:if "%TGT_PATH%x" == "x" set TGT_PATH=.
goto start

:no_tgt_file
echo File "%CHK_PATH%\%TGT_FILE%" not found!
goto tgt_path_chk

:set_std_files
set TGT_FILE=hash.xml

:set_std_files2
if not "%HASH_FILE%x" == "x" goto start
set HASH_FILE=hash
goto start

:set_folder_files
set TGT_FILE=folder.xml

:set_folder_files2
if not "%HASH_FILE%x" == "x" goto start
set HASH_FILE=folder

:start
if not %TGT_FILE%x == x goto start_mode_chk
if %HASH_MODE% == chk set_std_files
if %HASH_MODE% == chkf set_folder_files

:start_mode_chk
if "%TGT_PATH%x" == "x" set TGT_PATH=%CD%
:if "%TGT_PATH%x" == "x" set TGT_PATH=.
if %HASH_MODE%x == x goto no_mode

:startup_chk
set TGT_DRV=%TGT_PATH:~0,2%
set TGT_DRV_CHK=%TGT_PATH:~1,1%
if not "%TGT_DRV_CHK%x" == ":x" set TGT_DRV=%START_DRV%
if not "%TGT_DRV%x" == "%START_DRV%x" pushd "%TGT_DRV%"
set NEW_PATH=%CD%

if %HASH_MODE% == make goto make_file_hash
if %HASH_MODE% == folder goto make_folder_hash
if %HASH_MODE% == chk goto chk_hash
if %HASH_MODE% == chkf goto chk_hash
if %HASH_MODE% == list goto list_hash
goto bad_mode

:chk_hash
if not "%TGT_PATH%x" == "%NEW_PATH%x" pushd "%TGT_PATH%"
if "%TGT_FILE%x" == "hash.xmlx" set HASH_FILE=hash
if "%TGT_FILE%x" == "folder.xmlx" set HASH_FILE=folder
if "%HASH_PATH%x" == "x" set HASH_PATH=%TGT_PATH%
if %HASH_MODE% == chkf goto chk_hash2
fciv.exe -v %TGT_FILE% -both -xml "%HASH_PATH%\%HASH_FILE%.xml"
goto chk_hash_xit

:chk_hash2
fciv.exe -v -both -xml "%HASH_PATH%\%HASH_FILE%.xml"

:chk_hash_xit
if not "%TGT_PATH%x" == "%NEW_PATH%x" popd
goto xit_wait

:make_file_hash
if "%HASH_PATH%x" == "x" set HASH_PATH=%TGT_PATH%
if "%HASH_PATH%x" == "..x" set HASH_PATH=%TGT_PATH%\..
if "%HASH_FILE%x" == "x" set HASH_FILE=hash
if %HASH_FILE% == hash set FULL_PATH_FLAG= -wp
if not "%ADD_FLAG%x" == "x" goto make_file_hash1
if exist "%HASH_PATH%\%HASH_FILE%.xml" del /F /Q "%HASH_PATH%\%HASH_FILE%.xml"
if exist "%HASH_PATH%\%HASH_FILE%.txt" del /F /Q "%HASH_PATH%\%HASH_FILE%.txt"

:make_file_hash1
if not "%HASH_PATH%x" == "%NEW_PATH%x" pushd "%HASH_PATH%"
if not "%TGT_PATH%x" == "%HASH_PATH%x" pushd "%TGT_PATH%"
fciv.exe%ADD_FLAG% "%TGT_PATH%\%TGT_FILE%"%FULL_PATH_FLAG% -both -xml "%HASH_PATH%\%HASH_FILE%.xml"
goto show_hash

:make_folder_hash
if "%HASH_PATH%x" == "x" set HASH_PATH=..
if "%HASH_FILE%x" == "x" set HASH_FILE=folder
if "%HASH_PATH%x" == "..x" set HASH_PATH=%TGT_PATH%\..
if exist "%HASH_PATH%" goto make_folder_hash1
set HASH_PATH=%TGT_PATH%
goto make_folder_hash2

:make_folder_hash1
if exist "%TGT_PATH%\%HASH_FILE%.xml" del /F /Q "%TGT_PATH%\%HASH_FILE%.xml"
if exist "%TGT_PATH%\%HASH_FILE%.txt" del /F /Q "%TGT_PATH%\%HASH_FILE%.txt"

:make_folder_hash2
if not "%ADD_FLAG%x" == "x" goto make_folder_hash3
if exist "%HASH_PATH%\%HASH_FILE%.xml" del /F /Q "%HASH_PATH%\%HASH_FILE%.xml"
if exist "%HASH_PATH%\%HASH_FILE%.txt" del /F /Q "%HASH_PATH%\%HASH_FILE%.txt"

:make_folder_hash3
if not "%HASH_PATH%x" == "%NEW_PATH%x" pushd "%HASH_PATH%"
if not "%TGT_PATH%x" == "%HASH_PATH%x" pushd "%TGT_PATH%"
fciv.exe%ADD_FLAG% %TGT_PATH% -r -exc %TGT_PATH%\hash-excludes.txt -both -xml %HASH_PATH%\%HASH_FILE%.xml

:show_hash
: after generating the hash keys, list them from the key fileinto the text file, then send the list to the console
fciv.exe -list -both -xml "%HASH_PATH%\%HASH_FILE%.xml" > "%HASH_PATH%\%HASH_FILE%.txt"
type "%HASH_PATH%\%HASH_FILE%.txt"
if not "%TGT_PATH%x" == "%HASH_PATH%x" popd
if not "%HASH_PATH%x" == "%NEW_PATH%x" popd
goto xit_wait

:list_hash
fciv.exe -list -both -xml "%HASH_PATH%\%HASH_FILE%.xml"
goto xit_wait

:no_file_param
echo Missing file parameter!
goto usage

:no_path_param
echo Missing path parameter!
goto usage

:no_hash_file
echo No hash file found!
goto usage

:bad_path_param
echo Invalid path parameter!
goto usage

:bad_mode
echo unrecognized hash operation mode!
goto usage

:no_mode
echo No hash operation mode!
goto usage

:bad_param
echo Unrecognized parameter "%1"!

:usage
@echo.
@echo "USAGE: GetHash.bat [options]                                                       "
@echo "  NOTE: the hash key file always uses standard names for the output hash key file. "
@echo "        the standard file names are 'hash.xml' for single file hashes, and         "
@echo "        'folder.xml' for folder content file hashes                                "
@echo "                                                                                   "
@echo "    -a             create the file hash and add to the xml hash key file           "
@echo "                   hash mode may be specified separately as -c (default) or -r     "
@echo "    -c             create the file hash and store in the xml hash key file         "
@echo "                   removes any previous hash key file found                        "
@echo "    -f <filename>  specify hash target file name                                   "
@echo "    -h <filename>  specify hash file name {hash | h; folder | f)                   "
@echo "    -hp <filepath> specify path to the hash file (without terminating '\')         "
@echo "    -l             list the hashes contained in the xml hash key file              "
@echo "    -p <filepath>  specify target file path (without terminating '\')              "
@echo "    -r <filepath>  create the hash for all files in a folder structure and         "
@echo "                   store in the xml hash key file                                  "
@echo "    -v             check file hash against the xml hash key file                   "
@echo "    -vf <filepath> check folder files hash against the xml hash key file           "
@echo "             NOTE: folder hash file is placed in the parent of the target folder   "
@echo "    -? or ?        display this help                                               "
@echo.
goto xit

:xit_wait
echo.
timeout /T 300

:xit
if not "%TGT_DRV%x" == "%START_DRV%x" popd
popd

set ADD_FLAG=
set TGT_FILE=
set TGT_PATH=
set TGT_DRV=
set TGT_DRV_CHK=
set SET_PATH=
set SET_FILE=
set SET_FLAG=
set CHK_PATH=
set HASH_FILE=
set HASH_FLAG=
set HASH_MODE=
set HASH_PATH=
set NEXT_FLAG=
set FULL_PATH_FLAG=
set START_DRV=
set START_PATH=
set NEW_PATH=

set PFLAG_START=
set PFLAG=
