@ECHO OFF

PUSHD %~dp0

REM Update these lines if the currently installed version of Visual Studio is not 2017.
SET CMAKE="cmake"
SET CMAKE_GENERATOR="Visual Studio 16 2019"
SET CMAKE_BINARY_DIR=build_vs2019

MKDIR %CMAKE_BINARY_DIR% 2>NUL
PUSHD %CMAKE_BINARY_DIR%

%CMAKE% -G %CMAKE_GENERATOR% -Wno-dev "%~dp0"

cd D:\Repos\DX12-HybridPipeline
XCOPY .\DX12Lib\lib\dxcompiler\*.dll .\build_vs2019\bin\Release /e /I
XCOPY .\DX12Lib\lib\dxcompiler\*.dll .\build_vs2019\bin\Debug /e /I

IF ERRORLEVEL 1 (
    PAUSE
) ELSE (
    PAUSE
    REM START DirectX12-Demo.sln
)

POPD
POPD
