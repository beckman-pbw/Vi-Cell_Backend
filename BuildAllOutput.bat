:@echo off
if "%1." == "." goto no_tgt
if "%2." == "." goto no_bld

echo.
if not exist .\BuildOutput mkdir .\BuildOutput
if not exist .\BuildOutput\%1 mkdir .\BuildOutput\%1
if not exist .\BuildOutput\%1\%2 mkdir .\BuildOutput\%1\%2

copy /Y ".\BASLER\Assemblies\Basler.Pylon\x64\*.dll" .\BuildOutput\%1\%2

copy /Y "%OPENCV_ROOT%bin\opencv_core249.dll" .\BuildOutput\%1\%2
copy /Y "%OPENCV_ROOT%bin\opencv_highgui249.dll" .\BuildOutput\%1\%2

copy /Y ".\FTDI\amd64\*.dll" .\BuildOutput\%1\%2
if exist .\BuildOutput\%1\%2\ftd2xx.dll del .\BuildOutput\%1\%2\ftd2xx.dll
rename   .\BuildOutput\%1\%2\ftd2xx64.dll ftd2xx.dll

if not exist ".\HawkeyeCore\%1\%2\HawkeyeCore.dll" goto no_core
copy /Y ".\HawkeyeCore\%1\%2\HawkeyeCore.dll" .\BuildOutput\%1\%2

if not exist ".\HawkeyeCore\%1\%2\HawkeyeCore.pdb" goto no_pdb1
copy /Y ".\HawkeyeCore\%1\%2\HawkeyeCore.pdb" .\BuildOutput\%1\%2

if not exist ".\Test\CameraTest\%1\%2\CameraTest.exe" goto no_cam
copy /Y ".\Test\CameraTest\%1\%2\CameraTest.exe" .\BuildOutput\%1\%2

if not exist ".\Test\CameraTest\%1\%2\CameraTest" goto no_pdb2
copy /Y ".\Test\CameraTest\%1\%2\CameraTest.pdb" .\BuildOutput\%1\%2

echo.
echo BUILD SUCCESS!
goto done_xit

:no_tgt
echo.
echo BUILD ERROR: no platform configuration target specified (Win32 | x64)
goto done_xit

:no_bld
echo.
echo BUILD ERROR: no build configuration target specified (Debug | Release)
goto done_xit

:no_core
echo.
echo BUILD ERROR: core output dll file not found!
goto done_xit

:no_pdb1
echo.
echo BUILD ERROR: core output pdb file not found!
goto done_xit

:no_cam
echo.
echo BUILD ERROR: camera output exe file not found!
goto done_xit

:no_pdb2
echo.
echo BUILD ERROR: camera output pdb file not found!

:done_xit
echo.
@echo on
