/* THIS CODE WAS MY OWN WORK , IT WAS WRITTEN WITHOUT CONSULTING ANY
SOURCES OUTSIDE OF THOSE APPROVED BY THE INSTRUCTOR . Lex Lei */
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include "inodemap.h"

extern char const ** Map;//declaring global variable

typedef struct {               
    int createArchive, extractArchive, printContents;
    char *archiveFile;
    char *inputDirectory;
}Options;

/*
we can handel the following errors in parse:
“Error: No tarfile specified.\n”
“Error: Multiple modes specified.\n”
“Error: No mode specified.\n”
done

the rest of the errors are:

“Error: Specified target(\“%s\” is not a directory.\n)” 
when create has a dir but is not a dir 

“Error: No directory target specified.\n”
when create has no dir 

“Error: Bad magic number (%d), should be: %d.\n”
error for extract and print

“Error: Specified target(\“%s\” does not exist.\n)” 
when extract and print are given a tar that does not exist

*/

// Function prototypes
Options parseArgs(int argc, char *argv[]);
void checkModes (int numTrans);
void createArchive(FILE *archiveFile, char *inputDirectoryString);
void printContents(char *archiveFileString);
void extractArchive(FILE *archiveFile);

Options parseArgs(int argc, char *argv[]){
    Options opts;
    opts.createArchive = 0;
    opts.extractArchive = 0;
    opts.printContents = 0;
    opts.archiveFile=NULL;
    opts.inputDirectory=NULL;
    int count=0;
    int countflags=0;
    int option;
    while ((option = getopt(argc, argv, "cxtf:")) != -1) {
        switch (option) {
            case 'c':
                count++;
                opts.createArchive = 1;
                break;
            case 'x':
                count++;
                opts.extractArchive = 1;
                break;
            case 't':
                count++;
                opts.printContents = 1;
                break;
            case 'f':
                countflags++;
                opts.archiveFile = optarg;
                break;
            default:
                fprintf(stderr, "Error: No tarfile specified\n");
                exit(-1);
        }
        
    }
    if (count>1 || countflags>1){
        fprintf(stderr, "Error: Multiple modes specified.\n");
        exit(-1);
    }
    if (count<1){
        fprintf(stderr, "Error: No mode specified\n");
        exit(-1);
    }
    if (countflags<1 || !opts.archiveFile){
        fprintf(stderr, "Error: No tarfile specified\n");
        exit(-1);
    }
    
    if (optind < argc) {
        opts.inputDirectory = argv[optind];
    }

    if (opts.createArchive && opts.inputDirectory == NULL){            
        fprintf(stderr, "Error: No directory target specified\n");
        exit(-1);
    }

    if (opts.createArchive && opts.inputDirectory != NULL){    
        struct stat file_stat;
        if (lstat(opts.inputDirectory, &file_stat) != 0) {     
            fprintf(stderr, "Specified target(\"%s\") does not exist.\n", opts.inputDirectory);
            exit(-1);
        }else{
            if (!S_ISDIR(file_stat.st_mode)) {             
                fprintf(stderr, "Specified target(\"%s\") is not a directory.\n", opts.inputDirectory);
                exit(-1);
            }
        }
    }
    return opts;
}

