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

struct exec_info{
	char* command_words[MAX_LINE_WORDS + 1];
    	int out_redirect, in_redirect;
	char out_file_name[MAX_LINE_CHARS];
	char in_file_name[MAX_LINE_CHARS];
};

void Parse(char ** commmands, struct exec_info * ParsedCommands, int numCommands);
void Execute(struct exec_info * ParsedCommands, int numCommands);
void setup();
void syserror(const char *);
void removeQuotes(char * line);

int main() {

	// Buffer for reading one line of input
	char line[MAX_LINE_CHARS];
	//array to store multiple commands
	char *commands[MAX_LINE_WORDS+1];

	setup();
    
	// Loop until user hits Ctrl-D (end of input)
	// or some other input error occurs
	while( fgets(line, MAX_LINE_CHARS, stdin) ) {
		removeQuotes(line); //removes any quotes in the line, since exec will not work with them. 

		int num_cmds = split_line_at_pipes(line, commands);//splits the command line where there is a pipe and stores the (parsed) result in 'commands'
		
		struct exec_info parsedCommands[num_cmds];//this struct stores all of the commands in a parsed form. 
		
		Parse(commands, parsedCommands, num_cmds);//parse the commands and then execute
	}
	return 0;
}




void setup()
{
	printf("This is a simple shell\npress ctrl d to exit\n");
}



void syserror(const char *s){
	extern int errno;
	fprintf(stderr, "%s\n", s);
	fprintf(stderr, " (%s)\n", strerror(errno));
	exit(1);
}



void Parse(char ** commands, struct exec_info* ParsedCommands, int numCommands){
	int size_of_cmd = 0;
	for(int i = 0; i< numCommands; i++){
		//allocating memory for my struct array
		for(int j = 0; j < MAX_LINE_WORDS +1; j++){
			ParsedCommands[i].command_words[j] = (char *)malloc(MAX_LINE_CHARS * sizeof(char));
		}
		ParsedCommands[i].out_redirect = 0;
		ParsedCommands[i].in_redirect = 0;
		//removing leading and trailing spaces, since we only split at pipes
		commands[i] = RemoveSpaces(commands[i]);
		//for every command, split at white spaces and store in the parsedCommands struct array
		size_of_cmd = split_cmd_line(commands[i], ParsedCommands[i].command_words);

		for (int j = 0; j < size_of_cmd ; j++){
			//strcmp returns 0 if the two strings are equal
			if(!strcmp(ParsedCommands[i].command_words[j], ">")){
				//printf("we are in > or >> \n");	
				ParsedCommands[i].out_redirect = 1;

				//TODO: error check strcpy 
				strcpy(ParsedCommands[i].out_file_name, ParsedCommands[i].command_words[j+1]); 
				
				size_of_cmd -= 2;	
				ParsedCommands[i].command_words[j] = '\0';
				ParsedCommands[i].command_words[j+1] = '\0';
			} 
			else if(!strcmp(ParsedCommands[i].command_words[j], ">>")){
				ParsedCommands[i].out_redirect = 2;

				//TODO: error check strcpy 
				strcpy(ParsedCommands[i].out_file_name, ParsedCommands[i].command_words[j+1]); 
				
				size_of_cmd -= 2;	
				ParsedCommands[i].command_words[j] = '\0';
				ParsedCommands[i].command_words[j+1] = '\0';				
			}
			else if(!strcmp(ParsedCommands[i].command_words[j], "<")){
				ParsedCommands[i].in_redirect = 1;
				
				//TODO: error check strcpy 
				strcpy(ParsedCommands[i].in_file_name, ParsedCommands[i].command_words[j+1]); 
				//need to check for output too
				if((j+3) >= size_of_cmd){
					size_of_cmd -= 2;
					ParsedCommands[i].command_words[j] = '\0';
					ParsedCommands[i].command_words[j+1] = '\0';
					//break;
				}
				else{
					if(!strcmp(ParsedCommands[i].command_words[j+2], ">")){
						ParsedCommands[i].out_redirect = 1;
						strcpy(ParsedCommands[i].out_file_name, ParsedCommands[i].command_words[j+3]); 
						size_of_cmd -= 4;
						ParsedCommands[i].command_words[j] = '\0';
						ParsedCommands[i].command_words[j+1] = '\0';
						ParsedCommands[i].command_words[j+2] = '\0';
						ParsedCommands[i].command_words[j+3] = '\0';
					}
					else if(!strcmp(ParsedCommands[i].command_words[j+2], ">>")){
						ParsedCommands[i].out_redirect = 2;
						strcpy(ParsedCommands[i].out_file_name, ParsedCommands[i].command_words[j+3]); 
						size_of_cmd -= 4;
						ParsedCommands[i].command_words[j] = '\0';
						ParsedCommands[i].command_words[j+1] = '\0';
						ParsedCommands[i].command_words[j+2] = '\0';
						ParsedCommands[i].command_words[j+3] = '\0';
					}
				}
				//syserror("cannot have an io redirection without any file specified");
			}
		}
	}
	//will execute the command read in from the command line
	Execute(ParsedCommands, numCommands);
}


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

