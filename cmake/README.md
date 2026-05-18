# platform-cmake

This folder contains a collection of useful CMake modules for working with EVE's C++ codebase outside the
Perforce monolith.

## Usage

Simply use [CMake's include](https://cmake.org/cmake/help/latest/command/include.html) command to include the things
you would like to use. 

## Available CMake modules

Most of these are optional, except for `CcpMinimumTargetPlatform.cmake` which is required in order to guarantee that your C++ code runs our supported platforms.

- CcpBuildConfigurations.cmake: defines supported build configurations with all relevant compiler switches; also known as [flavors](https://wiki.ccpgames.com/pages/viewpage.action?pageId=167285354#Glossary-Flavor)
- CcpMinimumTargetPlatform.cmake: configures key CMake variables to be correct for our target platforms
- CcpPacIdentifiers.cmake: provides variables containing our platform, architecture and compiler identifiers
- PyTestDiscoverTests.cmake: allows running pytest tests through ctest
