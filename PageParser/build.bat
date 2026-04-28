@echo off 

IF NOT EXIST "build" (
    mkdir build
)

CALL "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" x64

PUSHD "build"
    REM cl /EHsc /Zi /Wall /wd4091 /wd4057 /Fe:PageParser.exe ..\PageParser\main.c
    cl /EHsc /Zi /Fe:PageParser.exe ..\PageParser\main.c
    REM cl /EHsc /Zi /Wall /wd4091 /wd4057 /Fe:tests.exe ..\PageParser\tests.c
    cl /EHsc /Zi /Fe:tests.exe ..\PageParser\tests.c
POPD