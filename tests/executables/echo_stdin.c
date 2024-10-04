#include <stdio.h>

int main(void) {
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
