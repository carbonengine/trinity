#!/bin/sh

echo "generating XCode project file..."

cd `dirname $0`
/usr/local/bin/python ../../tools/ProjectFileGenerator/ProjectFileGenerator.py -x -i TrinityAL.ccpproj


