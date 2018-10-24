/*
 *Authors: Brandon Williams and Skyler Penna
 *File/Project: main.c/CS 450 Problem Set 2 
 *Date: 10/23/2018
 *Description: This is the main function for running a simple shell. 
 *             user defined objects are located in "constants.h" and
 *             tools to parse user input from the command line are 
 *             located in "parsetools.h"
*/

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

void Execute(struct exec_info * ParsedCommands, int numCommands);
void syserror(const char *);

int main() {

	char line[MAX_LINE_CHARS];// Buffer for reading one line of input
	char *commands[MAX_LINE_WORDS+1];//array to store multiple commands

	printf("Welcome to Brandon Williams and Skyler Penna's simple shell\npress ctrl d to exit\n>");
	while( fgets(line, MAX_LINE_CHARS, stdin) ) {//we believe this line will flush the stdin buffer on every loop
		removeQuotes(line); //removes any quotes in the line, since exec will not work with them. 

		int num_cmds = split_line_at_pipes(line, commands);//splits the command line where there is a pipe and stores the (parsed) result in 'commands'
		
		struct exec_info parsedCommands[num_cmds];//this struct stores all of the commands in a parsed form. 
		
		Parse(commands, parsedCommands, num_cmds);//parse the commands so we can pass to the Execute function
		
		Execute(parsedCommands, num_cmds);
		
		printf("\n>");
	}
	printf("\n");
	return 0;
}

/*
 *Function Name: syserror
 *Parameters: a string that helps describe the error that called the function
 *Return Value: void
 *Description: This function is used to output useful error messages to stderr
 * 	       in our project we use this to error check open(), close(), pipe(),
 * 	       fork(), and exec() function calls. 
*/
void syserror(const char *s){
	extern int errno;
	fprintf(stderr, "%s\n", s);
	fprintf(stderr, " (%s)\n", strerror(errno));
	exit(1);
}

/*
 *Function Name: Execute
 *Parameters: an exec_info struct (please see "constants.h" for a description
 * 	       and the number of commands, based on pipe count read in from the 
 * 	       user from the command line
 *Return Value: void
 *Description: This function is the main logic handler for executing command line
 * 	       arguments. It takes into account redirection and pipe operators
 * 	       such as <, >, >>, and |. This function also assumes the user will 
 * 	       enter valid Unix commands that are NOT Bash builtins. 
*/
void Execute(struct exec_info* ParsedCommands, int numCommands){
	int pfds[numCommands-1][2];
	pid_t pid;
	int save_std_out;
	int save_std_in;

	//initialize the pipes
	for(int i = 0; i < numCommands; i++){
		if(pipe(pfds[i]) == -1){
			syserror("could not create a pipe");
		}
	}

	if(numCommands == 1){//only one command, but can have input and/or output redirection. 	
		pid = fork();
		if(pid == -1){
			syserror("Could not fork the first command");
		}
		if(pid == 0){
			if(ParsedCommands[0].out_redirect || ParsedCommands[0].in_redirect){
				if(ParsedCommands[0].out_redirect == 1){
					pfds[0][1] = open(ParsedCommands[0].out_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0700);//0700 is for read, write, and execute permission
					if(pfds[0][1] < 0){
						syserror("could not open the output file");
					}
					dup2(pfds[0][1], 1);
				}
				else if(ParsedCommands[0].out_redirect == 2){
					pfds[0][1] = open(ParsedCommands[0].out_file_name, O_WRONLY | O_CREAT |  O_APPEND, 0700);
					if(pfds[0][1] < 0){
						syserror("could not open the output file");
					}
					dup2(pfds[0][1], 1);
				}
				if(ParsedCommands[0].in_redirect == 1){
					pfds[0][0] = open(ParsedCommands[0].in_file_name, O_RDONLY, 0400);//0400 is for read permission only
					if(pfds[0][0] < 0){
						syserror("could not open the input file");
					}
					dup2(pfds[0][0], 0);
				}
				printf("no\n");
				if (close(pfds[0][0]) == -1 || close(pfds[0][1]) == -1)
					syserror( "Could not close pfds from here" );
			}
			execvp(ParsedCommands[0].command_words[0], ParsedCommands[0].command_words);
			syserror("Could not exec the command");
		}
		else{
			if (close(pfds[0][0]) == -1 || close(pfds[0][1]) == -1)
				syserror( "Could not close pfds from here" );
			while(wait(NULL) != -1);
		}
	}
	else{//we must have multiple commands with piping
		for(int i = 0; i < numCommands; i++){
			if(i == 0){
				if(ParsedCommands[i].out_redirect){
					syserror("cannot redirect output when there is a following pipe");
				}
				switch ( pid = fork() ) {
					case -1: 
						syserror("first fork failed");
						break;
					case  0:
						dup2(pfds[i][1], 1);//changing output from stdout to the pipe
					
						if(ParsedCommands[i].in_redirect){//checking for input redirection
							pfds[i][0] = open(ParsedCommands[0].in_file_name, O_RDONLY, 0400);//0400 is for read permission only
							if(pfds[i][0] < 0){
								syserror("could not open the input file");
							}
							dup2(pfds[i][0], 0);
						}
						if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
							syserror( "Could not close pfds from first command child" );
						
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec the first command");
						break;
					default:
						break;
				}
			}
			//last command, our output should go to stdout, and input from the previous pipe
			else if(i == (numCommands-1)){
				if(ParsedCommands[i].in_redirect){
					syserror("cannot have an in redirection with a pipe coming before");
				}
                    		switch ( pid = fork() ) {
					case -1: 
						syserror("last fork failed");
						break;
					case  0:
						dup2(pfds[i-1][0], 0);//here we are getting input from pipe	

						if(ParsedCommands[i].out_redirect){
							if(ParsedCommands[i].out_redirect == 1){
								pfds[i][1] = open(ParsedCommands[i].out_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
								if(pfds[i][1] < 0){
									syserror("could not open the output file");
								}
							}
							else if(ParsedCommands[i].out_redirect == 2){
								pfds[i][1] = open(ParsedCommands[i].out_file_name, O_WRONLY | O_APPEND, 0777);
								if(pfds[i][1] < 0){
									syserror("could not open the output file");
								}
							}
							dup2(pfds[i][1], 1);
							if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
								syserror( "Could not close pfds from last child for pfd[i][0] or 1" );
						}

						if (close(pfds[i-1][0]) == -1 || close(pfds[i-1][1]) == -1)
							syserror( "Could not close pfds from last child i-1" );
						
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec the last command");
						break;
					default:
						break;
				}
			}
			else{
				//we are in a command surrounded by two pipes
				//read in from pipe and output to pipe
				switch ( pid = fork() ) {
					case -1: 
						syserror("middle fork failed");
						break;
					case  0:
						dup2(pfds[i-1][0], 0);
						dup2(pfds[i][1], 1);
						if (close(pfds[i-1][0]) == -1 || close(pfds[i-1][1]) == -1)
							syserror( "Could not close pfds from i-1  middle child" );
						
						if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
							syserror( "Could not close pfds from middle child" );
						
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec a middle command");
						break;
					default:
						break;
				}
			}
			//in parent
			if(i>0){
				if(close(pfds[i-1][0]) == -1){
					syserror("parent could not close stdin");
				}
				if(close(pfds[i-1][1]) == -1){
					syserror("parent could not close stdout");
				}
			}
			while(wait(NULL) != -1);
		}
	}
}



