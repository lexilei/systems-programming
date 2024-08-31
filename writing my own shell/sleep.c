#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Sleeping for 1 second...\n");
    sleep(1);  // Sleep for 1 second
    printf("Finished sleeping. Hello!\n");
    return 0;
}