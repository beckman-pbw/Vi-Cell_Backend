@echo off
set OPCUA_PORT=
set OPCUA_EXE=
set PORT_DBG_MODE=
set QUIET_MODE=T
set DO_CLEAR=
cls

:param_loop
if "%1x" == "x" goto start
if "%1x" == "-dx" goto set_debug
if "%1x" == "-Dx" goto set_debug
if "%1x" == "-px" goto set_port
if "%1x" == "-Px" goto set_port
if "%1x" == "-ex" goto set_exe
if "%1x" == "-Ex" goto set_exe
if "%1x" == "-qx" goto set_quiet
if "%1x" == "-Qx" goto set_quiet
if "%1x" == "-q-x" goto clear_quiet
if "%1x" == "-Q-x" goto clear_quiet
if "%1x" == "-cx" goto set_clear
if "%1x" == "-Cx" goto set_clear
if "%1x" == "-sx" goto show_rules
if "%1x" == "-Sx" goto show_rules
echo "unrecognized command line parameter...  ignored"
goto next_param

:set_debug
@echo on
set PORT_DBG_MODE=T
goto next_param

:set_quiet
set QUIET_MODE=T
goto next_param

:clear_quiet
set QUIET_MODE=
goto next_param

:set_clear
set DO_CLEAR=T
goto next_param

:set_port
if "%2x" == "x" goto missing_port
shift
set OPCUA_PORT=%1
goto next_param

:set_exe
if "%2x" == "x" goto missing_name
shift
set OPCUA_EXE=%1

:next_param
shift
goto param_loop

:start
if not "%DO_CLEAR%x" == "x" goto clear_rules
if "%OPCUA_PORT%x" == "x" goto set_default_port
if "%OPCUA_PORT%x" == "?x" goto get_port

:get_port
echo.
set OPCUA_PORT=
@set /p OPCUA_PORT="Enter the port number associated with the OpcUa server, or 'x' to exit without setting.  If blank the default will be used (62641): "
if "%OPCUA_PORT%x" == "x" goto set_default_port
if "%OPCUA_PORT%x" == "xx" goto xit
if "%OPCUA_PORT%x" == "?x" goto bad_port
echo.
goto chk_exe

:bad_port
echo unrecognized/illegal response!
goto get_port

:no_port
set OPCUA_PORT=
goto chk_exe

:set_default_port
set OPCUA_PORT=62641

:chk_exe
if "%OPCUA_EXE%x" == "x" goto set_firewall
if "%OPCUA_EXE%x" == "?x" goto get_exe

:get_exe
echo.
set OPCUA_EXE=
@set /p OPCUA_EXE="Enter the OpcUa server name with the full path, or 'x' to exit without setting.  If blank, no executable entry wil be added: "
if "%OPCUA_EXE%x" == "x" goto set_firewall
if "%OPCUA_EXE%x" == "xx" goto xit
if "%OPCUA_EXE%x" == "?x" goto bad_exe
echo.
goto set_firewall

:bad_exe
echo unrecognized/illegal response!
goto get_port

:set_default_exe
set OPCUA_EXE=\Instrument\OpcUaServer\ViCellOpcUaServer.exe
goto set_firewall

:clear_rules
set OPCUA_PORT=
set OPCUA_EXE=

:set_firewall
rem ####################################################################################
rem                       configure the appropriate firewall rules
rem ####################################################################################
rem  Remove all existing old OpcUa Server rules before adding the updated executable and/or port rules
if not "%PORT_DBG_MODE%x" == "x" echo Clearing all known old rules...
if not "%QUIET_MODE%x" == "x" goto do_clear_quiet
netsh advfirewall firewall delete rule name="OpcUa Server"
netsh advfirewall firewall delete rule name="OpcUa Server Port"
netsh advfirewall firewall delete rule name="OpcUa Server Exe"
goto set_port_rules

:do_clear_quiet
netsh advfirewall firewall delete rule name="OpcUa Server" >> nul
netsh advfirewall firewall delete rule name="OpcUa Server Port" >> nul
netsh advfirewall firewall delete rule name="OpcUa Server Exe" >> nul

:set_port_rules
rem  If using ports, open the firewall ports required by OpcUa
if "%OPCUA_PORT%x" == "x" goto set_exe_rules
if not "%QUIET_MODE%x" == "x" goto set_port_rules_quiet
netsh advfirewall firewall add rule name="OpcUa Server Port" dir=in action=allow enable=yes protocol=tcp interfacetype=lan localport=%OPCUA_PORT% profile=domain,private
netsh advfirewall firewall add rule name="OpcUa Server Port" dir=out action=allow enable=yes protocol=tcp interfacetype=lan localport=%OPCUA_PORT% profile=domain,private

:set_port_rules_quiet
netsh advfirewall firewall add rule name="OpcUa Server Port" dir=in action=allow enable=yes protocol=tcp interfacetype=lan localport=%OPCUA_PORT% profile=domain,private >> nul
netsh advfirewall firewall add rule name="OpcUa Server Port" dir=out action=allow enable=yes protocol=tcp interfacetype=lan localport=%OPCUA_PORT% profile=domain,private >> nul

:set_exe_rules
rem  If using named executables, configure the firewall to allow the OpcUa server exe file through the firewall
if "%OPCUA_EXE%x" == "x" goto show_set_rules
if not "%QUIET_MODE%x" == "x" goto set_exe_rules_quiet
netsh advfirewall firewall add rule name="OpcUa Server Exe" dir=in program=%OPCUA_EXE% action=allow enable=yes profile=domain,private
netsh advfirewall firewall add rule name="OpcUa Server Exe" dir=out program=%OPCUA_EXE% action=allow enable=yes profile=domain,private

:set_exe_rules_quiet
netsh advfirewall firewall add rule name="OpcUa Server Exe" dir=in program=%OPCUA_EXE% action=allow enable=yes profile=domain,private >> nul
netsh advfirewall firewall add rule name="OpcUa Server Exe" dir=out program=%OPCUA_EXE% action=allow enable=yes profile=domain,private >> nul

:show_set_rules
if not "%QUIET_MODE%%x" == "x" goto xit

:show_rules
netsh advfirewall firewall show rule name="OpcUa Server"
netsh advfirewall firewall show rule name="OpcUa Server Port"
netsh advfirewall firewall show rule name="OpcUa Server Exe"
goto xit

:missing_port
echo Missing port number parameter to the '-p' option flag.
goto xit

:missing_name
echo Missing name parameter to the '-e' option flag.

:xit
if not "%PORT_DBG_MODE%x" == "x" pause
set OPCUA_PORT=
set OPCUA_EXE=
set PORT_DBG_MODE=
set QUIET_MODE=
set DO_CLEAR=
