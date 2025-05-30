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

#define MAX_CMD_BUFFER 255

//global variables
pid_t foreground_pid = 0;
int last_status = 0; //exit status of last command - 0=success, non-0=error

//remove trailing newline \n to \0
void trim_str(char *str) {
	size_t len = strlen(str);
	if (len > 0 && str[len-1] == '\n') str[len-1] = 0;
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
	}
	printf("\n");
	if (foreground_pid == 0) {
		printf("icsh $ ");
		fflush(stdout);
	}
}

//run external programs (unix fork/exec/wait)
int process_external(char **args) {
	pid_t pid = fork();

	if (pid==0) { //child process
		
		// reset SIGINT and SIGTSTP to default 
		signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);

		if (execvp(args[0], args)==-1) { //replace child with external program
			perror("icsh");
			exit(1);
		}
	} else if (pid >0) { //parent process
		foreground_pid = pid;
		int status;
		waitpid(pid, &status, 0); //waiting for child to complete
		foreground_pid = 0; //clear when done
		
		//exit status for ?
		if(WIFEXITED(status)) { //process exited normally
			last_status = WEXITSTATUS(status);
		} else if (WIFSIGNALED(status)) { //process killed by a signal
			last_status = 128 + WTERMSIG(status);
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

	if (strcmp(buffer, "!!") == 0) {
		if (strlen(last_command) == 0) return 0; //if no command pevious exists
		if (file_indicator) printf("%s\n", last_command);
		strcpy(buffer, last_command);
	} else {
		strcpy(last_command, buffer);
	}

	if (strncmp(buffer, "echo ", 5) == 0) { //command starts with echo
		if (strcmp(buffer + 5, "$?")==0) {
			printf("%d\n", last_status);
		} else {
		printf("%s\n", buffer + 5);
		}
		last_status =0;
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
			if (process_external(args) ==-1){
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
	}


	while (1) {

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

