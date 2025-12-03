@echo off

SETLOCAL

set BaseFilename=MainWin32
set MainDir=%~dp0
set SrcDir=%~dp0Src\
set LibsDir=%~dp0Libs\
set BuildDir=%MainDir%Build\

set CompilerFlags=/Zi /Od /nologo /std:c++20

set FilesToCompile=%SrcDir%SM\%BaseFilename%.cpp
REM set FilesToCompile=%FilesToCompile%" "%MainDir%SomeNewFile.cpp

set IncludeDirs=/I%SrcDir%
set LibsPath=/LIBPATH:%LibsDir%

set Libs=user32.lib vulkan-1.lib dxcompiler.lib

set OutputName=SM-Engine
set ExeOutput=%BuildDir%%OutputName%.exe
set PdbOutput=%BuildDir%%OutputName%.pdb
set ObjOutput=%BuildDir%%OutputName%.obj
set OutputFiles=/Fe%ExeOutput% /Fd%PdbOutput% /Fo%ObjOutput%

set LinkerFlags=-subsystem:windows

mkdir %BuildDir% >nul 2>&1
@echo on
cl %CompilerFlags% %FilesToCompile% %IncludeDirs% %Libs% %OutputFiles% /link %LinkerFlags% %LibsPath%
@echo off

ENDLOCAL

EXIT /b %ERRORLEVEL%