void printContents(char *archiveFileString){
    /* IMPORTANT NOTICE: from now we need to close archive*/
    FILE *archiveFile = fopen(archiveFileString,"r");      
    if (archiveFile == NULL){
        perror("fopen");
        exit(-1);
    }
    int32_t magic_number;
    if (!fread(&magic_number, 4, 1, archiveFile)){                       
        perror("fread");
        if (fclose(archiveFile) != 0){         
            perror("fclose");
            exit(-1);
        }
        exit(-1);
    }
    if (magic_number != 0x7261746D){                                                 
        fprintf(stderr, "Error: Bad magic number (%d), should be %d.\n", magic_number, 0x7261746D);
        if (fclose(archiveFile) != 0){         
        perror("fclose");
        exit(-1);
    }
        exit(-1);
    }
    ino_t inode_num = 0;
    int64_t inode_num_int;
    while(fread(&inode_num_int, 8, 1, archiveFile) == 1){     
        inode_num = (ino_t) inode_num_int;                           
        int32_t file_name_length;
        if (!fread(&file_name_length, 4, 1, archiveFile)){   
            perror("fread");
            if (fclose(archiveFile) != 0){         
                perror("fclose");
                exit(-1);
            }
            exit(-1);
        }
        /* from now we need to free*/
        char *file_name = (char *) malloc((file_name_length) * sizeof(char));   
        if (file_name == NULL){
            perror("malloc");
            if (fclose(archiveFile) != 0){         
                perror("fclose");
                exit(-1);
            }
            exit(-1);
        }
        if (!fread(file_name, file_name_length, 1, archiveFile)){    
            free(file_name);   
            perror("fread");
            if (fclose(archiveFile) != 0){         
                perror("fclose");
                exit(-1);
            }
            exit(-1);
        }
        file_name[file_name_length] = '\0';                                   
        if (get_inode(inode_num) != NULL){
            printf("%s/ -- inode: %lu\n", file_name, inode_num);           //hardlink            
        }else{
            mode_t read_mode;
            int32_t read_mode_int;
            if (fread(&read_mode_int, 4, 1, archiveFile) == -1) { 
                free(file_name);
                perror("fread");
                if (fclose(archiveFile) != 0){         
                    perror("fclose");
                    exit(-1);
                }
                exit(-1);
            }
            read_mode = (mode_t) read_mode_int;
            time_t read_mtime;
            int64_t read_mtime_int;
            if (!fread(&read_mtime_int, 8, 1, archiveFile)){   
                free(file_name);      
                perror("fread");
                if (fclose(archiveFile) != 0){   
                    perror("fclose");
                    exit(-1);
                }
                exit(-1);
            }
            read_mtime = (time_t) read_mtime_int;
            set_inode(inode_num, file_name);                               
            if (S_ISDIR(read_mode)){                                                 
                printf("%s/ -- inode: %llu, mode %o, mtime: %llu\n", file_name, (unsigned long long) inode_num, read_mode & 0777, (unsigned long long) read_mtime); //dir
            }else {
                off_t read_size;                                                    
                int64_t read_size_int;
                if (!fread(&read_size_int, 8, 1, archiveFile)){
                    free(file_name);
                    perror("fread");
                    if (fclose(archiveFile) != 0){         
                        perror("fclose");
                        exit(-1);
                    }
                    exit(-1);
                }
                read_size = (off_t) read_size_int;
                if (read_size != 0) {                                           
                    char *read_contents = (char *) malloc(read_size * sizeof(char)); 
                    if (read_contents == NULL) {
                        free(file_name);
                        perror("malloc");
                        if (fclose(archiveFile) != 0){         
                            perror("fclose");
                            exit(-1);
                        }
                        exit(-1);
                    }
                    if (!fread(read_contents, read_size, 1, archiveFile)) {  
                        free(file_name);
                        free(read_contents);     
                        perror("fread");
                        if (fclose(archiveFile) != 0){       
                            perror("fclose");
                            exit(-1);
                        }
                        exit(-1);
                    }
                    free(read_contents);
                }
                if ((read_mode & S_IXUSR) || (read_mode & S_IXGRP) || (read_mode & S_IXOTH)){    
                    printf("%s* -- inode: %lu, mode: %o, mtime: %llu, size: %lu\n", file_name, inode_num, read_mode& 0777,
                           (unsigned long long) read_mtime, read_size);
                }else{                                                                      
                    printf("%s -- inode: %lu, mode: %o, mtime: %llu, size: %lu\n", file_name, inode_num, read_mode& 0777,
                           (unsigned long long) read_mtime, read_size);
                }
            }
        }
        free(file_name);
    }
    
    if (fclose(archiveFile) != 0){         
        perror("fclose");
        exit(-1);
    }
    
}
/*create does the following: opens up dir, and starts writing everything in it 
i.e. we open input and put things in archive
*/
void createArchive(FILE *archiveFile, char *inputDirectoryString){
    struct dirent *de;
    struct stat file_stat;
    char * fullname;

    DIR *input_directory = opendir(inputDirectoryString);//closed         
    if (input_directory == NULL){
        perror("opendir");
        exit(-1);
    }

    fullname = (char *) malloc((strlen(inputDirectoryString)+256));     //freed
    if (fullname == NULL){
        perror("malloc");
        exit(-1);
    }
    for (de = readdir(input_directory); de != NULL; de = readdir(input_directory)) {    
        if (!sprintf(fullname, "%s/%s", inputDirectoryString, de->d_name)){              
            perror("sprintf");
            exit(-1);
        }
        if (lstat(fullname, &file_stat) != 0){                                          
            perror("lstat");
            exit(-1);
        }
        if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0 && !S_ISLNK(file_stat.st_mode)){
            int64_t inode_num = (int64_t) file_stat.st_ino;            
            if (!fwrite(&inode_num, 8, 1, archiveFile)) {               
                perror("fwrite");
                exit(-1);
            }
            int32_t file_name_length = strlen(fullname);
            if (!fwrite(&file_name_length, 4, 1, archiveFile)) {                  
                perror("fwrite");
                exit(-1);
            }
            if (!fwrite(fullname, file_name_length, 1, archiveFile)) {             
                perror("fwrite");
                exit(-1);
            }
            if(!get_inode( file_stat.st_ino )) {         
                set_inode(file_stat.st_ino, fullname);
                int32_t mode = (int32_t) file_stat.st_mode;
                if (!fwrite(&mode, 4, 1, archiveFile)) {                    
                    perror("fwrite");
                    exit(-1);
                }
                int64_t time_num = (int64_t) file_stat.st_mtim.tv_sec;
                if (!fwrite(&time_num, 8, 1, archiveFile)) {         
                    perror("fwrite");
                    exit(-1);
                }
                if (!S_ISDIR(file_stat.st_mode)){
                    int64_t file_size = (int64_t) file_stat.st_size;
                    if (!fwrite(&file_size, 8, 1, archiveFile)) {           
                        perror("fwrite");
                        exit(-1);
                    }
                    if (file_size != 0) {                                                
                        FILE *inputFile = fopen(fullname, "r");                     
                        if (inputFile == NULL) {
                            perror("fopen");
                            exit(-1);
                        }
                        char *read_contents = malloc(file_stat.st_size * sizeof(char));    
                        if (read_contents == NULL) {
                            perror("malloc");
                            exit(-1);
                        }
                        if (!fread(read_contents, file_stat.st_size, 1, inputFile)) {    
                            perror("fread");
                            exit(-1);
                        }
                        if (!fwrite(read_contents, file_stat.st_size, 1, archiveFile)) {   
                            perror("fwrite");
                            exit(-1);
                        }
                        if (fclose(inputFile) != 0){                                              
                            perror("fclose");
                            exit(-1);
                        }
                        free(read_contents);
                    }
                }
            }
        }
        if (S_ISDIR(file_stat.st_mode) && strcmp(de->d_name, ".") !=0 && strcmp(de->d_name, "..") !=0 ) {
            createArchive(archiveFile, fullname);              
        }
    }
    free(fullname);
    if (closedir(input_directory) != 0){       
        perror("closedir");
        exit(-1);
    }
}

