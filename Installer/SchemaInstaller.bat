@echo off
set username=
set TEST_MODE=
set INSTALL_TGT=
cls

:chk_loop
if "%1x" == "x" goto start
if "%1x" == "-dx" goto set_debug
if "%1x" == "-Dx" goto set_debug
if "%1x" == "-tx" goto set_test
if "%1x" == "-Tx" goto set_test
if "%1x" == "-Ix" goto set_install_tgt
if "%1x" == "-ix" goto set_install_tgt
if "%1x" == "-ITx" goto set_template_tgt
if "%1x" == "-itx" goto set_template_tgt
if "%1x" == "-IWx" goto set_working_tgt
if "%1x" == "-iwx" goto set_working_tgt
if "%1x" == "-IBx" goto set_both_tgt
if "%1x" == "-ibx" goto set_both_tgt
if "%1x" == "-ICx" goto set_combined_tgt
if "%1x" == "-icx" goto set_combined_tgt
echo Unrecognized command line parameter...  ignored
goto next_param

:set_install_tgt
if not "%2x" == "x" goto chk_install_tgt
echo Missing install target parameter...  ignored
goto next_param

:chk_install_tgt
shift
if "%1x" == "TEMPLATE" goto set_template_tgt
if "%1x" == "template" goto set_template_tgt
if "%1x" == "WORKING" goto set_working_tgt
if "%1x" == "working" goto set_working_tgt
if "%1x" == "BOTH" goto set_both_tgt
if "%1x" == "both" goto set_both_tgt
if "%1x" == "COMBINED" goto set_combined_tgt
if "%1x" == "combined" goto set_combined_tgt
if "%1x" == "-tx" goto set_test
if "%1x" == "-Tx" goto set_test
if "%1x" == "-Ix" goto set_install
if "%1x" == "-ix" goto set_install
echo Unrecognized  or missing install target parameter...  ignored
goto chk_loop

:set_working_tgt
set INSTALL_TGT=WORKING
goto next_param

:set_template_tgt
set INSTALL_TGT=TEMPLATE
goto next_param

:set_both_tgt
set INSTALL_TGT=BOTH
goto next_param

:set_combined_tgt
set INSTALL_TGT=COMBINED
goto next_param

:set_test
set TEST_MODE=true

:set_debug
@echo on

:next_param
shift
goto chk_loop

:start
if "%INSTALL_TGT%x" == "x" set INSTALL_TGT=COMBINED

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
if errorlevel 1 goto no_db1
echo.
echo "%PGDATABASE%" Database found! Using expected DB owner username.
goto chk_tgt_install

:no_db1
rem echo.
rem echo Database "%PGDATABASE%" not found
set PGDATABASE=postgres
set PGUSER=postgres
rem use the original postgres password
@set PGPASSWORD=postgres

:test2
echo.
"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto no_db2
echo.
echo "%PGDATABASE%" Database found! Using default installation username.
goto chk_tgt_install

:no_db2
rem continue using the default postgres database but try the updated postgres password
@set PGPASSWORD=$3rgt$0P
echo.

:test3
echo.
"%PGPATH%psql.exe" --dbname=%PGDATABASE% --username=%PGUSER% --command=\connect --quiet
if errorlevel 1 goto no_db3
echo.
echo "%PGDATABASE%" Database found! Using default installation username.
goto chk_tgt_install

:no_db3
set PGUSER=
set PGPASSWORD=
echo.
rem echo Database "%PGDATABASE%" not found

rem Use the default PostgreSQL installer superuser name
rem comment out the two lines below if prompting for the username
rem set username=postgres
rem goto enter_pwd

rem USE THE FOLLOWING SECTION IF THE USERNAME USED DURING POSTGRESQL INSTALLATION IS UNKNOWN
echo When prompted, enter the username and password used during the PostgreSQL installation.
echo If this is an upgrade to the database schemas, the BCIViCellAdmin username is suggested.
echo.
@timeout /t 5
goto enter_user

:no_name
echo "Name cannot be blank!"

:enter_user
set PGUSER=
set username=
set PGPASSWORD=
echo.
set /P username=Enter the default username used during installation of PostgreSQL, or 'x' to exit:" 
if "%username%x" == "x" goto no_name
if "%username%x" == "xx" goto xit
if "%username%x" == "Xx" goto xit
echo.

:name_ok
set PGUSER=%username%

:enter_pwd
echo When prompted, enter the password associated with the entered username.
echo.

:chk_tgt_install
rem For all build conditions, the 'postgres' database is the default maintenance database created during PostgreSQL installation
rem ASSUMES psql is in the path; if not, add "\Program Files\PostgreSQL\10\bin" to the path or add it to the command esecution as in the line below
rem "C:\Program Files\PostgreSQL\10\bin\psql.exe" --quiet --file=.\RolesBackup.sql --dbname=postgres --username=%PGUSER%
set PGDATABASE=
if "%INSTALL_TGT%x" == "COMBINEDx" goto combined_bld

rem  Roles are always installed...
if not "%TEST_MODE%x" == "x" goto xit
if exist .\RolesBackup.sql goto install_roles
echo Missing roles script file!
goto err_xit

:install_roles
@"%PGPATH%psql.exe" --quiet --file=.\RolesBackup.sql --dbname=postgres --username=%PGUSER%
echo error level = %ERRORLEVEL%
if errorlevel 1 goto roles_db_error

:install_tgt_chk
if "%INSTALL_TGT%x" == "BOTHx" goto install_template
if "%INSTALL_TGT%x" == "TEMPLATEx" goto install_template
if "%INSTALL_TGT%x" == "WORKINGx" goto install_working
echo Unknown/unrecognized install target!
goto err_xit

:install_template
if exist .\ViCellDB_template.sql goto do_template_install
echo Missing template schema script file!
rem goto install_tgt_chk2
goto err_xit

:do_template_install
@"%PGPATH%psql.exe" --quiet --file=.\ViCellDB_template.sql --dbname=postgres --username=%PGUSER%
echo error level = %ERRORLEVEL%
if errorlevel 1 goto template_db_error

:install_tgt_chk2
if "%INSTALL_TGT%x" == "BOTHx" goto install_working
goto ok_xit

:install_working
if exist .\ViCellDB.sql goto do_template_install
echo Missing working schema script file!
goto err_xit

:do_working_install
@"%PGPATH%psql.exe" --quiet --file=.\ViCellDB.sql --dbname=postgres --username=%PGUSER%
echo error level = %ERRORLEVEL%
if errorlevel 1 goto primary_db_error
goto ok_xit

:combined_bld
if not "%TEST_MODE%x" == "x" goto xit
set PGDATABASE=
@"%PGPATH%psql.exe" --quiet --file=.\DBCreate.sql --dbname=postgres --username=%PGUSER%
if errorlevel 1 goto db_error
goto ok_xit

:roles_db_error
echo Error creating DB Users and roles!
goto err_xit

:template_db_error
echo Error creating template DB!
goto err_xit

:primary_db_error
echo Error creating primary DB!
goto err_xit

:db_error
echo Error creating DB elements!

:err_xit
echo Error during roles and schema installation!
@timeout /t 300
goto xit

:ok_xit
echo Database roles and schemas installed successfully!

:xit
set PGUSER=%PGUSEROLD%
set PGUSEROLD=
set PGPASSWORD=
set PGDATABASE=%PGDATABASEOLD%
set PGDATABASEOLD=
set PGPATH=
set username=
set TEST_MODE=
set INSTALL_TGT=

