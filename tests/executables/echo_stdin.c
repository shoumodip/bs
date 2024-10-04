#include <stdio.h>

#ifdef _WIN32
#    include <fcntl.h>
#    include <io.h>
#endif

int main(void) {
#ifdef _WIN32
    setmode(fileno(stdout), O_BINARY);
    setmode(fileno(stderr), O_BINARY);
    setmode(fileno(stdin), O_BINARY);
#endif

    puts("Echo stdin!");

    char buffer[1024];
    while (1) {
        char *bytes = fgets(buffer, sizeof(buffer), stdin);
        if (!bytes) {
            break;
        }

        printf("> %s", bytes);
    }

    puts("Done!");
    return 0;
}
