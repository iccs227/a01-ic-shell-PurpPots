/* ICCS227: Project 1: icsh
 * Name: Alyza Cadelina
 * StudentID: 6480512
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CMD_BUFFER 255

void trim_str(char *str) { //removing trailing newline \n to \0
	size_t len = strlen(str);
	if (len > 0 && str[len-1] == '\n') str[len-1] = 0;
}

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
		printf("%s\n", buffer + 5);
	} else if (strncmp(buffer, "exit ", 5) == 0) { //command starts with exit
		int exit_code = atoi(buffer + 5) & 0xFF; //converting string to int
		if (file_indicator) printf("bye\n");
		exit(exit_code);
	} else {
		if (file_indicator) printf("bad command\n");
	}
	return 0;

}

int main(int argc, char *argv[]) {
	char buffer[MAX_CMD_BUFFER];
	char last_command[MAX_CMD_BUFFER]; //store last command if !!
	FILE *input_file =stdin;
	int file_indicator = 1; //1 - interactive, 0- script file
	

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
