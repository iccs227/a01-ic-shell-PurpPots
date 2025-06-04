/* ICCS227: Project 1: icsh
 * Name: Alyza Cadelina
 * StudentID: 6480512
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#define MAX_CMD_BUFFER 255

//global variables
pid_t foreground_pid = 0;
int last_status = 0; //exit status of last command - 0=success, non-0=error

//for milestone 7
time_t session_start;
int commands_run = 0;

// structure for background jobs
typedef struct {
    int job_id;
    pid_t pid;
    char command[MAX_CMD_BUFFER];
    int status;
    int is_curr;
} job_t;

job_t jobs[100]; //arr for background jobs, max 100
int job_count = 0;
int next_job_id=1;

//check if command should run in background
int is_background(char **args) { // 1 if background, 0 if foreground
    int i= 0;
    while(args[i] !=NULL) {
        i++;
    }
    if (i > 0 && strcmp(args[i-1], "&") == 0) { //check if last argument is &
        args[i-1] = NULL; //removing & from args array
        return 1;
    }
    return 0;
}

//adding new background job to job arr
void add_background_job(pid_t pid, char *command) {
    if (job_count < 100) { //limit 100 max jobs
        jobs[job_count].job_id = next_job_id++;
        jobs[job_count].pid = pid;
        strcpy(jobs[job_count].command, command);
        jobs[job_count].status = 0; //0 for running
        jobs[job_count].is_curr = 1;

        for(int i=0; i< job_count; i++) {
            jobs[i].is_curr = 0;
        }
        printf("[%d] %d\n", jobs[job_count].job_id, pid);
        job_count++;
    }
}

//removing jobs once they have been completed
void delete_job(int idx) {
    for (int i=idx; i< job_count-1; i++) {
        jobs[i] = jobs[i+1];
    }
    job_count--;
}

// find job, return arr index of job, otherwise -1 if not found
int find_job(int job_id) {
    for (int i=0; i< job_count; i++) {
        if(jobs[i].job_id == job_id) return i;
    }
    return -1;
}

//bringing background job to foreground
void make_foreground(int job_id) {
    int index = find_job(job_id);
    if (index == -1) {
        printf("icsh: fg: %d: no such job\n", job_id);
        last_status = 1; //error exit status
        return;
    }

    for (int i=0; i< job_count; i++) {
        jobs[i].is_curr = (i == index); //true for target job
    }

    printf("%s\n", jobs[index].command); //print command
    
    if (jobs[index].status ==2) { // if job status is stopped (2)
        kill(jobs[index].pid, SIGCONT); // resume job
        jobs[index].status = 0;
    }

    foreground_pid = jobs[index].pid;
    int status; //storing exit status
    waitpid(jobs[index].pid, &status, 0); // waiting for job to complete
    foreground_pid = 0;

    if(WIFEXITED(status)) { //job exited normally
        last_status = WEXITSTATUS(status); //store exit code
    } else if (WIFSIGNALED(status)) { // job killed by signal
        last_status = 128 + WTERMSIG(status); 
    }

    delete_job(index); //remove completed job
}

//send job to background
void send_background(int job_id) {
    int index = find_job(job_id);
    if (index == -1) {
        printf("icsh: bg: %d: no such job\n", job_id);
        last_status = 1; //error exit status
        return;
    }

    for (int i=0; i< job_count; i++) {
        jobs[i].is_curr = (i==index); //true for target job
    }

    if (jobs[index].status == 2) { //job stopped
        kill(jobs[index].pid, SIGCONT);
        jobs[index].status = 0;
        printf("[%d]+ %s &\n", jobs[index].job_id, jobs[index].command);
    } else {
        printf("[%d]+ %s &\n", jobs[index].job_id, jobs[index].command);
    }

    last_status = 0;
}

//check for completed background jobs
void check_background_jobs() {
    for (int i=0; i< job_count; i++) {
        int status;
        pid_t result = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED);
        //WNOHANG - dont wait if still running, WUNTRACED - report if job stopped

        if (result == jobs[i].pid) {
            if (WIFSTOPPED(status)) {
                jobs[i].status = 2;
                printf("[%d]+ Stopped    %s\n", jobs[i].job_id, jobs[i].command);
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) { //job is finished
                printf("[%d]%c Done    %s\n",
                jobs[i].job_id, jobs[i].is_curr ? '+' : '-', jobs[i].command);
                delete_job(i);
                i--;
            }
        } else if (result == -1) { //job might have been done or doesn't exist
            delete_job(i);
            i--;
        }
    }
}

//printing all background jobs
void print_jobs() {
    
    if (job_count == 0) {
	    printf("No current jobs");
	    return;
    }

    for (int i=0; i<job_count; i++) {
        char *status_str; //status to display in string
        if (jobs[i].status == 0) {
            status_str = "running";
        } else if (jobs[i].status == 2) {
            status_str = "stopped";
        } else { 
            status_str = "done";
        }

        printf("[%d]%c    %-20s %s\n", jobs[i].job_id, jobs[i].is_curr ? '+' : '-', status_str, jobs[i].command);
    }
}



//remove trailing newline \n to \0
void trim_str(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len-1] == '\n') str[len-1] = 0;
}

//check and handle I/O redirection
int check_io_redirection(char **args, char **input_file, char **output_file) {
    int argc = 0; //counter for command arguments
    *input_file = NULL;
    *output_file = NULL;

    for (int i=0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") ==0) { //input direction
            if (args[i+1] != NULL) {
                *input_file = args[i+1]; //set input file to name after
                i++; //skip filename
            }
        } else if (strcmp(args[i], ">") ==0) { //output direction
            if (args[i+1] != NULL) {
                *output_file = args[i+1]; //set output file to name after
                i++; //skip filename
            }
        } else {
            args[argc] = args[i];
            argc++;
        }
    }
    args[argc] = NULL;
    return argc;
}


//split command str into arr of arguments for external program
void parse_command(char *command, char **args) {
    char *token =strtok(command, " \t"); //first token
    for (int i = 0; i < 23 && token !=NULL; i++) {
        args[i] = token; //curr token stored in args array
        token = strtok(NULL, " \t"); //get next token
        if(token == NULL) {
            args[i+1] = NULL;
            break;
        }
    }
}

void sigint_handler(int signal) {
    if (foreground_pid >0) {
        kill(foreground_pid, SIGINT);
        printf("\n");
    } 
    printf("\n");
    if (foreground_pid == 0) {
        printf("icsh $ ");
        fflush(stdout); //force output bugger to display prompt
    }
}

void sigtstp_handler(int signal) {
    if(foreground_pid > 0) {
        kill(foreground_pid, SIGTSTP);
        printf("\n");
    } else {
        printf("\n");
        printf("icsh $ ");
        fflush(stdout);
    }
}

//run external programs (unix fork/exec/wait)
int process_external(char **args, int is_background) {

    char *input_file = NULL;
    char *output_file = NULL;
    check_io_redirection(args, &input_file, &output_file);

    pid_t pid = fork();

    if (pid==0) { //child process
        
	if (is_background) setpgid(0,0);
        // reset SIGINT and SIGTSTP to default 
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        if (input_file != NULL) {
            int fd = open(input_file, O_RDONLY);
            if (fd == -1) { //file couldn't be open
                perror("icsh: input redirection");
                exit(1);
            }
            dup2(fd, 0); //redirect stdin to the opened file
            close(fd);
        }

        if (output_file != NULL) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) { //error opening file
                perror("icsh: output redirection");
                exit(1);
            }
            dup2(fd,1);
            close(fd);
        }

        if (execvp(args[0], args)==-1) { //replace child with external program
            perror("icsh");
            exit(1);
        }

    } else if (pid >0) { //parent process
        
        if (is_background) {
            char command_str[MAX_CMD_BUFFER];
            command_str[0] = '\0'; //initialize empty string

            for (int i = 0; args[i] != NULL; i++) {
                strcat(command_str, args[i]); //append argument to command 
                strcat(command_str, " ");
            }
            strcat(command_str, "&");
            add_background_job(pid, command_str);
        } else {
            foreground_pid = pid;
            int status;
            waitpid(pid, &status, WUNTRACED); //waiting for child to complete
            if (WIFSTOPPED(status)) { // if process was stopped
                char command_str[MAX_CMD_BUFFER];
                command_str[0] = '\0';

                for (int i = 0; args[i] != NULL; i++) {
                    strcat(command_str, args[i]);
                    strcat(command_str, " ");
                }
                add_background_job(pid, command_str);
                jobs[job_count-1].status = 2; //stopped status
                printf("[%d]+ stopped    %s\n", jobs[job_count-1].job_id, command_str);
                last_status = 128 + WSTOPSIG(status);
            } else {
                //exit status for ?
                if(WIFEXITED(status)) { //process exited normally
                    last_status = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) { //process killed by a signal
                    last_status = 128 + WTERMSIG(status);
                }
            }

            foreground_pid = 0; //clear foreground PID when process finished/stopped
    }
        return 0;
    } else {
        perror("icsh: fork failed");
        return -1;
    }
    return 0;
}

//process individual commands
int command_process(char *buffer, char *last_command, int file_indicator) {
    if (strlen(buffer) == 0) return 0;

    // !!
    if (strcmp(buffer, "!!") == 0) {
        if (strlen(last_command) == 0) return 0; //if no command pevious exists
        if (file_indicator) printf("%s\n", last_command);
        strcpy(buffer, last_command);
    } else {
        strcpy(last_command, buffer); //store curr command as last command for !!
    	if (file_indicator) commands_run++; //add to counter
    }

    //milestone 7
    if (strcmp(buffer, "yeet") == 0) {
	    if (file_indicator) printf("Bye bestie!✨\n");
	    exit(0); //exit shell
    }

    if (strcmp(buffer, "stats") == 0) {
	    time_t now = time(NULL);
	    int minutes = (now - session_start)/60; //minutes
	    int seconds = (now - session_start)%60; //remaining seconds
	    printf("Session stats:\n");
	    printf("Time: %d minutes, %d seconds\n", minutes, seconds);
	    printf("Number of commands run: %d\n", commands_run);
	    if (commands_run < 5) {
		    printf("Nice!👍\n");
	    } else if (commands_run < 10) {
		    printf("You're productive!\n");
	    } else {
		    printf("Dayumn, that's a lot!👀\n");
	    }

	    last_status = 0;
	    return 0;
    }

    if (strncmp(buffer, "echo ", 5) == 0) { //command starts with echo
        if (strcmp(buffer + 5, "$?")==0) {
            printf("%d\n", last_status);
        } else {
            printf("%s\n", buffer + 5);
        }
        last_status =0;
    } else if (strcmp(buffer, "jobs") == 0) {
        print_jobs(); //printing all jobs
        last_status = 0;
    } else if (strncmp(buffer, "fg ", 3) == 0) { //command if fg
        if (buffer[3] == '%') { //if job ID starts with %
            int job_id = atoi(buffer + 4); //convert to integer
            make_foreground(job_id);
        } else {
            printf("icsh: fg: job ID should start with %%\n");
            last_status = 1;
        }
    } else if (strncmp(buffer, "bg ", 3) == 0) {
    if (buffer[3] == '%') {
        int job_id = atoi(buffer + 4);
        send_background(job_id);
    } else {
        printf("icsh: bg: job ID must start with %%\n");
        last_status=1;
    }
    } else if (strncmp(buffer, "exit ", 5) == 0) { //command starts with exit
        int exit_code = atoi(buffer + 5) & 0xFF; //converting string to int
        if (file_indicator) printf("bye\n");
        exit(exit_code);
    } else if (strcmp(buffer, "exit") ==0) {
        if (file_indicator) printf("bye\n");
        exit(0);
    } else {
        char *args[24]; //arr to hold command arguments (24 max)
        char buffer_copy[MAX_CMD_BUFFER];
        strcpy(buffer_copy, buffer);
        parse_command(buffer_copy, args); //split command into arr
        if(args[0]!= NULL) {
            int is_bg = is_background(args);
            if (process_external(args, is_bg) ==-1){
                if(file_indicator) printf("bad command\n");
                last_status = 1;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER]; //store last command if !!
    FILE *input_file =stdin;
    int file_indicator = 1; //1 - interactive, 0- script file
    
    //signal handlers
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    if (argc ==2) { // only one argument
        input_file = fopen(argv[1], "r"); //open file to read
        if (input_file == NULL) { //can't open file
            fprintf(stderr, "Error: can't open file %s\n", argv[1]);
            return 1;
        }
        file_indicator = 0;
    }

    if (file_indicator) {
        printf("Welcome! Starting IC shell\n"); //welcome message
    	session_start = time(NULL);
    }


    while (1) {
        check_background_jobs(); //check for completed background jobs

            if (file_indicator) {
            printf("icsh $ ");
            fflush(stdout);
        }

        //end of file or error
        if(fgets(buffer, 255, input_file)==NULL) {
            break;
        }

        trim_str(buffer); //remove trailing newline
             
        command_process(buffer, last_command, file_indicator); // handling command
    }

    if (input_file != stdin) fclose(input_file); //closing file
                            
    return 0;
}
