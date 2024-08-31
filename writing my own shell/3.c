// THIS CODE IS OUR OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR
// OR CODE WRITTEN BY OTHER STUDENTS OUTSIDE OF OUR TEAM. lex lei
//
//

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


typedef struct {                // defines the options structure to hold the options for the program.
    int promptType;             // 0 = no prompt, 1 = default, 2 = custom
    char* promptString;
}Options;

typedef struct {
    bool append;
    char *input_file_name, *output_file_name, **cmd;
}Cmd;

typedef struct {
    bool foreground_process; // true is foreground, false is background
    Cmd *cmd_list;
    int num_commands;
}CmdSet;

Options parseArgs(int argc, char *argv[]);
CmdSet parseCommands(char *cmd_line_args);
void checkForRedirectError(char *cmdargv);
bool keepWaiting(int *foreground_processes, int num_cmds);
void setForegroundProcess(int *foreground_processes, int pid, int num_cmds);
char** removeOperators(char **tokens, int operator_loc);
int error = 0;

int main(int argc, char **argv){
    char * cmdargv;
    char * fgets_return;
    bool cmds_executed = false;
    char **cmd_tokens;
    int pid, wpid, status;
    Options opts = parseArgs(argc, argv);
    int *foreground_pids;

    while(1){
        printf("%s", opts.promptString);
        cmdargv = malloc(1024);                                    // set space for cmd
        do{                                                             // check if fgets fails, call again if interrupt
            fgets_return = fgets(cmdargv, 1024, stdin);
        }while(fgets_return == NULL && errno == EINTR);

        cmdargv[strlen(cmdargv) - 1] = '\0';                            // remove training newline char
        error = 0;                                                      // error number used to skip over running cmd
        if (strcmp(cmdargv, "") == 0)                                   // and go back to the top of while
            error = 1;                                                  // check for no cmd entered

        // if the user types exit or control-d, exit shell
        if( fgets_return == NULL || (strcmp(cmdargv, "exit") == 0)){
            if (errno != EINTR){
                exit(0);
            }
        }
        checkForRedirectError(cmdargv);                                 // check for redirect errors
        CmdSet set_of_commands;                                         // create struct for all cmds
        if (!error){                                                    // if no error, parse the cmd line
            set_of_commands = parseCommands(cmdargv);
            for (int i = 0; i < set_of_commands.num_commands; i++){     // for each cmd given,
                if (set_of_commands.cmd_list[i].cmd[0] == NULL){        // check if a cmd is given, otherwise null error
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                    break;
                }
            }
        }

        if (!error){
            for (int i = 0; i < set_of_commands.num_commands; i++){             // loop through each cmd
                // if the first first cmd has an output file and a pipe, error
                if (set_of_commands.num_commands > 1){
                    if ((i == 0) && (strcmp(set_of_commands.cmd_list[i].output_file_name, "") != 0) && (set_of_commands.num_commands > 1)){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                    }else if ((i == set_of_commands.num_commands - 1) && (strcmp(set_of_commands.cmd_list[i].input_file_name, "") != 0)){
                        // if the last cmd has an input file and pipe, error
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");
                        error = 1;
                    }else if ((i != 0) && (i != set_of_commands.num_commands - 1)){
                        // if a middle cmd has either an input or output file, error
                        if ((strcmp(set_of_commands.cmd_list[i].output_file_name, "") != 0)){
                            fprintf(stderr, "Error: Ambiguous output redirection.\n");
                            error = 1;
                        }else if ((strcmp(set_of_commands.cmd_list[i].input_file_name, "") != 0)){
                            fprintf(stderr, "Error: Ambiguous input redirection.\n");
                            error = 1;
                        }
                    }
                }
                // if a cmd doesn't have a cmd, i.e. only a redirection, error.
                if (set_of_commands.cmd_list[i].cmd[0] == NULL){
                    fprintf(stderr, "Error: Invalid null command.\n");
                    error = 1;
                    break;
                }
            }
            // make space to save the foreground pids
            foreground_pids = (int *) malloc(set_of_commands.num_commands * sizeof(int));
        }

        // create pipes
        int pipefd[] = {-1, -1}, prevpipefd[] = {-1, -1};
        if (!error) {
            // if no error discovered, go and iterate through each cmd that was parsed for execution
            for (int cmd_index = 0; cmd_index < set_of_commands.num_commands; cmd_index++) {
                // if theres more than 1 cmd, theres a pipe so create a pipe for the middle cmds
                if ((set_of_commands.num_commands > 1) && (cmd_index < set_of_commands.num_commands - 1)) {
                    pipe(pipefd);
                }
                pid = fork();   // fork off child process
                if (pid == 0) {
                    // child process
                    int file_descriptor;
                    if (set_of_commands.num_commands > 1) {             // if more than 1 cmd, handle duping pipes
                        if (cmd_index == 0) {                   // if this is the first cmd in a cmd with pipes
                            dup2(pipefd[1], 1);
                            close(pipefd[0]);
                            close(pipefd[1]);
                        } else if (cmd_index == set_of_commands.num_commands - 1) {     // if this is the last cmd
                            dup2(prevpipefd[0], 0);
                            close(prevpipefd[0]);
                        } else {
                            dup2(prevpipefd[0], 0);             // if its a middle cmd with pipes on both sides
                            dup2(pipefd[1], 1);
                            close(pipefd[0]);
                            close(pipefd[1]);
                            close(prevpipefd[0]);
                        }
                    }
                    // if there is an output file, open it with either append or create it and dup
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
                    char * input=set_of_commands.cmd_list[cmd_index].input_file_name;
                    int a=strlen(input);
                    if (strlen(input) >0) {
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
                    foreground_pids[cmd_index] = pid;       // add foreground pids to list for tracking

                    // if the cmd is a foreground process and you're at the last cmd, do the waiting
                    if (set_of_commands.foreground_process && (cmd_index == (set_of_commands.num_commands - 1))) {
                        // keep waiting for all the foreground processes
                        while (keepWaiting(foreground_pids, set_of_commands.num_commands)) {
                            // wait for first child process that terminates
                            do{
                                wpid = wait(&status);
                            }while(wpid == -1 && errno == EINTR);
                            // after waiting, set that process to have been waited for
                            setForegroundProcess(foreground_pids, wpid, set_of_commands.num_commands);
                        }
                    }
                    // if theres a pipes, close fd's in the parent
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

// checks the cmd string for redirect errors
void checkForRedirectError(char * cmdargv){
    
    char** cmds = get_pipes(cmdargv); // split by pipe to get number of cmds
    int num_cmds = 0;
    while(cmds[num_cmds] != NULL){
        num_cmds++;
    }

    char** tokens = get_tokens(cmdargv);       // get each part of total cmd as tokens
    int counter = 0;
    int input_num = 0, output_num = 0;
    if (strcmp(&cmdargv[strlen(cmdargv) - 1], "|") == 0){     // if the last token is a pipe, error
        fprintf(stderr, "Error: Ambiguous output redirection.\n");
        error = 1;
    }else{
        for (int i = 0; tokens[i] != NULL; i++){              // iterate through each token
            if (!error){
                if (strcmp(tokens[i], "<") == 0){               // if there is more than one input, error
                    input_num++;
                    if (input_num >= 2){
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");
                        error = 1;
                    }
                }else if (strcmp(tokens[i], ">") == 0){         // more than one output, error
                    output_num++;
                    if (output_num >= 2){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                    }
                }else if (strcmp(tokens[i], ">>") == 0){        // more than one output error
                    output_num++;
                    if (output_num >= 2){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                    }
                }else if (strcmp(tokens[i], "|") == 0){         // if theres a pipe, add an output to previous cmd
                    output_num++;
                    if (output_num >= 2){
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");  // if more than one output, error
                        error = 1;
                        output_num = 0;
                    }else if (input_num >= 2 ){
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");   // if more than one input, error
                        error = 1;
                        input_num = 0;
                    }else{
                        counter++;                              // set the next cmd to already have one input from pipe
                        input_num = 1;
                        output_num = 0;
                    }
                }
            }
        }
    }
}

// loop through the list of foreground pids and if that pid was waited for, set it to -1
void setForegroundProcess(int *foreground_processes, int pid, int num_cmds){
    for (int i = 0; i < num_cmds; i++){
        if (foreground_processes[i] == pid){
            foreground_processes[i] = -1;
        }
    }
}

// check if you need to wait for more foreground pids. Do it by adding up the values and if
// its all -1, it will be -num_cmds. If all waited for (i.e -1) then return false.
bool keepWaiting(int *foreground_processes, int num_cmds){
    int sum = 0;
    for (int i = 0; i < num_cmds; i++){
        sum += foreground_processes[i];
    }
    if (sum == (-1 * num_cmds)) {
        return 0;
    }
    return 1;
}

// parse the cmd line that was given
CmdSet parseCommands(char* cmd_line_args){
    CmdSet set_of_commands;
    char *background_char;
    background_char = strchr(cmd_line_args, '&');       // look for background operator

    if (strcmp(&cmd_line_args[strlen(cmd_line_args)-1], "&") == 0) {    // if  last character, save it as bkgd process
        cmd_line_args[strlen(cmd_line_args)-1] = '\0';
        set_of_commands.foreground_process = false;
    }else{                                                              // else it is a foreground process
        set_of_commands.foreground_process = true;
    }

    // if the operator isn't the last character, error
    if ((strchr(cmd_line_args, '&') != NULL) && strcmp(background_char, "&")){
        fprintf(stderr, "Error: \"&\" must be last token on command line\n");
        error = 1;
    }else {
        char **pipe_tokens = get_pipes(cmd_line_args);        // otherwise split cmd by pipe
        int num_cmds;
        int counter = 0;
        while (pipe_tokens[counter] != NULL) {                                // number of commands
            counter++;
        }
        set_of_commands.cmd_list = malloc(counter * sizeof(Cmd));   // malloc space for cmds

        for (num_cmds = 0; pipe_tokens[num_cmds] != NULL; num_cmds++) {             // for each cmd
            char **cmd_tokens = get_tokens(pipe_tokens[num_cmds]);   // split by args
            Cmd new_cmd;
            new_cmd.append = false;
            new_cmd.input_file_name = malloc(1024 * sizeof(char));             // malloc space for input and output
            new_cmd.output_file_name = malloc(1024 * sizeof(char));
            new_cmd.cmd = cmd_tokens;
            for (int num_args = 0; new_cmd.cmd[num_args] != NULL; num_args++) {     // for each arg in a cmd
                // if there is a output redirection check if its valid and then save it
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
                            new_cmd.output_file_name = new_cmd.cmd[num_args + 1];   // remove the operators from cmd
                            new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                            if (num_args != 0)  // go back one cause the operator has been removed
                                num_args -= 1;
                        }
                    }
                }
                // if there is a input redirection check if its valid and then save it
                if ((new_cmd.cmd[num_args] != NULL) && strcmp(new_cmd.cmd[num_args], "<") == 0) {
                    if (new_cmd.cmd[num_args + 1] == NULL) {
                        fprintf(stderr, "Error: Missing filename for input redirection.\n");
                        error = 1;
                    } else if (strcmp(new_cmd.input_file_name, "") != 0) {
                        fprintf(stderr, "Error: Ambiguous input redirection.\n");
                        error = 1;
                    } else {
                        int file_descriptor = open(new_cmd.cmd[num_args + 1], O_RDONLY);
                        // if the file descriptor errors (i.e no file, error)
                        if (file_descriptor == -1){
                            fprintf(stderr, "Error: open(\"%s\"): %s\n", new_cmd.cmd[num_args + 1], strerror(errno));
                            error = 1;
                        }else{
                            close(file_descriptor);
                            new_cmd.input_file_name = new_cmd.cmd[num_args + 1]; // remove the operator and save
                            new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                            if (num_args != 0)      // go back one because operator was removed
                                num_args -= 1;
                        }
                    }
                }
                // if there is a input redirection to append check if its valid and then save it
                if ((new_cmd.cmd[num_args] != NULL) && strcmp(new_cmd.cmd[num_args], ">>") == 0) {
                    if (new_cmd.cmd[num_args + 1] == NULL) {
                        fprintf(stderr, "Error: Missing filename for output redirection.\n");
                        error = 1;
                    } else if (strcmp(new_cmd.output_file_name, "") != 0) {
                        fprintf(stderr, "Error: Ambiguous output redirection.\n");
                        error = 1;
                    } else {
                        new_cmd.output_file_name = new_cmd.cmd[num_args + 1];
                        new_cmd.append = true;                              // set append to true and remove operators
                        new_cmd.cmd = removeOperators(new_cmd.cmd, num_args);
                        if (num_args != 0)          // go back one because operator was removed
                            num_args -= 1;
                    }
                }
            }
            set_of_commands.cmd_list[num_cmds] = new_cmd;       // add the final parsed cmd to list
            set_of_commands.num_commands = counter;             // save number of cmds
        }
    }
    return set_of_commands;
}

// removes the <, >, >> operators from the cmd
char** removeOperators(char **tokens, int operator_loc){
    int counter = 0;
    // get number of args in cmd
    while(tokens[counter] != NULL){
        counter++;
    }
    char **cmd_tokens = (char**) malloc((counter - 1) * sizeof(char*)); // malloc space just cmds and NULL end

    int offset = 0;
    for (int i = 0; tokens[i] != NULL; i++){                                        // iterate through each arg
        if ((offset != 2) && (i == operator_loc || (i == operator_loc + 1))) {     // if its the operator or file,remove
            offset++;
        }else{                                                                      // otherwise keep it
            cmd_tokens[i - offset] = tokens[i];
        }
    }
    cmd_tokens[counter-offset] = NULL;                                      // set last index to NULL for end of cmd
    return cmd_tokens;
}

// parse the args
Options parseArgs(int argc, char *argv[]){
    Options opts;
    if (argc == 1){                             // if no args, default prompt
        opts.promptType = 1;
        opts.promptString = "mysh: ";
        return opts;
    }else if (argc == 2){                      // if  - then no prompt
        if (strcmp(argv[1], "-") == 0){
            opts.promptType = 0;
            opts.promptString = "";
            return opts;
        }else {                                 // if a second prompt then make it the custom one.
            opts.promptType = 2;
            opts.promptString = strcat(argv[1],": ");
            return opts;
        }
    }
    fprintf(stderr, "Error: Usage: %s [prompt]\n", "mysh");
    exit(1);
}