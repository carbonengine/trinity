@echo off
SET FXC="c:\VulkanSDK\1.1.97.0\Bin\glslangValidator.exe" 
del /q *.bin
for %%i in (*.psh) do (
    %FXC% -S frag -V -D -e main --sub 32 --ssb 32 --suavb 224 --stb 416 -o %%~ni.ps.bin %%i
)
for %%i in (*.vsh) do (
    %FXC% -S vert -V -D -e main --sub 0 --ssb 0 --suavb 192 --stb 384 -o %%~ni.vs.bin %%i
)
for %%i in (*.csh) do (
    %FXC% -S comp -V -D -e main --sub 64 --ssb 64 --suavb 256 --stb 448 -o %%~ni.cs.bin %%i
)
for %%i in (*.bin) do (
    call ..\bin2h.cmd %%~ni.bin %%~ni.h
)
del /q *.bin
