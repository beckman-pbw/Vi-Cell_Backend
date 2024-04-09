
@if exist .\WhitelistingUpdate.bat del /F /Q .\WhitelistingUpdate.bat

:cert_chk1
rem cleanup any leftover cert files
@if not exist ".\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer" goto cert_chk2
@del /F /Q ".\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer"

:cert_chk2
@if not exist ".\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer" goto cert_chk3
@del /F /Q ".\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer"

:cert_chk3
@if not exist ".\BeckmanConnect Certificate.cer" goto cleanup_xit
@del /F /Q ".\BeckmanConnect Certificate.cer"

:cleanup_xit
