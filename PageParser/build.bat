@echo off 

IF NOT EXIST "build" (
    mkdir build
)

CALL "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" x64

PUSHD "build"
    REM cl /EHsc /Zi /Wall /wd4091 /Fe:PageParser.exe ..\PageParser\main.c ..\PageParser\tex.c ..\PageParser\html.c
    cl /EHsc /Zi /Fe:page_parser.exe ..\PageParser\main.c ..\PageParser\page_parser.c ..\PageParser\tex.c ..\PageParser\html.c
    cl /EHsc /Zi /Fe:tests.exe ..\PageParser\tests.c ..\PageParser\page_parser.c ..\PageParser\tex.c ..\PageParser\html.c
POPD