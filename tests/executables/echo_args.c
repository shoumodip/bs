#include <stdio.h>

#ifdef _WIN32
#    include <fcntl.h>
#    include <io.h>
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
    setmode(fileno(stdout), O_BINARY);
    setmode(fileno(stderr), O_BINARY);
    setmode(fileno(stdin), O_BINARY);
#endif

    fprintf(stderr, "Number of arguments: %d\n", argc);
    fflush(stderr);

    for (int i = 1; i < argc; i++) {
        puts(argv[i]);
    }
}
