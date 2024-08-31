#include <stdio.h>

int main() {
    char buffer[1024];

    printf("Program started. Awaiting input...\n");

    if (fgets(buffer, sizeof(buffer), stdin)) {
        // Print the line to stdout
        printf("%s", buffer);
    }

    printf("Program finished.\n");

    return 0;
}