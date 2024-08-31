// THIS CODE IS OUR OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR
// OR CODE WRITTEN BY OTHER STUDENTS OUTSIDE OF OUR TEAM. lex lei
//
//this submition is temporary at 8:04 pm, I'll replace later

/* assignment breakdown
Utility Functions:
   - `pushForward`: Sets a foreground process as "waited for" after it has finished execution.
   - `keepWaiting`: Checks if any foreground processes are still running and need to be waited for.
   - `removeOperators`: Removes input/output redirection operators ("<", ">", ">>") from the command tokens.

Main Loop:continuously prompts the user for input and processes the entered commands until the user exits by typing "exit" or using Ctrl-D.

Input/Output Redirection: handles input redirection ("<") and output redirection (">" and ">>") for each command within a pipeline.

Prompting: The code displays a prompt to the user before accepting each command. The prompt type and string are determined by the `Options` structure.

Error Handling: includes error handling for various scenarios, such as invalid commands, missing filenames for redirection, and file-related errors.

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

typedef struct {              
    int promptType;           
    char* promptString;
}Prompt;

typedef struct {
    bool append;
    char *input_file_name, *output_file_name, **cmd;
}Cmd;

typedef struct {
    bool foreground_process; 
    Cmd *cmd_list;
    int num_commands;
}CmdSet;

// Function to parse command-line arguments and determine the shell prompt settings.
Prompt parseArgs(int argc, char *argv[]);

// Function to remove input/output redirection operators ("<", ">", ">>") from a list of tokens.
char** removeOperators(char **tokens, int operator_loc);

// Function to check for errors in the command line input.
void check(char *cmdargv);

// Function to push a foreground process into the "waited for" state.
void pushForward(int *foreground_processes, int pid, int num_cmds);

// Function to check if any foreground processes are still running and need to be waited for.
bool waitHelper(int *foreground_processes, int num_cmds);

// Function to parse and process commands from the command line input.
CmdSet parseCommands(char *cargs);

int error = 0;


Prompt parseArgs(int argc, char *argv[]) {
    Prompt opts;

    if (argc == 1) { // No arguments provided, use default prompt.
        opts.promptType = 1; 
        opts.promptString = "mysh: "; // Default prompt string
        return opts;
    } else if (argc == 2) {
        if (strcmp(argv[1], "-") == 0) { // If a single argument is "-", set to no prompt.
            opts.promptType = 0; 
            opts.promptString = ""; // Empty prompt string
            return opts;
        } else { // If a second argument is provided, set it as a custom prompt.
            opts.promptType = 2; // Custom prompt type
            opts.promptString = strcat(argv[1], ": "); // Custom prompt string
            return opts;
        }
    }

    // Invalid usage, print an error message and exit.
    fprintf(stderr, "Error: Usage: %s [prompt]\n", "mysh");
    exit(1);
}



char ** get_pipes( const char * line ) {
    char **tokens=NULL;
    char * line_copy;
    const char * delim = "|";
    char * cur_token;
    int num_tokens=0;
    
    tokens = (char**) malloc( sizeof(char*) );
    tokens[0] = NULL;
    
    if( line == NULL )
        return tokens;
    
    line_copy = strdup( line );
    cur_token = strtok( line_copy, delim );
    if( cur_token == NULL )
        return tokens;
    
    do {
        num_tokens++;
        tokens = ( char ** ) realloc( tokens, (num_tokens+1) * sizeof( char * ) );
        tokens[ num_tokens - 1 ] = strdup(cur_token);
        tokens[ num_tokens ] = NULL;
    } while( (cur_token = strtok( NULL, delim )) );
    free(line_copy);
    
    return tokens;
}

//catch errors here.
void check(char * cmdargv){
    
    char** cmds = get_pipes(cmdargv); 
    int num_cmds = 0;
    while(cmds[num_cmds] != NULL){
        num_cmds++;
    }

    char** tokens = get_tokens(cmdargv);    
    int counter = 0;
    int input_num = 0, output_num = 0;
    if (strcmp(&cmdargv[strlen(cmdargv) - 1], "|") == 0){    
        fprintf(stderr, "Error: Ambiguous output redirection.\n");
        error = 1;
    }else{
        for (int i = 0; tokens[i+1] != NULL; i++){   
            if (strcmp(tokens[i], "|") == 0){  
                if (strcmp(tokens[i], ">") == 0 ||strcmp(tokens[i], "|")== 0){  
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                }
            }
        }
        if (strcmp(tokens[0], "|")== 0){      
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                    return;
                }
        if (strcmp(tokens[0], ">")== 0){      
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                    return;
                }
        for (int i = 0; tokens[i] != NULL; i++){             
            if (!error){
                if (strcmp(tokens[i], "<") == 0){           
                    input_num++;
                    if (input_num >= 2){
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");
                        error = 1;
                        return;
                    }
                }else if (strcmp(tokens[i], ">") == 0){       
                    output_num++;
                    if (output_num >= 2){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                        return;
                    }
                }else if (strcmp(tokens[i], ">>") == 0){       
                    output_num++;
                    if (output_num >= 2){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                        return;
                    }
                }else if (strcmp(tokens[i], "|") == 0){       
                    output_num++;
                    if (output_num >= 2){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");  
                        error = 1;
                        output_num = 0;
                        return;
                    }else if (input_num >= 2 ){
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");  
                        error = 1;
                        input_num = 0;
                        return;
                    }else{
                        counter++;                            
                        input_num = 1;
                        output_num = 0;
                    }
                }
            }
        }
        

    }
}


void pushForward(int *foreground_processes, int pid, int num_cmds){
    for (int i = 0; i < num_cmds; i++){
        if (foreground_processes[i] == pid){
            foreground_processes[i] = -1;
        }
    }
}


bool waitHelper(int *foreground_processes, int num_cmds){
    int sum = 0;
    for (int i = 0; i < num_cmds; i++){
        sum += foreground_processes[i];
    }
    if (sum == (-1 * num_cmds)) {
        return 0;
    }
    return 1;
}


CmdSet parseCommands(char* cargs){
    CmdSet set_of_commands;
    char *background_char;
    background_char = strchr(cargs, '&');    
    
    if (strcmp(&cargs[strlen(cargs)-1], "&") == 0) {   
        cargs[strlen(cargs)-1] = '\0';
        set_of_commands.foreground_process = false;
    }else{                                                            
        set_of_commands.foreground_process = true;
    }

    if ((strchr(cargs, '&') != NULL) && strcmp(background_char, "&")){
        fprintf(stderr, "Error: \"&\" must be last token on command line\n");
        error = 1;
    }else {
        char **pipe_tokens = get_pipes(cargs);     
        int num_cmds;
        int counter = 0;
        while (pipe_tokens[counter] != NULL) {                             
            counter++;
        }
        set_of_commands.cmd_list = malloc(counter * sizeof(Cmd));  

        for (num_cmds = 0; pipe_tokens[num_cmds] != NULL; num_cmds++) {       
            char **cmd_tokens = get_tokens(pipe_tokens[num_cmds]); 
            Cmd new_cmd;
            new_cmd.append = false;
            new_cmd.input_file_name = malloc(1024 * sizeof(char));           
            new_cmd.output_file_name = malloc(1024 * sizeof(char));
            new_cmd.cmd = cmd_tokens;
            for (int num_args = 0; new_cmd.cmd[num_args] != NULL; num_args++) {     
                
                if ((new_cmd.cmd[num_args] != NULL) && strcmp(new_cmd.cmd[num_args], ">") == 0) {
                    if (new_cmd.cmd[num_args + 1] == NULL) {
                        fprintf(stderr, "Error: Missing filename for output redirection.\n");
                        error = 1;
                    } else if (strcmp(new_cmd.output_file_name, "") != 0) {
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                    }else {
                        int file_descriptor = open(new_cmd.cmd[num_args + 1], O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        if (file_descriptor == -1){
                            fprintf(stderr, "Error: open(\"%s\"): %s\n", new_cmd.cmd[num_args + 1], strerror(errno));
                            error = 1;
                        }else {
                            close(file_descriptor);
                            new_cmd.output_file_name = new_cmd.cmd[num_args + 1];  
                            new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                            if (num_args != 0) 
                                num_args -= 1;
                        }
                    }
                }
                
                if ((new_cmd.cmd[num_args] != NULL) && strcmp(new_cmd.cmd[num_args], "<") == 0) {
                    if (new_cmd.cmd[num_args + 1] == NULL) {
                        fprintf(stderr, "Error: Missing filename for input redirection.\n");
                        error = 1;
                    } else if (strcmp(new_cmd.input_file_name, "") != 0) {
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");
                        error = 1;
                    } else {
                        int file_descriptor = open(new_cmd.cmd[num_args + 1], O_RDONLY);
                        
                        if (file_descriptor == -1){
                            fprintf(stderr, "Error: open(\"%s\"): %s\n", new_cmd.cmd[num_args + 1], strerror(errno));
                            error = 1;
                        }else{
                            close(file_descriptor);
                            new_cmd.input_file_name = new_cmd.cmd[num_args + 1]; 
                            new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                            if (num_args != 0)      
                                num_args -= 1;
                        }
                    }
                }
                
                if ((new_cmd.cmd[num_args] != NULL) && strcmp(new_cmd.cmd[num_args], ">>") == 0) {
                    if (new_cmd.cmd[num_args + 1] == NULL) {
                        fprintf(stderr, "Error: Missing filename for output redirection.\n");
                        error = 1;
                    } else if (strcmp(new_cmd.output_file_name, "") != 0) {
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                    } else {
                        new_cmd.output_file_name = new_cmd.cmd[num_args + 1];
                        new_cmd.append = true;                            
                        new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                        if (num_args != 0)          
                            num_args -= 1;
                    }
                }
            }
            set_of_commands.cmd_list[num_cmds] = new_cmd;      
            set_of_commands.num_commands = counter;         
        }
    }
    return set_of_commands;
}

// Function to remove input/output redirection operators ("<", ">", ">>") from a list of tokens.
char** removeOperators(char **tokens, int operator_loc) {
    int counter = 0;
    
    // Count the number of tokens in the original list.
    while (tokens[counter] != NULL) {
        counter++;
    }
    
    // Allocate memory for the modified token list (excluding operators) and a NULL terminator.
    char **cmd_tokens = (char**) malloc((counter - 1) * sizeof(char*));
    
    int offset = 0;
    
    for (int i = 0; tokens[i] != NULL; i++) {
        // Iterate through each argument in the original token list.
        
        if ((offset != 2) && (i == operator_loc || (i == operator_loc + 1))) {
            // If it's the operator or the associated file, skip them by adjusting the offset.
            offset++;
        } else {
            // Otherwise, copy the token to the modified list, adjusting for the offset.
            cmd_tokens[i - offset] = tokens[i];
        }
    }
    
    // Set the last index of the modified list to NULL to indicate the end of the command.
    cmd_tokens[counter - offset] = NULL;
    
    return cmd_tokens;
}


int main(int argc, char **argv){
    char * cmdargv;
    char * fgets_return;
    bool cmds_executed = false;
    char **cmd_tokens;
    int pid, wpid, status;
    Prompt opts = parseArgs(argc, argv);
    int *foreground_pids;

    while(1){
        printf("%s", opts.promptString);
        cmdargv = malloc(1024);                                    
        do{                                                           
            fgets_return = fgets(cmdargv, 1024, stdin);
        }while(fgets_return == NULL && errno == EINTR);

        cmdargv[strlen(cmdargv) - 1] = '\0';                          
        error = 0;                                                     
        if (strcmp(cmdargv, "") == 0)                                 
            error = 1;                                               

        if( fgets_return == NULL || (strcmp(cmdargv, "exit") == 0)){
            if (errno != EINTR){
                exit(0);
            }
        }
        check(cmdargv);                                
        CmdSet set_of_commands;                            
        if (!error){                                             
            set_of_commands = parseCommands(cmdargv);
            for (int i = 0; i < set_of_commands.num_commands; i++){  
                if (set_of_commands.cmd_list[i].cmd[0] == NULL){      
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                    break;
                }
            }

        }

        if (!error){
            for (int i = 0; i < set_of_commands.num_commands; i++){           
                
                if (set_of_commands.num_commands > 1){
                    if ((i == 0) && (strcmp(set_of_commands.cmd_list[i].output_file_name, "") != 0) && (set_of_commands.num_commands > 1)){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                    }else if ((i == set_of_commands.num_commands - 1) && (strcmp(set_of_commands.cmd_list[i].input_file_name, "") != 0)){
                        
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");
                        error = 1;
                    }else if ((i != 0) && (i != set_of_commands.num_commands - 1)){
                        
                        if ((strcmp(set_of_commands.cmd_list[i].output_file_name, "") != 0)){
                            fprintf(stderr, "Error: Ambiguous output redirection.\n");
                            error = 1;
                        }else if ((strcmp(set_of_commands.cmd_list[i].input_file_name, "") != 0)){
                            fprintf(stderr, "Error: Ambiguous input redirection.\n");
                            error = 1;
                        }
                    }
                }
                
                if (set_of_commands.cmd_list[i].cmd[0] == NULL){
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                    break;
                }
            }
            
            foreground_pids = (int *) malloc(set_of_commands.num_commands * sizeof(int));
        }

        
        int pipefd[] = {-1, -1}, prevpipefd[] = {-1, -1};
        if (!error) {
            
            for (int cmd_index = 0; cmd_index < set_of_commands.num_commands; cmd_index++) {
                
                if ((set_of_commands.num_commands > 1) && (cmd_index < set_of_commands.num_commands - 1)) {
                    pipe(pipefd);
                }
                pid = fork();  
                if (pid == 0) {
                    
                    int file_descriptor;
                    if (set_of_commands.num_commands > 1) {        
                        if (cmd_index == 0) {               
                            dup2(pipefd[1], 1);
                            close(pipefd[0]);
                            close(pipefd[1]);
                        } else if (cmd_index == set_of_commands.num_commands - 1) {   
                            dup2(prevpipefd[0], 0);
                            close(prevpipefd[0]);
                        } else {
                            dup2(prevpipefd[0], 0);           
                            dup2(pipefd[1], 1);
                            close(pipefd[0]);
                            close(pipefd[1]);
                            close(prevpipefd[0]);
                        }
                    }
                    
                    if ((strcmp(set_of_commands.cmd_list[cmd_index].output_file_name, "") != 0)) {
                        if (set_of_commands.cmd_list[cmd_index].append == true) {
                            file_descriptor = open(set_of_commands.cmd_list[cmd_index].output_file_name,
                                                   O_CREAT | O_WRONLY | O_APPEND,
                                                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        } else {
                            file_descriptor = open(set_of_commands.cmd_list[cmd_index].output_file_name,
                                                   O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        }
                        if (file_descriptor == -1){
                            fprintf(stderr, "Error: open(\"%s\"): %s\n", set_of_commands.cmd_list[cmd_index].output_file_name, strerror(errno));
                            exit(0);
                        }
                        int dup_out = dup2(file_descriptor, 1);
                        close(file_descriptor);
                    }

                    
                    // if there is an input file then open it and dup the fd
                    if ((strcmp(set_of_commands.cmd_list[cmd_index].input_file_name, "") != 0)) {
                        printf("eye we are here doing this");
                        file_descriptor = open(set_of_commands.cmd_list[cmd_index].input_file_name, O_RDONLY);
                        if (file_descriptor == -1){
                            fprintf(stderr, "Error: open(\"%s\"): %s\n", set_of_commands.cmd_list[cmd_index].input_file_name, strerror(errno));
                            exit(0);
                        }
                        int dup_out = dup2(file_descriptor, 0);
                        close(file_descriptor);
                    }
                    // execute the cmd and check if it fails
                    if (execvp(set_of_commands.cmd_list[cmd_index].cmd[0], set_of_commands.cmd_list[cmd_index].cmd) ==
                        -1) {
                        perror("execvp()");
                    }
                    exit(-1);
                } else {
                    // parent process
                    foreground_pids[cmd_index] = pid;      

                    if (set_of_commands.foreground_process && (cmd_index == (set_of_commands.num_commands - 1))) {
                        while (waitHelper(foreground_pids, set_of_commands.num_commands)) {
                            // wait for termination
                            do{
                                wpid = wait(&status);
                            }while(wpid == -1 && errno == EINTR);
                            
                            pushForward(foreground_pids, wpid, set_of_commands.num_commands);
                        }
                    }
                    // check pipes
                    if (set_of_commands.num_commands > 1) {
                        close(pipefd[1]);
                        close(prevpipefd[0]);
                        prevpipefd[0] = pipefd[0];
                        prevpipefd[1] = pipefd[1];
                    }
                }
            }
        }
    }
}