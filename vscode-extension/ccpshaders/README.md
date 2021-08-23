# ccpshaders README

This extension allows invoking shader compiler for effect files.

See https://wiki.ccpgames.com/display/PAD/Shader+workspace for more info.

## Features

* Compile opened effect file (press `Ctrl+F7`)
* Compile a file from explorer tree: right-click a file and choose Compile option
* Open FXInfo tool for an effect file from explorer tree: right-click a file and choose FX Info option

Tasks:
* `shadercompiler` - builds shaders for the entire workspace

## Requirements

Extension requires intalled p4 executable in the path and p4config file in the branch. 

The extension uses shader compiler in `<branch>/carbon/tools/ShaderCompiler` directory that needs to be synced.

On MacOS, XCode and Command line tools for XCode need to be installed. 

On Windows, Windows SDK is required. For Metal compilation on Windows, Metal Developer Tools need to be installed (in the default location).