/*	
	printf("num cmds = %d\n", numCommands);
	printf("outredirect = %d, inredirect = %d\n", ParsedCommands[0].out_redirect, ParsedCommands[0].in_redirect);
	printf("out file name = %s\n", ParsedCommands[0].out_file_name);
	printf("in file name = %s\n", ParsedCommands[0].in_file_name);
*/	

	if(numCommands == 1){//only one command, but can have input and/or output redirection. 	
		pid = fork();

		if(pid == 0){
			//redireting output
			if(ParsedCommands[0].out_redirect || ParsedCommands[0].in_redirect){
				//save stdout?

				//TODO: error check on open
				if(ParsedCommands[0].out_redirect == 1){
					fflush(stdout);
					save_std_out = dup(1);
					pfds[0][1] = open(ParsedCommands[0].out_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0700);//0700 is for read, write, and execute permission
					if(pfds[0][1] < 0){
						syserror("could not open the output file");
					}
					dup2(pfds[0][1], 1);
				}
				else if(ParsedCommands[0].out_redirect == 2){
					fflush(stdout);
					save_std_out = dup(1);
					pfds[0][1] = open(ParsedCommands[0].out_file_name, O_WRONLY | O_CREAT |  O_APPEND, 0700);
					if(pfds[0][1] < 0){
						syserror("could not open the output file");
					}
					dup2(pfds[0][1], 1);
				}
				if(ParsedCommands[0].in_redirect == 1){
					fflush(stdin);
					save_std_in = dup(0);
					pfds[0][0] = open(ParsedCommands[0].in_file_name, O_RDONLY, 0400);//0400 is for read permission only
					if(pfds[0][0] < 0){
						syserror("could not open the input file");
					}
					dup2(pfds[0][0], 0);
				}
				/*
				if(ParsedCommands[0].out_redirect){
					dup2(pfds[0][1], 1);
				}
				*/
				printf("no\n");
				if (close(pfds[0][0]) == -1 || close(pfds[0][1]) == -1)
					syserror( "Could not close pfds from here" );
			}

			execvp(ParsedCommands[0].command_words[0], ParsedCommands[0].command_words);
		}
		else{
			if(ParsedCommands[0].out_redirect){
				//restore stdout
				fflush(stdout);
				dup2(save_std_out, 1);
				printf("no\n");
				close(save_std_out);
			}
			if(ParsedCommands[0].in_redirect){
				//restore stdin
				fflush(stdin);
				dup2(save_std_in, 0);
				close(save_std_in);
			}
			//while(wait(NULL) != -1);
			//wait(NULL);
		}
	}
	//we must have multiple commands with piping
	else{
		for(int i = 0; i < numCommands; i++){
			//printf("command # = %d\n", i);
			if(i == 0){//create the writer process (writing to the pipe)
				if(ParsedCommands[i].out_redirect){
					syserror("cannot redirect output when there is a following pipe");
				}
				//execute
				switch ( pid = fork() ) {
					//printf("inside seecond fork\n");
					case -1: 
						syserror("fork failed");
						break;
					case  0:
						/*
						if (close(1) == -1)
							syserror("Could not close stdout");
						*/
						dup2(pfds[i][1], 1);//changing output from stdout to the pipe
					
						if(ParsedCommands[i].in_redirect){//checking for input redirection
							fflush(stdin);
							save_std_in = dup(0);
							pfds[i][0] = open(ParsedCommands[0].in_file_name, O_RDONLY, 0400);//0400 is for read permission only
							dup2(pfds[i][0], 0);
						}
					/*	
						if(dup2(pfd[1], 1) == -1){
						perror("dup2");
						exit(1);
						}
						*/

						if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
							syserror( "Could not close pfds from first command child" );
						
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec the first command");
						break;
					default:
						//fprintf(stderr, "when i == 0 The child's pid is: %d\n", pid);
						break;
				}
				/*
				if(ParsedCommands[i].in_redirect){
					//restore stdin
					fflush(stdin);
					dup2(save_std_in, 0);
					close(save_std_in);
				}
				*/

			}
			//last command, our output should go to stdout, and input from the previous pipe
			else if(i == (numCommands-1)){
				if(ParsedCommands[i].in_redirect){
					syserror("cannot have an in redirection with a pipe coming before");
				}
                    		switch ( pid = fork() ) {
					//printf("inside first fork\n");
					case -1: 
						syserror("fork failed");
						break;
					case  0:
						//here we are getting input from pipe	
						dup2(pfds[i-1][0], 0);

						//if below is true we are sending the output to a file
						if(ParsedCommands[i].out_redirect){
							//save stdout?
							fflush(stdout);
							save_std_out = dup(1);
							//TODO: error check on open
							if(ParsedCommands[i].out_redirect == 1){
								pfds[i][1] = open(ParsedCommands[i].out_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
							}
							else if(ParsedCommands[i].out_redirect == 2){
								pfds[i][1] = open(ParsedCommands[i].out_file_name, O_WRONLY | O_APPEND, 0777);
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
						//fprintf(stderr, "in i == numCommands The child's pid is: %d\n", pid);
						break;
				}
				/*
				if(ParsedCommands[i].out_redirect){
				//restore stdout
				fflush(stdout);
				dup2(save_std_out, 1);
				close(save_std_out);
				}
				*/
			}
			else{
				//we are in a command surrounded by two pipes
				//read in from pipe and output to pipe
				//TODO: for multiple pipes
				switch ( pid = fork() ) {
					//printf("inside first fork\n");
					case -1: 
						syserror("fork failed");
						break;
					case  0:
						dup2(pfds[i-1][0], 0);
						dup2(pfds[i][1], 1);
						if (close(pfds[i-1][0]) == -1 || close(pfds[i-1][1]) == -1)
							syserror( "Could not close pfds from i-1  middle child" );
						/*
						if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
							syserror( "Could not close pfds from middle child" );
						*/
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec");
						break;
					default:
						//fprintf(stderr, "in the middle command, The child's pid is: %d\n", pid);
						break;
				}
			}
			/*
			*/
			//for(int i = 0; i < numCommands-1; i++){
			/*
			if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
				printf("i = %d\n", i);
				syserror( "Could not close pfds from parent" );
			*/

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
	       /*	
		for(int i = 0; i < numCommands; i++){
			if(close(pfds[i][0]) == -1){
				syserror("parent could not close stdin");
			}
			if(close(pfds[i][1]) == -1){
				printf("when i = %d\n", i);
				syserror("parent could not close stdout");
			}
		}
		*/
		
	}
}

void removeQuotes(char * line){
	for(int i = 0; line[i] != '\0'; i++){
		if(line[i] == '\"' || line[i] == '\''){
			line[i] = ' ';
		}
	}
}

