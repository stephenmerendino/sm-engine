@echo off

SETLOCAL

set BaseFilename=MainWin32
set MainDir=%~dp0
set SrcDir=%~dp0Src\SM-Engine\
set BuildDir=%MainDir%Build\

mkdir %BuildDir% >nul 2>&1

set ExeOutput=%BuildDir%%BaseFilename%.exe
set PdbOutput=%BuildDir%%BaseFilename%.pdb
set ObjOutput=%BuildDir%%BaseFilename%.obj

set FilesToCompile=%SrcDir%%BaseFilename%.cpp
REM set FilesToCompile=%FilesToCompile%" "%MainDir%SomeNewFile.cpp

set Libs=user32.lib

cl /Zi /Od %FilesToCompile% %Libs% /Fe%ExeOutput% /Fd%PdbOutput% /Fo%ObjOutput%

ENDLOCAL
