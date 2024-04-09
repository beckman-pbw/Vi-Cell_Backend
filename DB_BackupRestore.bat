@echo off
@cls

rem save any PostgreSQL environment user name
set PGUSEROLD=%PGUSER%
set PGUSER=
set PGPASSWORD=
set PGDATABASEOLD=%PGDATABASE%
set PGDATABASE=
set PGPATH=
set DB_FILE=
set DB_FILE_PATH=
set FULL_PATH=
set FILE_MONTH=
set FILE_DAY=
set FILE_YEAR=
set FILE_DATE=
set OP_MODE=
set BACKUP_TGT=
set BACKUP_FLAG=
set VERBOSE_MODE=
set DISPLAY_TIMEOUT=
set DEBUG_MODE=
set MCAFEE_MODE=
set OVERWRITE=
set FLAG_CHAR=-


: all valid instrument OS versions SHOULD have whitelisting installed; missing components should generate a message
if exist C:\Cell-Health-OS-v*.txt goto mcafee_chk
if exist C:\Instrument\Cell-Health-OS-v*.txt goto mcafee_chk
if exist \Cell-Health-OS-v*.txt goto mcafee_chk
if exist \Instrument\Cell-Health-OS-v*.txt goto mcafee_chk
@echo Non-standard installation OS!

:mcafee_chk
if exist "C:\Program Files\McAfee\Solidcore\sadmin.exe" goto mcafee_ok
@echo Missing application control component expected in instrument OS configuration!
goto param_loop

:mcafee_ok
set MCAFEE_MODE=TRUE
@sadmin begin-update
if errorlevel == 0 goto param_loop
@echo Error entering update mode! Non-admin environment!
set DISPLAY_TIMEOUT=300
goto xit

:param_loop
if "%1x" == "x" goto start
if "%1x" == "%FLAG_CHAR%Bx" goto set_backup_op_mode
if "%1x" == "%FLAG_CHAR%bx" goto set_backup_op_mode
if "%1x" == "%FLAG_CHAR%Cx" goto set_full_tgt
if "%1x" == "%FLAG_CHAR%cx" goto set_full_tgt
if "%1x" == "%FLAG_CHAR%Dx" goto set_debug
if "%1x" == "%FLAG_CHAR%dx" goto set_debug
if "%1x" == "%FLAG_CHAR%Fx" goto set_db_file
if "%1x" == "%FLAG_CHAR%fx" goto set_db_file
if "%1x" == "%FLAG_CHAR%Mx" goto set_mode
if "%1x" == "%FLAG_CHAR%mx" goto set_mode
if "%1x" == "%FLAG_CHAR%Ox" goto set_data_tgt
if "%1x" == "%FLAG_CHAR%Ox" goto set_data_tgt
if "%1x" == "%FLAG_CHAR%Px" goto set_file_path
if "%1x" == "%FLAG_CHAR%px" goto set_file_path
if "%1x" == "%FLAG_CHAR%Rx" goto set_restore_op_mode
if "%1x" == "%FLAG_CHAR%rx" goto set_restore_op_mode
if "%1x" == "%FLAG_CHAR%Tx" goto set_backup_tgt
if "%1x" == "%FLAG_CHAR%tx" goto set_backup_tgt
if "%1x" == "%FLAG_CHAR%Vx" goto set_verbose_mode
if "%1x" == "%FLAG_CHAR%vx" goto set_verbose_mode
if "%1x" == "%FLAG_CHAR%V-x" goto set_verbose_mode_off
if "%1x" == "%FLAG_CHAR%v-x" goto set_verbose_mode_off
if "%1x" == "%FLAG_CHAR%?x" goto show_help
if "%1x" == "?x" goto show_help
if "%1x" == "%FLAG_CHAR%HELPx" goto show_help
if "%1x" == "%FLAG_CHAR%helpx" goto show_help
if "%FLAG_CHAR%x" == "/x" goto unrecognized_param
set FLAG_CHAR=/
goto param_loop

:set_db_file
if "%2x" == "x" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%Bx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%bx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%Cx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%cx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%Dx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%dx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%Fx" goto next_param
if "%2x" == "%FLAG_CHAR%fx" goto next_param
if "%2x" == "%FLAG_CHAR%Mx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%mx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%Ox" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%ox" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%Px" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%px" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%Rx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%rx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%Tx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%tx" goto missing_db_file_param
if "%1x" == "%FLAG_CHAR%Vx" goto missing_db_file_param
if "%1x" == "%FLAG_CHAR%vx" goto missing_db_file_param
if "%1x" == "%FLAG_CHAR%V-x" goto missing_db_file_param
if "%1x" == "%FLAG_CHAR%v-x" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%?x" goto missing_db_file_param
if "%2x" == "?x" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%HELPx" goto missing_db_file_param
if "%2x" == "%FLAG_CHAR%helpx" goto missing_db_file_param
shift
set DB_FILE=%1
goto next_param