/*
we first read dir, but we don't need dir's time
we read in the order: mode, modifictaion time, size content
*/
void extractArchive(FILE *archiveFile){
    int32_t magic_number;
        if (!fread(&magic_number, 4, 1, archiveFile)){
            perror("fread");
            exit(-1);
        }
        if (magic_number != 0x7261746D){                              
            fprintf(stderr, "Error: Bad magic number (%d), should be %d.\n", magic_number, 0x7261746D);
            exit(-1);
        }
    ino_t inode_num;
    int64_t inode_num_int;

    while(fread(&inode_num_int, 8, 1, archiveFile) == 1){           
        inode_num = (ino_t) inode_num_int;                                     
        int32_t file_name_length;
        if (!fread(&file_name_length, 4, 1, archiveFile)){          
            perror("fread");
            exit(-1);
        }
        char *file_name =(char *)  malloc((file_name_length+1)* sizeof(char));   //file name needs to be freed
        if (file_name == NULL){
            perror("malloc");
            exit(-1);
        }
        if (!fread(file_name, file_name_length, 1, archiveFile)){      
            perror("fread");
            exit(-1);
        }
        file_name[file_name_length] = '\0';   

        if(get_inode(inode_num) != NULL){                              
            char *src_name = (char *) malloc((strlen(get_inode(inode_num)) + 1) * sizeof(char)); 
            if (src_name == NULL){
                perror("malloc");
                exit(-1);
            }
            const char * temp=get_inode(inode_num);
            if (strcpy(src_name, get_inode(inode_num)) == NULL){       
                perror("strcpy");
                exit(-1);
            }
            if (link( src_name, file_name) != 0){  //para1 is existing file para2 is link to be created        
                perror("link");
                exit(-1);
            }
            free(src_name);
            

        }else{                                                         
            int32_t file_mode;
            if (!fread(&file_mode, 4, 1, archiveFile)){        
                perror("fread");
                exit(-1);
            }
            int64_t file_mtime;  
            if (!fread(&file_mtime, 8, 1, archiveFile)){      
                perror("fread");
                exit(-1);
            }

            if (S_ISDIR((mode_t) file_mode)){                           
                if (mkdir(file_name, (mode_t) file_mode) != 0){           
                    perror("mkdir");
                    exit(-1);
                }
                set_inode(inode_num, file_name);                        
            }else{ //now it's not a link or dir, it must be a file
                int64_t file_size;
                if (!fread(&file_size, 8, 1, archiveFile)){    
                    perror("fread");
                    exit(-1);
                }
                FILE *currentFile = fopen(file_name, "w+");           
                if (currentFile == NULL) {
                    perror("fopen");
                    exit(-1);
                }
                if (file_size != 0) {                                      

                    char *contents = (char *) malloc(file_size * sizeof(char));   //file content waiting to be freed
                    if (contents == NULL) {
                        perror("malloc");
                        exit(-1);
                    }
                    if (!fread(contents, file_size, 1, archiveFile)) {         
                        perror("fread");
                        exit(-1);
                    }
                    if (!fwrite(contents, file_size, 1, currentFile)) {           
                        perror("fwrite");
                        exit(-1);
                    }
                    free(contents);
                }
                if (chmod(file_name, (mode_t) file_mode) != 0){            
                    perror("fread");
                    exit(-1);
                }

                struct timeval times[2];
                // Set access time to current time
                if (gettimeofday(&times[0], NULL) != 0) { //this gives secons and microseconds of times[0]=access time
                    perror("gettimeofday");
                    exit(-1);
                }
                // Set modification time to the current modification time of the file
                times[1].tv_sec = (time_t) file_mtime;
                times[1].tv_usec = 0;

                                      

                if (fclose(currentFile) != 0){                            
                    perror("fclose");
                    exit(-1);
                }
                if (utimes(file_name, times) != 0) {
                    perror("utimes");
                    exit(-1);
                }
                char * in;
                if (strcpy(in, file_name) == NULL){       
                    perror("strcpy");
                    exit(-1);
                }
                const char * inputStringName=in;
                set_inode(inode_num, inputStringName);   
                
            }
        }
        free(file_name);
    }
}

