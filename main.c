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
    int redirection;
    char file_name[MAX_LINE_CHARS];
};

void Execute(struct exec_info * ParsedCommands, int numCommands);
void setup();
void syserror(const char *);

int main() {

	// Buffer for reading one line of input
	char line[MAX_LINE_CHARS];
	//array to store multiple commands
	char *commands[MAX_LINE_WORDS+1];

	setup();
    
	// Loop until user hits Ctrl-D (end of input)
	// or some other input error occurs
	while( fgets(line, MAX_LINE_CHARS, stdin) ) {
		for(int i = 0; line[i] != '\0'; i++){
			if(line[i] == '\"'){
				line[i] = ' ';
			}
		}

		//this line will split the command line at pipes if any exist and store the result in 'commands'
		int num_cmds = split_line_at_pipes(line, commands);
		//printf("num_cmds = %d\n", num_cmds);	
		//the below struct is used to store every command in a parsed form
		struct exec_info parsedCommands[num_cmds];

		//allocating memory for my struct array
		for(int i = 0; i < num_cmds; i++){
			//for every parsedCommands[i] malloc its corresponding array
			for(int j = 0; j < MAX_LINE_WORDS +1; j++){
				parsedCommands[i].command_words[j] = (char *)malloc(MAX_LINE_CHARS * sizeof(char));
			}
			parsedCommands[i].redirection = 0;
		}

		int size_of_cmd = 0;
		for(int i = 0; i< num_cmds; i++){
			//removing leading and trailing spaces, since we only split at pipes
			commands[i] = RemoveSpaces(commands[i]);
			//for every command, split at white spaces and store in the parsedCommands struct array
			size_of_cmd = split_cmd_line(commands[i], parsedCommands[i].command_words);

	        	for (int j = 0; j < size_of_cmd ; j++){
				//strcmp returns 0 if the two strings are equal
				if(!strcmp(parsedCommands[i].command_words[j], ">")){
					//printf("we are in > or >> \n");	
					parsedCommands[i].redirection = 1;

					//TODO: error check strcpy 
					strcpy(parsedCommands[i].file_name,parsedCommands[i].command_words[j+1]); 
					
					size_of_cmd -= 2;	
					parsedCommands[i].command_words[j] = '\0';
					parsedCommands[i].command_words[j+1] = '\0';
				} 
				else if(!strcmp(parsedCommands[i].command_words[j], ">>")){
					parsedCommands[i].redirection = 2;

					//TODO: error check strcpy 
					strcpy(parsedCommands[i].file_name,parsedCommands[i].command_words[j+1]); 
					
					size_of_cmd -= 2;	
					parsedCommands[i].command_words[j] = '\0';
					parsedCommands[i].command_words[j+1] = '\0';
					
				}
				else if(!strcmp(parsedCommands[i].command_words[j], "<")){
                    			parsedCommands[i].redirection = 3;
					
					//TODO: error check strcpy 
					strcpy(parsedCommands[i].file_name,parsedCommands[i].command_words[j+1]); 
			
					size_of_cmd -= 2;
					parsedCommands[i].command_words[j] = '\0';
					parsedCommands[i].command_words[j+1] = '\0';
                		}
			}

				/*	
			//testing
			printf("redirection = %d\n", parsedCommands[i].redirection);
			if(parsedCommands[i].redirection){
				printf("file name = %s\n", parsedCommands[i].file_name);
			}
			printf("command =\n");
	        	for (int j = 0; j < size_of_cmd ; j++){
				//if(strcmp(parsedCommands[i].command_words[j], '\0'))
					//break;
				printf("%s\n", parsedCommands[i].command_words[j]);
				//if(parsedCommands[i].command_words[j][0] == '<')
				//if(!strcmp(parsedCommands[i].command_words[j], "<"))
				//	printf("%s ", parsedCommands[i].command_words[j]);
			}
			printf("\n");
			*/
				
        	}
		

		//will execute the command read in from the command line
		Execute(parsedCommands ,num_cmds);

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
void Execute(struct exec_info* ParsedCommands, int numCommands){
	int pfds[numCommands-1][2];
	pid_t pid;
	int save_stdout;
	//initialize the pipes
	for(int i = 0; i < numCommands; i++){
		pipe(pfds[i]);
	}
	printf("num cmds = %d\n", numCommands);
	
	//only 1 command
	if(numCommands == 1){	
		pid = fork();

		if(pid == 0){
			//redireting output
			if(ParsedCommands[0].redirection){
				//save stdout?
				fflush(stdout);
				save_stdout = dup(1);

				//TODO: error check on open
				if(ParsedCommands[0].redirection == 1){
					pfds[0][1] = open(ParsedCommands[0].file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
				}
				else if(ParsedCommands[0].redirection == 2){
					pfds[0][1] = open(ParsedCommands[0].file_name, O_WRONLY | O_APPEND, 0777);
				}
				dup2(pfds[0][1], 1);
				if (close(pfds[0][0]) == -1 || close(pfds[0][1]) == -1)
					syserror( "Could not close pfds" );
			}
			execvp(ParsedCommands[0].command_words[0], ParsedCommands[0].command_words);
		}
		else{
			if(ParsedCommands[0].redirection == 1){
				//restore stdout
				fflush(stdout);
				dup2(save_stdout, 1);
				close(save_stdout);
			}
			//wait(NULL);
		}
	}
	//we must have multiple commands with piping
	else{
		for(int i = 0; i < numCommands; i++){
			if(i == 0){//create the writer process (writing to the pipe)
				//execute
				switch ( pid = fork() ) {
					printf("inside seecond fork\n");
					case -1: 
						syserror("fork failed");
						break;
					case  0:
						if (close(1) == -1)
							syserror("Could not close stdout");
						dup2(pfds[i][1], 1);

					/*	
						if(dup2(pfd[1], 1) == -1){
						perror("dup2");
						exit(1);
						}
						*/

						if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
							syserror( "Could not close pfds from first command child" );
						
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec");
						break;
					default:
						//fprintf(stderr, "when i == 0 The child's pid is: %d\n", pid);
						break;
				}

			}
			//last command, our output should go to stdout, and input from the previous pipe
			else if(i == (numCommands-1)){
				//if(ParsedCommands[i].redirection == 0){
                    		switch ( pid = fork() ) {
					printf("inside first fork\n");
					case -1: 
						syserror("fork failed");
						break;
					case  0:
						//here we are getting input from pipe	
						dup2(pfds[i-1][0], 0);

						//if below is true we are sending the output to a file
						if(ParsedCommands[i].redirection){
							//save stdout?
							fflush(stdout);
							save_stdout = dup(1);
							//TODO: error check on open
							if(ParsedCommands[0].redirection == 1){
								pfds[i][1] = open(ParsedCommands[0].file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
							}
							else if(ParsedCommands[0].redirection == 2){
								pfds[i][1] = open(ParsedCommands[0].file_name, O_WRONLY | O_APPEND, 0777);
							}
							dup2(pfds[i][1], 1);
							if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
								syserror( "Could not close pfds from last child for pfd[i][0] or 1" );
						}

					        /*
					        if(dup2(pfd[0], 0) == -1){
					        perror("dup2");
					        exit(1);	
					        }
					        */

						if (close(pfds[i-1][0]) == -1 || close(pfds[i-1][1]) == -1)
							syserror( "Could not close pfds from last child i-1" );
						
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec");
						break;
					default:
						//fprintf(stderr, "in i == numCommands The child's pid is: %d\n", pid);
						break;
				}
			}
			//we are in a command surrounded by two pipes
			else{
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

					/*
					   if(dup2(pfd[0], 0) == -1){
					   perror("dup2");
					   exit(1);	
					   }
					   */

						if (close(pfds[i-1][0]) == -1 || close(pfds[i-1][1]) == -1)
							syserror( "Could not close pfds from middle child" );
						if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
							syserror( "Could not close pfds from middle child" );
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec");
						break;
					default:
						fprintf(stderr, "in the middle command, The child's pid is: %d\n", pid);
						break;
				}
			}
			if(ParsedCommands[0].redirection == 1){
				//restore stdout
				fflush(stdout);
				dup2(save_stdout, 1);
				close(save_stdout);
			}
		} 

		
		for(int i = 0; i < numCommands-1; i++){
			if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
				syserror( "Could not close pfds from parent" );
		}
		
		while(wait(NULL) != -1);
	}

}


