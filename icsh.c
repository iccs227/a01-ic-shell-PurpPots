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
int main() {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER]; //store last command if !!

    printf("Welcome! Starting IC shell\n"); //welcome message
    while (1) {
        printf("icsh $ ");
        fgets(buffer, 255, stdin);

	trim_str(buffer); //remove trailing newline
			 
	if(strlen(buffer) == 0) { continue;} //if no command, skip and prompt again

	// handling !!
	if(strcmp(buffer, "!!") == 0) {
		if(strlen(last_command) == 0) {continue;} //no last command
		printf("%s\n", last_command);
		strcpy(buffer, last_command);
	} else {
		strcpy(last_command, buffer);
	}

	if (strncmp(buffer, "echo ", 5) == 0) {
		printf("%s\n", buffer + 5);
	} else if(strncmp(buffer, "exit ", 5) ==0) {
			int exit_code = atoi(buffer + 5) % 256; //make sure to fit within 0-255
			printf("bye\n");
			return exit_code;
	} else {
		printf("bad command\n");
	}
    }
}
