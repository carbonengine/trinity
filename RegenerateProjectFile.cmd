@echo off
echo Checking out project and filters file
p4 edit TrinityAL_v141.vcxproj
p4 edit TrinityAL_v141.vcxproj.filters
echo Regenerating
..\..\..\..\..\..\shared_tools\python\27\python.exe ..\..\tools\ProjectFileGenerator\ProjectFileGenerator.py -i TrinityAL.ccpproj --toolset=v141 --outfile=TrinityAL_v141.vcxproj
pause
