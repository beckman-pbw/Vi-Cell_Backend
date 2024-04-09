@echo off

set importDir=%USERPROFILE%\AppData\Local\Temp\
set targetDir=%~dp0..\
set zipfile=%1

:IS_VICELL_BLU_RUNNING
tasklist /fi "IMAGENAME eq ViCellBLU_UI.exe" | find ":" > nul
if not errorlevel 1 goto GET_FILENAME
	echo Cannot import data while CydemVT instrument software is running.
	goto FAILED_XIT

:GET_FILENAME
if not "%zipfile%" == "" goto HAS_PARAMETER
	set /p zipfile="CydemVT zip file: "	
	if not "%zipfile%" == "" goto HAS_PARAMETER	
		goto FAILED_XIT

:HAS_PARAMETER
echo Input zip file to import %zipfile%
if exist "%zipfile%" goto EXTRACT
	echo %zipfile% not found... 
	set zipfile=
	goto GET_FILENAME
	echo.
	
:CHECK_FOR_7ZIP
if exist "C:/Program Files/7-Zip/7z.exe" goto EXTRACT
	echo "C:/Program Files/7-Zip/7z.exe" not found... 
	goto FAILED_XIT
	echo.

:EXTRACT
echo Extracting the data, this may take a while Please Wait .....
rem import to ???  user specified...
"C:/Program Files/7-Zip/7z.exe" x "%zipfile%" -o"%importDir%" -r -aoa
if errorlevel 255 goto USER_STOPPED_THE_PROCESS
if errorlevel 8 goto NOT_ENOUGH_MEMORY
if errorlevel 7 goto COMMAND_LINE_ERROR
if errorlevel 2 goto FATAL_ERROR
if errorlevel 1 goto OK_WARNINGS
echo.

rem These checks are for importing v1.2 data.
:CHECK_FOR_AUDIT_LOG
set al=
for /R "%importDir%Instrument\Export\" %%G in (*.log) do if "%%~nxG"=="Audit.log" set al=%%G
if defined al goto CHECK_FOR_ERROR_LOG
	echo Missing Audit.log file, assuming importing v1.3 or later data...

:CHECK_FOR_ERROR_LOG
if defined al (copy %al% %targetDir%Logs)

set el=
for /R "%importDir%Instrument\Export\" %%G in (*.log) do if "%%~nxG"=="Error.log" set el=%%G
if defined el goto CHECK_FOR_SAMPLE_LOG
	echo Missing Error.log file, assuming importing v1.3 or later data...

:CHECK_FOR_SAMPLE_LOG
if defined el (copy %el% %targetDir%Logs)

set sl=
for /R "%importDir%Instrument\Export\" %%G in (*.log) do if "%%~nxG"=="Sample.log" set sl=%%G
if defined sl goto CHECK_FOR_METADATA_FILE
	echo Missing Sample.log file, assuming importing v1.3 or later data...

:CHECK_FOR_METADATA_FILE
if defined sl (copy %sl% %targetDir%Logs)

set mf=
for /R "%importDir%Instrument\Export\" %%G in (*.exml) do if "%%~nxG"=="HawkeyeMetadata.exml" set mf=%%G
if defined mf goto CHECK_FOR_CONFIG_FOLDER
	echo Missing HawkeyeMetadata.exml file, assuming importing v1.3 or later data...

:CHECK_FOR_CONFIG_FOLDER
if defined mf (copy "%mf%" "%importDir%Instrument\ResultsData")

if exist "%importDir%Instrument\Config" goto CHECK_FOR_RESULTS_DATA_IMAGES
	echo Missing Config folder in imported data
	if not defined mf goto CHECK_FOR_RESULTS_DATA_IMAGES
	goto FAILED_XIT

:CHECK_FOR_RESULTS_DATA_IMAGES
if exist "%importDir%Instrument\ResultsData\Images" goto CHECK_FOR_RESULTS_DATA_BINARIES
	echo Missing Images data folder
	goto FAILED_XIT

:CHECK_FOR_RESULTS_DATA_BINARIES
if exist "%importDir%Instrument\ResultsData\ResultBinaries" goto IMPORT
	echo Missing ResultBinaries data folder
	goto FAILED_XIT

:IMPORT
echo Importing the data, this may take a while...

Set Filename=%~n1
Set InstrumentSerialNumber=%Filename:*_SN=%
c:\instrument\software\DataImporter -o -s %InstrumentSerialNumber%

goto REMOVE_EXTRACTED_DATA_AND_XIT

:USER_STOPPED_THE_PROCESS
echo user stopped the unzip process
goto REMOVE_EXTRACTED_DATA_AND_XIT

:NOT_ENOUGH_MEMORY
echo Not enough memory
goto REMOVE_EXTRACTED_DATA_AND_XIT

:COMMAND_LINE_ERROR
echo Command line error
goto REMOVE_EXTRACTED_DATA_AND_XIT

:FATAL_ERROR
echo Fatal error failed to extract
goto REMOVE_EXTRACTED_DATA_AND_XIT

:OK_WARNINGS
echo warnings, exiting file extract
goto REMOVE_EXTRACTED_DATA_AND_XIT

:FAILED_XIT
echo Failed to import the data
goto REMOVE_EXTRACTED_DATA_AND_XIT

:REMOVE_EXTRACTED_DATA_AND_XIT
if exist "%importDir%Instrument" rmdir /s /q "%importDir%Instrument" >> import_data.log

:XIT
PAUSE