:set_file_path
if "%2x" == "x" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%Bx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%bx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%Cx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%cx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%Dx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%dx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%Fx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%fx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%Mx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%mx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%Ox" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%ox" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%Px" goto next_param
if "%2x" == "%FLAG_CHAR%px" goto next_param
if "%2x" == "%FLAG_CHAR%Rx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%rx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%Tx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%tx" goto missing_db_path_param
if "%1x" == "%FLAG_CHAR%Vx" goto missing_db_path_param
if "%1x" == "%FLAG_CHAR%vx" goto missing_db_path_param
if "%1x" == "%FLAG_CHAR%V-x" goto missing_db_path_param
if "%1x" == "%FLAG_CHAR%v-x" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%?x" goto missing_db_path_param
if "%2x" == "?x" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%HELPx" goto missing_db_path_param
if "%2x" == "%FLAG_CHAR%helpx" goto missing_db_path_param
shift
set DB_FILE_PATH=%1
goto next_param

:set_mode
if "%2x" == "x" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%Bx" goto next_param
if "%2x" == "%FLAG_CHAR%bx" goto next_param
if "%2x" == "%FLAG_CHAR%Cx" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%cx" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%Dx" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%dx" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%Fx" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%fx" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%Mx" goto next_param
if "%2x" == "%FLAG_CHAR%mx" goto next_param
if "%2x" == "%FLAG_CHAR%Ox" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%ox" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%Px" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%px" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%Rx" goto next_param
if "%2x" == "%FLAG_CHAR%rx" goto next_param
if "%2x" == "%FLAG_CHAR%Tx" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%tx" goto missing_mode_param
if "%1x" == "%FLAG_CHAR%Vx" goto missing_mode_param
if "%1x" == "%FLAG_CHAR%vx" goto missing_mode_param
if "%1x" == "%FLAG_CHAR%V-x" goto missing_mode_param
if "%1x" == "%FLAG_CHAR%v-x" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%?x" goto missing_mode_param
if "%2x" == "?x" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%HELPx" goto missing_mode_param
if "%2x" == "%FLAG_CHAR%helpx" goto missing_mode_param
if "%2x" == "BACKUPx" goto backup_mode_param_ok
if "%2x" == "backupx" goto backup_mode_param_ok
if "%2x" == "RESTOREx" goto restore_mode_param_ok
if "%2x" == "restorex" goto restore_mode_param_ok
goto unrecognized_param

:backup_mode_param_ok
shift

:set_backup_op_mode
set OP_MODE=BACKUP
goto next_param

:restore_mode_param_ok
shift

:set_restore_op_mode
set OP_MODE=RESTORE
goto next_param

:set_backup_tgt
if "%2x" == "x" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Bx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%bx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Cx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%cx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Dx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%dx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Fx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%fx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Mx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%mx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Ox" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%ox" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Px" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%px" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Rx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%rx" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%Tx" goto next_param
if "%2x" == "%FLAG_CHAR%tx" goto next_param
if "%1x" == "%FLAG_CHAR%Vx" goto missing_tgt_param
if "%1x" == "%FLAG_CHAR%vx" goto missing_tgt_param
if "%1x" == "%FLAG_CHAR%V-x" goto missing_tgt_param
if "%1x" == "%FLAG_CHAR%v-x" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%?x" goto missing_tgt_param
if "%2x" == "?x" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%HELP" goto missing_tgt_param
if "%2x" == "%FLAG_CHAR%help" goto missing_tgt_param
if "%2x" == "FULLx" goto full_backup_param_ok
if "%2x" == "fullx" goto full_backup_param_ok
if "%2x" == "DATAx" goto data_backup_param_ok
if "%2x" == "datax" goto data_backup_param_ok
goto unrecognized_param

:full_backup_param_ok
shift

:set_full_tgt
set BACKUP_TGT=FULL
goto next_param

:data_backup_param_ok
shift

:set_data_tgt
set BACKUP_TGT=DATA
goto next_param

:set_debug
set DEBUG_MODE=debug
@echo on
goto next_param

:set_verbose_mode
set VERBOSE_MODE=--verbose
@echo on
goto next_param

