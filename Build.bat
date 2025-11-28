@echo off

SETLOCAL

set BaseFilename=MainWin32
set MainDir=%~dp0
set SrcDir=%~dp0Src\
set BuildDir=%MainDir%Build\

set CompilerFlags=/Zi /Od /nologo /std:c++20

set FilesToCompile=%SrcDir%SM\%BaseFilename%.cpp
REM set FilesToCompile=%FilesToCompile%" "%MainDir%SomeNewFile.cpp

set IncludeDirs=/I%SrcDir%

set Libs=user32.lib

set ExeOutput=%BuildDir%%BaseFilename%.exe
set PdbOutput=%BuildDir%%BaseFilename%.pdb
set ObjOutput=%BuildDir%%BaseFilename%.obj
set OutputFiles=/Fe%ExeOutput% /Fd%PdbOutput% /Fo%ObjOutput%

set LinkerFlags=-subsystem:windows

mkdir %BuildDir% >nul 2>&1
cl %CompilerFlags% %FilesToCompile% %IncludeDirs% %Libs% %OutputFiles% /link %LinkerFlags%

ENDLOCAL
