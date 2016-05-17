@echo off
for %%i in (*.psh) do (
    ..\fxc\fxc /nologo /Tps_3_0 /Fo %%~ni.ps.bin %%i
    ..\fxc\fxc /nologo /Tps_3_0 /Fo %%~ni.ps.patched.bin %%i
)
for %%i in (*.vsh) do (
    ..\fxc\fxc /nologo /Tvs_3_0 /Fo %%~ni.vs.bin %%i
    ..\fxc\fxc /nologo /Tvs_3_0 /Fo %%~ni.vs.patched.bin %%i
)
for %%i in (*.bin) do (
    call ..\bin2h %%~ni.bin %%~ni.h
)
del /q *.bin