int main(int argc, char *argv[]) {
    Options opts = parseArgs(argc, argv);                                       
    if (opts.createArchive == 1){
        FILE *archiveFile = fopen(opts.archiveFile,"w+");                
        if (archiveFile == NULL) {
            perror("fopen");
            exit(-1);
        }
        int32_t magic_number = 0x7261746D;                                       
        if (!fwrite(&magic_number, 4, 1, archiveFile)){
            perror("fwrite");
            exit(-1);
        }
       
        struct stat file_stat;                                                    
        if (lstat(opts.inputDirectory, &file_stat) != 0) {
            perror("lstat");
            exit(-1);
        }
        int64_t inode_num = (int64_t) file_stat.st_ino;
        if (!fwrite(&inode_num , 8, 1, archiveFile)) {                  
            perror("fwrite");
            exit(-1);
        }
        set_inode(file_stat.st_ino, opts.inputDirectory);
        int32_t file_name_length = strlen(opts.inputDirectory);
        if (!fwrite(&file_name_length, 4, 1, archiveFile)) {                 
            perror("fwrite");
            exit(-1);
        }
        if (!fwrite(opts.inputDirectory, file_name_length, 1, archiveFile)) {    
            perror("fwrite");
            exit(-1);
        }
        int32_t mode = (int32_t) file_stat.st_mode;
        if (!fwrite(&mode, 4, 1, archiveFile)) {                   
            perror("fwrite");
            exit(-1);
        }
        int64_t time_num = (int64_t) file_stat.st_mtim.tv_sec;
        if (!fwrite(&time_num, 8, 1, archiveFile)) {         
            perror("fwrite");
            exit(-1);
        }
        createArchive(archiveFile, opts.inputDirectory);               
        if (fclose(archiveFile) != 0){                                 
            perror("fclose");
            exit(-1);
        }
    }
    if (opts.printContents == 1){                                    
        printContents(opts.archiveFile);
    }
    if (opts.extractArchive == 1){                                    
        FILE *archiveFile = fopen(opts.archiveFile,"r");        
        if (archiveFile == NULL) {
            perror("fopen");
            exit(-1);
        }
        
        extractArchive(archiveFile);                                 
        if (fclose(archiveFile) != 0){                                
            perror("fclose");
            exit(-1);
        }
    }
    free(Map);
    exit(0);
}