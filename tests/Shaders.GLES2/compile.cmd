@echo off
for %%i in (*.vsh) do (
    call ..\bin2h %%i %%~ni.vs.h
    call ..\bin2h %%i %%~ni.vs.patched.h
)
for %%i in (*.psh) do (
    call ..\bin2h %%i %%~ni.ps.h
    call ..\bin2h %%i %%~ni.ps.patched.h
)