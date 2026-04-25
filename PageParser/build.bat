@echo off 

IF NOT EXIST "build" (
    mkdir build
)

CALL "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" x64

PUSHD "build"
    cl /EHsc /Zi /Fe:PageParser.exe ..\PageParser\main.c
POPD