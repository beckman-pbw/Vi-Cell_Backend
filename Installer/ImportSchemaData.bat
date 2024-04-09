@echo off
set TEST_MODE=
set DISPLAY_TIMEOUT=
set OS_IMG_VER=
set SET_MCAFEE=
set DATA_SRC_PATH=
set IMG_SRC_PATH=
cls

:param_loop
if "%1x" == "x" goto get_data_path
if "%1x" == "-dx" goto set_debug
if "%1x" == "-Dx" goto set_debug
echo "unrecognized command line parameter...  ignored"
goto next_param

:set_debug
@echo on

:next_param
shift
goto param_loop

:get_data_path
echo.
echo "Locate the converted data file on the USB drive or other location.  File should be named 'DataImport.sql'."
set /p DATA_SRC_PATH="Enter the full path to the location of the converted data sql file, but do not add the file name! (e.g. D:\Data or 'x' to exit): "
if "%DATA_SRC_PATH%x" == "x" goto no_data_path
if "%DATA_SRC_PATH%x" == "xx" goto xit2
if "%DATA_SRC_PATH%x" == "Xx" goto xit2
if exist "%DATA_SRC_PATH%" goto get_image_path
echo.
echo Path not found!  Please re-enter or exit!
goto get_data_path

:no_data_path
echo.
echo Path cannot be empty!  Please re-enter or exit!
goto get_data_path

:get_image_path
echo.
echo "Locate the converted sample image folders on the USB drive or other location.  Folders will use a YYYY-MM-DD naming format"
set /p IMG_SRC_PATH="Enter the full path to the location of the converted sample images (e.g. D:\Data\Images or 'x' to exit): "
if "%IMG_SRC_PATH%x" == "x" goto no_data_path
if "%IMG_SRC_PATH%x" == "xx" goto xit2
if "%IMG_SRC_PATH%x" == "Xx" goto xit2
if exist "%IMG_SRC_PATH%" goto start_os_chk
echo.
echo Path not found!  Please re-enter or exit!
goto get_image_path

:no_data_path
echo.
echo Path cannot be empty!  Please re-enter or exit!
goto get_data_path

:start_os_chk
rem - all valid instrument OS versions SHOULD have whitelisting installed; missing components should generate a message
if exist \Cell-Health-OS-v*.txt goto os_chk
if exist \Instrument\Cell-Health-OS-v*.txt goto os_chk
@echo "Non-standard OS!"
goto mcafee_chk

:os_chk
rem - check for an instrument OS indicator; missing components should generate a message
if exist C:\Cell-Health-OS-v1.0*.txt goto os_1_0
if exist C:\Instrument\Cell-Health-OS-v1.0*.txt goto os_1_0

echo "Unrecognized OS indicator!
goto mcafee_chk

:os_1_0
set OS_IMG_VER=1.0

:mcafee_chk
if exist "C:\Program Files\McAfee\Solidcore\sadmin.exe" goto mcafee_found
@echo Application control component not found in OS configuration!
goto chk_db

:mccafee_found
@sadmin begin-update
if errorlevel == 0 goto mcafee_update_ok
@echo Error entering update mode! Non-admin environment!
set DISPLAY_TIMEOUT=300
goto xit

:mcafee_update_ok
set SET_MCAFEE=T

:chk_db
rem save any PostgreSQL environment user name
set PGUSEROLD=%PGUSER%
set PGUSER=
set PGPASSWORD=
@rem set PGUSER=postgres
@rem set PGPASSWORD=$3rgt$0P
set PGDATABASEOLD=%PGDATABASE%
set PGDATABASE=
set PGPATH=

rem  ASSUMES psql is installed into the expected location, or we are currently in the installed location;
rem  if not, add the default install location "C:\Program Files\PostgreSQL\10\bin" to the path
rem  or add it to the command execution through the path macro as in the line below
rem    set PGPATH=C:\Program Files\PostgreSQL\10\bin\
rem    "%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
rem        OR specify direcltly as below
rem    "C:\Program Files\PostgreSQL\10\bin\psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet

if exist "C:\Program Files\PostgreSQL\10\bin\psql.exe" goto pg_path_set
if exist ".\psql.exe" goto test_for_db
echo PostgreSQL does not appear to be installed in the expected location or the current folder.
echo Please ensure PostgreSQL is installed, and add the PostgreSQL bin folder to the path or
echo change to that folder to execute this script.
goto postgres_missing

:pg_path_set
set PGPATH=C:\Program Files\PostgreSQL\10\bin\

