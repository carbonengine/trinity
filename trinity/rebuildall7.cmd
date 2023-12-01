call "C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat"
start devenv trinity.sln /rebuild release
start devenv trinity.sln /rebuild debug
start devenv trinity.sln /rebuild dev9
start devenv trinity.sln /rebuild dev10

