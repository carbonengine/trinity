@echo off
SET FXC="c:\VulkanSDK\1.1.97.0\Bin\glslangValidator.exe" 
del /q *.bin
for %%i in (*.psh) do (
    %FXC% -S frag -V -D -e main -o %%~ni.ps.bin %%i
    %FXC% -S frag -V -D -e main -DPATCHED_SHADER=1 -o %%~ni.ps.patched.bin %%i
)
for %%i in (*.vsh) do (
    %FXC% -S vert -V -D -e main -o %%~ni.vs.bin %%i
    %FXC% -S vert -V -D -e main -DPATCHED_SHADER=1 -o %%~ni.vs.patched.bin %%i
)
for %%i in (*.csh) do (
    %FXC% -S comp -V -D -e main -o %%~ni.cs.bin %%i
)
for %%i in (*.bin) do (
    call ..\bin2h.cmd %%~ni.bin %%~ni.h
)
del /q *.bin
