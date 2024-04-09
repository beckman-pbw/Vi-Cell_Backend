
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\bin"
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\lib"
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\scripts"
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\share"
@sadmin trusted -i "C:\Program Files\PostgreSQL\10\pgAdmin 4"

@sadmin features enable pkg-ctrl

:chk_certs
rem update to add original BeckmanConnect certificate
@if not exist ".\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer" goto chk_cert2
@sadmin cert add -u ".\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer"
".\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer"
rem @copy /y /v ".\681c5a009d210131b0329ffbf95bc062c4f7380f1edf70e84d32ccad1b4fab5a.cer" "C:\Instrument\Tools\BeckmanConnect"

:chk_cert2
rem update to add original TeamViewer certificate
@if not exist ".\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer" goto chk_cert3
@sadmin cert add -u ".\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer"
".\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer"
rem @copy /y /v ".\5b7582701ef966127c7b828b225f082c08a4e610d2af4ed20d7ab49cd7261a6d.cer" "C:\Instrument\Tools\BeckmanConnect"

:chk_cert3
rem update to add new BeckmanConnect certificate
@if not exist ".\BeckmanConnect Certificate.cer" goto update_xit
@sadmin cert add -u ".\BeckmanConnect Certificate.cer"
".\BeckmanConnect Certificate.cer"
rem @copy /y /v ".\BeckmanConnect Certificate.cer" "C:\Instrument\Tools\BeckmanConnect"

:update_xit
