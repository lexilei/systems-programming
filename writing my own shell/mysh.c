// THIS CODE IS OUR OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR
// OR CODE WRITTEN BY OTHER STUDENTS OUTSIDE OF OUR TEAM. Lex Lei
/*
basic idea is: print 

standard format for catching errors.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include "tokens.h"
//structure: read input, the pass shell name into function

int main(int argc, char **argv){
    char* prompt;
    if (argc == 1){                            
        commandProcessing("mysh");
    }else if (argc == 2){                    
        if (strcmp(argv[1], "-") == 0){
            commandProcessing("");
        }else {                                
            commandProcessing(argv[1]);
            return 1;
        }
    }
    fprintf(stderr, "Error: Usage: %s [prompt]\n", "mysh");
    exit(1);
}

//so far I'm able to read and start my own terminal
void commandProcessing(char* prompt){
    char* address;
    while(1){
        printf("%s: ", prompt);
        address=malloc(1024);   
        fgets(address, 1024, stdin); //writing input in address
    }

}