:set_verbose_mode_off
set VERBOSE_MODE=
@echo on
goto next_param

:unrecognized_param
echo Unrecognized param to the '%1' flag: '%2'... ignored

:next_param
shift
set FLAG_CHAR=-
if not "%DEBUG_MODE%x" == "x" pause
goto param_loop

:show_help
echo.
@echo =======================================================================================================
@echo "                                                                                                     "
@echo "DB_BackupRestore:                                                                                    "
@echo "                                                                                                     "
@echo "    -b                     : set operating mode directly as 'BACKUP'                                 "
@echo "    -c                     : set backup target mode to full (default)                                "
@echo "    -d                     : enable debug mode; script operation is displayed to a console window    "
@echo "    -f [file-param]        : set the database sql file name (may include full path)                  "
@echo "                           : if full path is specifier, it may include a drive                       "
@echo "                           : NOTE: If no filename is specified, a filename will be created           "
@echo "                           :       using the date and default DB name.                               "
@echo "    -m [mode-param]        : set operating mode as 'BACKUP' or 'RESTORE'                             "
@echo "    -o                     : set backup target mode to data only                                     "
@echo "    -p [path-param]        : set installation path;                                                  "
@echo "                           : path specified may include a drive                                      "
@echo "                           : standalone path MUST end in a backslash character '\'                   "
@echo "                           : NOTE: A path MUST be specified without a filename.                      "
@echo "                           :       If necessary, the filename will be created as noted above.        "
@echo "    -r                     : set operating mode directly as 'RESTORE'                                "
@echo "    -t [FULL | DATA]       : set backup target mode                                                  "
@echo "                           :     FULL: includes full table rebuild (drops/recreates existing table)  "
@echo "                           :     DATA: includes only data (may not restore all data)                 "
@echo "                           : NOTE: If backup mode target is specified, the default is the            "
@echo "                           :       FULL backup target mode.                                          "
@echo "    -v                     : enable backup utility verbose mode (not the same as debug mode)         "
@echo "    -v-                    : disable backup utility verbose mode                                     "
@echo "                                                                                                     "
@echo "    -?                     : show this help                                                          "
@echo "    -help                  : show this help                                                          "
@echo "    ?                      : show this help                                                          "
@echo "                                                                                                     "
@echo =======================================================================================================
echo.
goto xit

:start
if "%OP_MODE%x" == "x" goto no_mode

if exist "C:\Program Files\PostgreSQL\10\bin\psql.exe" goto pg_path_set
if not exist ".\psql.exe" goto no_postgres
set PGPATH=.\
goto set_db_params

:pg_path_set
set PGPATH=C:\Program Files\PostgreSQL\10\bin\

:set_db_params
set PGDATABASE=ViCellDB
set PGUSER=postgres
@set PGPASSWORD=$3rgt$0P

if not "%DB_FILE_PATH%%x" == "x" goto set_path_termination
set DB_FILE_PATH=.

:set_path_termination
set DB_FILE_PATH="%DB_FILE_PATH%\"

rem - remove extra embedded quotes from the path for the check
set DB_FILE_PATH=%DB_FILE_PATH:"=%
if not exist "%DB_FILE_PATH%" goto missing_path

if "%OP_MODE%x" == "BACKUPx" goto chk_backup_file
if "%OP_MODE%x" == "RESTOREx" goto do_restore
goto unrecognized_mode


rem ############################################################################
rem     DB Backup section
rem ############################################################################
:chk_backup_file
if not "%DB_FILE%x" == "x" goto chk_backup

rem Make a filename for the backup if necessary
set FILE_MONTH=%DATE:~4,2%
set FILE_DAY=%DATE:~7,2%%
set FILE_YEAR=%DATE:~10,4%
set FILE_DATE=%FILE_YEAR%-%FILE_MONTH%-%FILE_DAY%
set FILE_DATE=%FILE_DATE: =%
set DB_FILE=ViCellDB_backup-%FILE_DATE%.sql

:chk_backup
if exist "%PGPATH%pg_dump.exe" goto do_backup
goto no_pgdump

:do_backup
if "%DB_FILE%x" == "x" goto no_db_file_name

rem - remove extra embedded quotes from the full filepath
set FULL_PATH=%DB_FILE_PATH:"=%%DB_FILE:"=%
if exist "%FULL_PATH%" goto chk_overwrite
goto run_backup

