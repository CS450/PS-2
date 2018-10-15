#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "constants.h"
#include "parsetools.h"

//int split_list_if_pipe();

int main() {

	// Buffer for reading one line of input
	char line[MAX_LINE_CHARS];
	// holds separated words based on whitespace
	char* line_words[MAX_LINE_WORDS + 1];

	//used for forking and checking the return status of a child process
	pid_t child_pid, wait_pid;
	int status;
	
	//for piping
	int pfd[2];

	//int id, status;

	// Loop until user hits Ctrl-D (end of input)
	// or some other input error occurs
	while( fgets(line, MAX_LINE_CHARS, stdin) ) 
	{	
		//changing any quotes in a line to white space. 
		for(int i = 0; line[i] != '\0'; i++){
			if(line[i] == '\"'){
				line[i] = ' ';
			}
			//TODO 
			if(line[i] == '|'){
				//split the line at this point because every thing up to the '|'
				//is our first command. 
			}
			if(line[i] == '>'){
				//check if line[i+1] is also '>'
				if(line[i+1] == '>'){
					//TODO implement the ">>"
				}
				else{//we are now at a file name and need to change the redirection
					
					pipe(pfd);//piping so we can change stdout to the file

				}
				//split the line at this point because every thing up to the '|'
				//is our first command. 
			}
			if(line[i] == '<'){
				//split the line at this point because every thing up to the '|'
				//is our first command. 
			}
		}

		int num_words = split_cmd_line(line, line_words);
		//for every line entered, fork() to create a new process
		if(child_pid = fork() < 0){
			printf("error: forking failed\n");
		}


		if(child_pid == 0){
			//we are in the child process

			//execute the command line arguments
			if(execvp(line_words[0], line_words) < 0){
				printf("error: exec failed\n");
			}



		}

		//wait for the child process to finish executing
		else{
			while(wait(&status) > 0){
				if(WIFEXITED(status)){
					printf("childs exit code: %d\n", WEXITSTATUS(status));
				}
				else if (WIFSIGNALED(status)){
					psignal(WTERMSIG(status), "Exit signal");
				}
			}

		}

		//while((wait_pid = wait(&status)) > 0){


		//}






		/*testing out put
		  for (int i=0; i < num_words; i++) {
		  printf("%s\n", line_words[i]);
		  }
		  */
	}

	return 0;
}


