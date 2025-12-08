@echo off

SETLOCAL

set BaseFilename=Engine
set PlatformFilename=PlatformWin32

set MainDir=%~dp0
set SrcDir=%~dp0Src\
set LibsDir=%~dp0Libs\
set BuildDir=%MainDir%Build\

set CompilerFlags=/c /Zi /Od /nologo /std:c++20

set LibsPath=/LIBPATH:%MainDir%\Libs\
set Libs=user32.lib vulkan-1.lib dxcompiler.lib

set BaseFileToCompile=%SrcDir%SM\%BaseFilename%.cpp
set PlatformFileToCompile=%SrcDir%SM\%PlatformFilename%.cpp

set IncludeDirs=/I%SrcDir%

set BaseOutputName=SM-Engine
set PlatformOutputName=PlatformWin32

set BaseLibOutput=%BuildDir%%BaseOutputName%.lib
set BasePdbOutput=%BuildDir%%BaseOutputName%.pdb
set BaseObjOutput=%BuildDir%%BaseOutputName%.obj
set BaseOutputFiles=/Fd%BasePdbOutput% /Fo%BaseObjOutput%

set PlatformLibOutput=%BuildDir%%PlatformOutputName%.lib
set PlatformPdbOutput=%BuildDir%%PlatformOutputName%.pdb
set PlatformObjOutput=%BuildDir%%PlatformOutputName%.obj
set PlatformOutputFiles=/Fd%PlatformPdbOutput% /Fo%PlatformObjOutput%

set Win32OutputFiles=/Fd%PdbOutput% /Fo%ObjOutput%

mkdir %BuildDir% >nul 2>&1

REM Compile Engine.cpp
cl %CompilerFlags% %BaseFileToCompile% %IncludeDirs% %BaseOutputFiles%

REM Compile PlatformWin32.cpp
cl %CompilerFlags% %PlatformFileToCompile% %IncludeDirs% %PlatformOutputFiles%

REM Link them together into SM-Engine.lib
lib /nologo /out:%BaseLibOutput% %BaseObjOutput% %PlatformObjOutput% %Libs% %LibsPath% /IGNORE:4006

ENDLOCAL

EXIT /b %ERRORLEVEL%
