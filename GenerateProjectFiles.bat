@ECHO OFF

SET CMAKE="cmake"
SET CMAKE_GENERATOR="Visual Studio 16 2019"
SET CMAKE_BINARY_DIR=build_vs2019

PUSHD %~dp0
MKDIR %CMAKE_BINARY_DIR% 2>NUL
PUSHD %CMAKE_BINARY_DIR%

%CMAKE% -G %CMAKE_GENERATOR% "%~dp0"
POPD

XCOPY .\DX12Lib\lib\dxcompiler\*.dll .\%CMAKE_BINARY_DIR%\bin\Release /e /I
XCOPY .\DX12Lib\lib\dxcompiler\*.dll .\%CMAKE_BINARY_DIR%\bin\Debug /e /I
XCOPY .\HybridRenderingPipeline\assimp\*.exp .\%CMAKE_BINARY_DIR%\bin\Release /e /I
XCOPY .\HybridRenderingPipeline\assimp\*.exp .\%CMAKE_BINARY_DIR%\bin\Debug /e /I
XCOPY .\HybridRenderingPipeline\assimp\*.dll .\%CMAKE_BINARY_DIR%\bin\Release /e /I
XCOPY .\HybridRenderingPipeline\assimp\*.dll .\%CMAKE_BINARY_DIR%\bin\Debug /e /I

PAUSE
POPD
