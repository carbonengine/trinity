@echo off
SET FXC="c:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\fxc.exe" 
del /q *.bin
for %%i in (*.psh) do (
    %FXC% /nologo /T ps_5_1 /Fo %%~ni.ps.bin %%i
)
for %%i in (*.vsh) do (
    %FXC% /nologo /T vs_5_1 /Fo %%~ni.vs.bin %%i
)
for %%i in (*.csh) do (
    %FXC% /nologo /T cs_5_1 /Fo %%~ni.cs.bin %%i
)
for %%i in (*.bin) do (
    call ..\bin2h.cmd %%~ni.bin %%~ni.h
)
del /q *.bin