:test_for_db
rem The ViCell DB schema and PostgreSQL MUST be installed for this script!
set PGDATABASE=ViCellDB
set PGUSER=BCIViCellAdmin
@set PGPASSWORD=nimd@1leCiV1CB

rem  The '--command=\connect" statement tells psql to just connect to the named DB and exit.
rem  OTHERWISE, without the 'connect' command it will stay in the psql command window!
echo.
"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto no_db
echo.
echo "%PGDATABASE%" Database found! Using expected DB owner username.

:do_alter
rem  update the database schemas; the 'SchemaAlter.sql file is structured to update both the working and template DBs
set "DB_SCHEMA_FILE=%TGT_DRV%%BASE_PATH%\Instrument\Tools\SchemaAlter.sql"
rem - remove quotes from the filename if necessary
set DB_SCHEMA_FILE=%DB_SCHEMA_FILE:"=%
if not exist %DB_SCHEMA_FILE% goto DB_update_missing
@"%PGPATH%psql.exe" --quiet --file=%DB_SCHEMA_FILE% --username=%PGUSER%
if errorlevel 1 goto DB_update_failed

:chk_data_import
rem  Import the data into the working database; the 'DataImport.sql file is structured to import data into the ViCellDB schemas
if "%DATA_SRC_PATH%x" == "x" goto bad_data_path
if not exist "%DATA_SRC_PATH%" goto bad_data_path
set "DB_DATA_FILE=%DATA_SRC_PATH%\DataImport.sql"
if not exist %DB_DATA_FILE% goto DB_data_missing
rem  check for debugging test with no action...
if not "%TEST_MODE%x" == "x" goto chk_image_import
@"%PGPATH%psql.exe" --quiet --file=%DB_DATA_FILE% --username=%PGUSER%
if errorlevel 1 goto data_import_error
if exist %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\ResultBinaries rmdir /s /q %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\ResultBinaries

:chk_image_import
rem  Import the data into the working database; the 'DataImport.sql file is structured to import data into the ViCellDB schemas
if "%IMG_SRC_PATH%x" == "x" goto bad_image_path
if not exist "%IMG_SRC_PATH%" goto bad_image_path

if not exist %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\Images\202?-* goto remove_old_images
mkdir %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\TempImages
move %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\Images\202?-* %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\TempImages

:remove_old_images
rmdir /s /q %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\Images\*

:move_images
move %IMG_SRC_PATH%\* %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\Images
if not exist %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\TempImages\* goto ok_xit
move %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\TempImages\* %TGT_DRV%%BASE_PATH%\Instrument\ResultsData\Images
goto ok_xit

:DB_data_missing
set ERROR_MSG="DB import data file is missing"
goto xit

:DB_update_failed
set ERROR_MSG="DB schema update failed"
goto xit

:data_import_error
echo Error importing DB data!
goto xit

:no_db
echo Database "%PGDATABASE%" not found
goto xit

:postgres_missing
echo Postgres not found
goto xit

:bad_data_path
echo Data path missing or not valid!
goto xit

:ok_xit
echo Database updated successfully!

:xit
set PGUSER=%PGUSEROLD%
set PGUSEROLD=
set PGPASSWORD=
set PGDATABASE=%PGDATABASEOLD%
set PGDATABASEOLD=
set PGPATH=

rem Always remove the DB scripts if defined
set "DB_SCHEMA_FILE=%TGT_DRV%%BASE_PATH%\Instrument\Tools\SchemaAlter.sql"
rem - remove quotes from the filename if necessary
set DB_SCHEMA_FILE=%DB_SCHEMA_FILE:"=%
if exist %DB_SCHEMA_FILE% DEL /F /Q %DB_SCHEMA_FILE%
set DB_SCHEMA_FILE=
set "DB_DATA_FILE=%TGT_DRV%%BASE_PATH%\Instrument\Tools\DataImport.sql"
if not exist %DB_DATA_FILE% goto DB_data_missing
set DB_DATA_FILE=%DB_DATA_FILE:"=%
if exist %DB_DATA_FILE% DEL /F /Q %DB_DATA_FILE%
set DB_DATA_FILE=

if "%SET_MCAFEE%x" == "x" goto xit2
@sadmin end-update

:xit2
if "%DISPLAY_TIMEOUT%x" == "x" goto xit3
@timeout /t %DISPLAY_TIMEOUT%

:xit3
set TEST_MODE=
set DISPLAY_TIMEOUT=
set OS_IMG_VER=
set SET_MCAFEE=
set DATA_SRC_PATH=
set IMG_SRC_PATH=
