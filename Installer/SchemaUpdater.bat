@echo off
set TEST_MODE=
cls

:chk_loop
if "%1x" == "x" goto start
if "%1x" == "-dx" goto set_debug
if "%1x" == "-Dx" goto set_debug
if "%1x" == "-tx" goto set_test
if "%1x" == "-Tx" goto set_test
echo "unrecognized command line parameter...  ignored"
goto next_param

:set_test
set TEST_MODE=true

:set_debug
@echo on

:next_param
shift
goto chk_loop

:start
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
if exist ".\psql.exe" goto pg_path_set_local
echo PostgreSQL does not appear to be installed in the expected location or the current folder.
echo Please ensure PostgreSQL is installed, and add the PostgreSQL bin folder to the path or
echo change to that folder to execute this script.
goto xit

:pg_path_set
set PGPATH=C:\Program Files\PostgreSQL\10\bin\
goto test_primary

:pg_path_set_local
set PGPATH=.\

:test_primary
rem test to see if the schema has been installed using the primary working database schema; if installed the BCIViCellAdmin user MUST be present
set PGDATABASE=ViCellDB
set PGUSER=BCIViCellAdmin
@set PGPASSWORD=nimd@1leCiV1CB

rem  The '--command=\connect" statement tells psql to just connect to the named DB and exit.
rem  OTHERWISE, without the 'connect' command it will stay in the psql command window!
echo.
"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto no_primary
echo.
echo "%PGDATABASE%" Database found! Using expected DB owner username.

rem  update the database schemas; the 'SchemaAlter.sql file is structured to update both schemas
rem  check for debugging test with no action...
if not "%TEST_MODE%x" == "x" goto xit
@"%PGPATH%psql.exe" --quiet --file=.\SchemaAlter.sql --username=%PGUSER%
if errorlevel 1 goto update_error
goto ok_xit

:no_primary
echo Primary database "%PGDATABASE%" not found
goto error_xit

:update_error
echo Error updating DB elements!

:err_xit
echo Error during schema update!
@timeout /t 300
goto xit

:ok_xit
echo Database schemas updated successfully!

:xit
set PGUSER=%PGUSEROLD%
set PGUSEROLD=
set PGPASSWORD=
set PGDATABASE=%PGDATABASEOLD%
set PGDATABASEOLD=
set PGPATH=
set TEST_MODE=

