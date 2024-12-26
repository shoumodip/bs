# BS
[Scripting Language](https://shoumodip.github.io/bs)

## Building
```console
$ ./build/linux.sh # If on Linux
$ ./build/macos.sh # If on macOS
$ build\windows.bat # If on Windows (In developer console)
```

## Tests
### Linux
```console
$ cd tests
$ ../build/tests/linux.sh          # Run only once
$ ./rere.py replay test.list
```

### MacOS
```console
$ export DYLD_LIBRARY_PATH=$PWD/lib:$DYLD_LIBRARY_PATH
$ cd tests
$ ../build/tests/macos.sh          # Run only once
$ ./rere.py replay test.list
```

### Windows
```console
$ cd tests
$ ..\build\tests\windows.bat       # Run only once (In developer console)
$ python3 rere.py replay test.list
```
