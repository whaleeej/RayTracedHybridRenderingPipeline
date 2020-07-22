@ECHO OFF

SET CMAKE="cmake"
SET CMAKE_GENERATOR="Visual Studio 15 2017"
SET CMAKE_BINARY_DIR=build_vs2019

PUSHD %~dp0
MKDIR %CMAKE_BINARY_DIR% 2>NUL
PUSHD %CMAKE_BINARY_DIR%

%CMAKE% -A x64 -G %CMAKE_GENERATOR% "%~dp0"
POPD

XCOPY .\3rd\dxcompiler\*.dll .\%CMAKE_BINARY_DIR%\bin\Release /I
XCOPY .\3rd\dxcompiler\*.dll .\%CMAKE_BINARY_DIR%\bin\Debug /I
XCOPY .\3rd\assimp\*.exp .\%CMAKE_BINARY_DIR%\bin\Release /I
XCOPY .\3rd\assimp\*.exp .\%CMAKE_BINARY_DIR%\bin\Debug /I
XCOPY .\3rd\assimp\*.dll .\%CMAKE_BINARY_DIR%\bin\Release /I
XCOPY .\3rd\assimp\*.dll .\%CMAKE_BINARY_DIR%\bin\Debug /I
XCOPY .\3rd\renderdoc\*.dll .\%CMAKE_BINARY_DIR%\bin\Release /I
XCOPY .\3rd\renderdoc\*.dll .\%CMAKE_BINARY_DIR%\bin\Debug /I

PAUSE
POPD
