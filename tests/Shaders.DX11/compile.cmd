@echo off
del /q *.bin
for %%i in (*.psh) do (
    ..\fxc\fxc /nologo /Tps_5_0 /Fo %%~ni.ps.bin %%i
)
for %%i in (*.vsh) do (
    ..\fxc\fxc /nologo /Tvs_5_0 /Fo %%~ni.vs.bin %%i
)
for %%i in (*.csh) do (
    ..\fxc\fxc /nologo /Tcs_5_0 /Fo %%~ni.cs.bin %%i
)
for %%i in (*.bin) do (
    call ..\bin2h.cmd %%~ni.bin %%~ni.h
)
del /q *.bin
