@echo off

set CFLAGS=/Iinclude /O2
set LIBS=/link /DEFAULTLIB:libcmt.lib

rmdir /S /Q bin lib
mkdir bin lib\.build

for %%f in (src\bs\*) do (
    cl /c /Fo:lib\.build\%%~nf.obj %CFLAGS% src\bs\%%~nf.c
)

cl %CFLAGS% /Fe:bin\bs.exe src\bs.c lib\.build\*.obj %LIBS%
cl %CFLAGS% /LD /Fe:lib\bs.dll lib\.build\*.obj %LIBS%
lib /OUT:lib\bs.lib lib\.build\*.obj
