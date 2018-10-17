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

	//flag for checking if we need to redirect from stdout. 
	int redirect_out;

	//for piping
	int pfd[2];

	int new_out;
	int save_stdout; 
	//int id, status;
	//char* c;
	// Loop until user hits Ctrl-D (end of input)
	// or some other input error occurs
	while( fgets(line, MAX_LINE_CHARS, stdin) ) 
	{
		printf("Lobo Shell\n$ ");
		//printf(c);
		//changing any quotes in a line to white space. 
		for(int i = 0; line[i] != '\0'; i++){
			if(line[i] == '\"'){
				line[i] = ' ';
			}
		}
		int num_words = split_cmd_line(line, line_words);

		/**
		  for (int i=0; i < num_words; i++) {
		  printf("%s\n", line_words[i]);
		  }
		  */

		//handling output and input redirection
		for(int i = 0; i < num_words; i++){
			/*
			   if(*line_words[i] == '\"'){
			 *line_words[i] = ' ';
			 }
			 */
			//TODO 
			if(*line_words[i] == '|'){
				//split the line at this point because every thing up to the '|'
				//is our first command. 
			}
			if(*line_words[i] == '>'){
				//check if line[i+1] is also '>'
				if(*line_words[i+1] == '>'){
					//TODO implement the ">>"
				}
				else{//we are now at a file name and need to change the redirection

					//piping so we can change stdout to the file
					redirect_out = 1;
					if(new_out = open(line_words[i+1], O_WRONLY|O_CREAT
								|O_TRUNC, 0600) == -1){
						perror("Opening file");
						return 255;
					}
					//here we need to redirect stdout to our file
					else{
						printf("changing stdout\n");
					}
					/*
					   if(pipe(pfd) == -1){
					   printf("Error in piping");	
					   }
					   */
				}
				//split the line at this point because every thing up to the '|'
				//is our first command. 
			}
			if(*line_words[i] == '<'){
				//split the line at this point because every thing up to the '|'
				//is our first command. 
			}

		}
		//for every line entered, fork() to create a new process


		if(child_pid = fork() < 0){
			printf("error: forking failed\n");
		}


		if(child_pid == 0){
			//we are in the child process
			printf("in a child process\n");

			//redirecting in child?
			if(redirect_out == 1){
				save_stdout = dup(fileno(stdout));
				if(dup2(new_out, fileno(stdout)) == -1){
					perror("cannot redirect stdout"); 
					return 255;
				}
				//printf("yesssss\n");
			}



			//execute the command line arguments, input and output redirection should be handled. 
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

			//need to restore stdout
			if(redirect_out == 1){
				fflush(stdout);
				close(new_out);
				dup2(save_stdout, fileno(stdout));
				close(save_stdout);
				redirect_out = 0;
			}

			//while((wait_pid = wait(&status)) > 0){


			//}

			/*testing out put
			  for (int i=0; i < num_words; i++) {
			  printf("%s\n", line_words[i]);
			  }
			  */
		}
	}
		return 0;

}

