#include <stdio.h>
#include <string.h>

int main() {
    char input[100]; // Assuming a maximum input length of 100 characters

    printf("Enter a line of text (Ctrl-D to exit):\n");

    while (fgets(input, sizeof(input), stdin) != NULL) {
        // Remove the newline character at the end of the input
        input[strcspn(input, "\n")] = '\0';

        printf("You entered: %s\n", input);

        printf("Enter another line of text (Ctrl-D to exit):\n");
    }

    printf("End of input reached. Exiting.\n");
    return 0;
}