:chk_overwrite
@echo.
set /p OVERWRITE=File '%FULL_PATH%' exists! Overwrite (Y/n)? 
if "%OVERWRITE%x" == "x" goto run_backup
if "%OVERWRITE%x" == "Yx" goto run_backup
if "%OVERWRITE%x" == "yx" goto run_backup
if "%OVERWRITE%x" == "nx" goto xit
if "%OVERWRITE%x" == "nx" goto xit
@echo.
@echo Unrecognized/illegal response!
goto chk_overwrite

:run_backup
if "%BACKUP_TGT%x" == "x" set set BACKUP_FLAG=--clean --create
if "%BACKUP_TGT%x" == "FULLx" set BACKUP_FLAG=--clean --create
if "%BACKUP_TGT%x" == "DATAx" set BACKUP_FLAG=--data-only

set CMD_PARAMS="%PGPATH%pg_dump.exe" --file="%FULL_PATH%" --host="localhost" --port="5432" --username="%PGUSER%" --no-password --role="%PGUSER%" --format=p --inserts --column-inserts %BACKUP_FLAG% %VERBOSE_MODE% "%PGDATABASE%"

if not "%DEBUG_MODE%x" == "x" pause
if "%DEBUG_MODE%x" == "x" goto run_backup
set CMD_PARAMS=@%CMD_PARAMS%

:run_backup
%CMD_PARAMS%
if %ERRORLEVEL% GEQ 1 goto sql_backup_error
if not exist %FULL_PATH% goto missing_db_backup_file
goto xit


rem ############################################################################
rem     DB Restore section
rem ############################################################################
:do_restore
if "%DB_FILE%x" == "x" goto no_db_file_name

rem - remove extra embedded quotes from the filepath
set FULL_PATH=%DB_FILE_PATH:"=%%DB_FILE:"=%

if not exist "%FULL_PATH%" goto missing_db_restore_file

if "%VERBOSE_MODE%x" == "x" set VERBOSE_MODE=--quiet
set CMD_PARAMS="%PGPATH%psql.exe" %VERBOSE_MODE% --file="%FULL_PATH%" --dbname=%PGDATABASE% --username=%PGUSER% %VERBOSE_MODE%

if not "%DEBUG_MODE%x" == "x" pause
if "%DEBUG_MODE%x" == "x" goto run_restore
set CMD_PARAMS=@%CMD_PARAMS%

:run_restore
%CMD_PARAMS%
if %ERRORLEVEL% GEQ 1 goto sql_restore_error
goto xit


:no_postgres
echo Cannot locate the 'psql.exe' file!
echo PostgreSQL does not appear to be in the expected location or the current folder.
echo Please add the path to the PostgreSQL bin folder to the path or change to that folder to execute this script.
goto xit

:no_pgdump
echo PostgreSQL backup dump file not found in the expected location.
echo Please ensure the PostgreSQL bin folder contains the 'pgdump.exe' executable.
goto xit

:sql_backup_error
echo 'pg_dump.exe' reports error while backing-up the database!
goto xit

:sql_restore_error
echo 'psql.exe' reports error while restoring the database!
goto xit

:no_db_file_name
echo No source/output file name specified!
goto xit

:missing_db_file_param
echo Missing source/output file name parameter!
goto xit

:missing_db_path_param
echo Missing filepath parameter!
goto xit

:missing_path
echo Missing filepath parameter!
goto xit

:missing_db_restore_file
echo Restore file not found!
goto xit

:missing_db_backup_file
echo Backup output file not found!
goto xit

:no_mode
@echo No operation mode specified!
goto xit

:unrecognized_param
echo Unrecognized mode '%OP_MODE%'!
goto xit

:missing_mode_param
echo Missing operation mode parameter!

:xit
if not "%MCAFEE_MODE%x" == "TRUEx" goto xit2
@sadmin end-update

:xit2
if %DISPLAY_TIMEOUT%x == x goto xit3
timeout /t %DISPLAY_TIMEOUT%

:xit3
if not "%DEBUG_MODE%x" == "x" pause
rem restore any previous PG user or database values
set PGUSER=%PGUSEROLD%
set PGUSEROLD=
set PGPASSWORD=
set PGDATABASE=%PGDATABASEOLD%
set PGDATABASEOLD=
set PGPATH=
set DB_FILE=
set DB_FILE_PATH=
set FULL_PATH=
set FILE_MONTH=
set FILE_DAY=
set FILE_YEAR=
set FILE_DATE=
set OP_MODE=
set BACKUP_TGT=
set BACKUP_FLAG=
set VERBOSE_MODE=
set DISPLAY_TIMEOUT=
set DEBUG_MODE=
set MCAFEE_MODE=
set OVERWRITE=
set FLAG_CHAR=
