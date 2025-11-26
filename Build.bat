@echo off

set BaseFilename=MainWin32
set MainDir=%~dp0
set BuildDir=%MainDir%Build\

mkdir %BuildDir% >nul 2>&1

set ExeOutput=%BuildDir%%BaseFilename%.exe
set PdbOutput=%BuildDir%%BaseFilename%.pdb
set ObjOutput=%BuildDir%%BaseFilename%.obj

set FilesToCompile=%MainDir%%BaseFilename%.cpp
REM set FilesToCompile=%FilesToCompile%" "%MainDir%SomeNewFile.cpp

cl /nologo /Zi /Od %FilesToCompile% /Fe%ExeOutput% /Fd%PdbOutput% /Fo%ObjOutput%
