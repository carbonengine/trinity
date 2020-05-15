@echo off
echo Checking out project and filters file
p4 edit TrinityALTest_v141.vcxproj
p4 edit TrinityALTest_v141.vcxproj.filters
echo Regenerating
..\..\..\..\..\..\shared_tools\python\27\python.exe ..\..\tools\ProjectFileGenerator\ProjectFileGenerator.py -i TrinityALTest.ccpproj --toolset=v141 --outfile=TrinityALTest_v141.vcxproj
pause
