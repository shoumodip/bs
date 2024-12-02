#include <stdio.h>

int main(int argc, char **argv) {
    fprintf(stderr, "Number of arguments: %d\n", argc);
    fflush(stderr);

    for (int i = 1; i < argc; i++) {
        puts(argv[i]);
    }
}
