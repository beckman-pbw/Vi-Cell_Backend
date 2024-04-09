mkdir "L&T_Package"

cd "L&T_Package"

xcopy /Y /E /Q /I ..\ScoutTestCS .\ScoutTestCS

mkdir .\include
copy /Y ..\..\HawkeyeCore\API .\include

xcopy /Y /E /Q /I ..\..\BuildOutput\x64\Release .\

copy ..\..\Documentation\API*.doc .\

del *.log*
del *.exe*
del *.pdb*

del BusinessObjectTest.*
del CameraTest.*
del ControllerBoardTest.*
del ImagePostProcessing.*
del MotorControl.exe
del MotorControlTest.*
del SyringeTest.*
del UserTest.*


pause
