cl /Foexecutables\ /Fe:executables\echo_args.exe executables\echo_args.c
cl /Foexecutables\ /Fe:executables\echo_stdin.exe executables\echo_stdin.c
cl /I..\include /LD /Foexecutables\ /Fe:executables\addsub.dll executables\addsub.c ..\lib\bs.lib
cl /I..\include /LD /Foexecutables\ /Fe:executables\invalid.dll executables\invalid.c ..\lib\bs.lib
