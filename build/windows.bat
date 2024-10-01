@echo off

mkdir lib\.build
mkdir bin

for %%f in (src\bs\*) do (
    cl /O2 /I include /c /Fo lib\.build\%%~nf.obj src\bs\%%~nf
)

cl /O2 /I include /Fe:bin\bs.exe src\bs.c lib\.build\*.obj pcre2-8.lib
cl /O2 /I include /LD /Fe:lib\libbs.dll lib\.build\*.obj pcre2-8.lib
lib /OUT:lib\libbs.lib lib\.build\*.obj
