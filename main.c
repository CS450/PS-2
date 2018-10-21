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
		//the below struct is used to store every command in a parsed form
		struct exec_info parsedCommands[num_cmds];

		//allocating memory for my struct array
		for(int i = 0; i < num_cmds; i++){
			//for every parsedCommands[i] malloc its corresponding array
			for(int j = 0; j < MAX_LINE_WORDS +1; j++){
				parsedCommands[i].command_words[j] = (char *)malloc(MAX_LINE_CHARS * sizeof(char));
			}
		}

		int size_of_cmd = 0;
		for(int i = 0; i< num_cmds; i++){
			//removing leading and trailing spaces, since we only split at pipes
			commands[i] = RemoveSpaces(commands[i]);
			//for every command, split at white spaces and store in the parsedCommands struct array
			size_of_cmd = split_cmd_line(commands[i], parsedCommands[i].command_words);

			/*
			printf("command %d = ", i);
			for(int j = 0; j < size_of_cmd ; j++){
				printf("%s ", parsedCommands[i].command_words[j]);
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
	printf("This is a sameple shell\npress ctrl d to exit\n");
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

	//initialize the pipes
	for(int i = 0; i < numCommands-1; i++){
		pipe(pfds[i]);
	}
	//only 1 command
	if(numCommands == 1){	
		pid = fork();

		if(pid == 0){
			execvp(ParsedCommands[0].command_words[0], ParsedCommands[0].command_words);
		}
		else{
			wait(NULL);
		}
	}
	//we must have multiple commands with piping
	else{
		/*	
			pid_t *pids;
			pids = mmap(0, (pipes+1)*sizeof(pid_t), PROT_READ|PROT_WRITE				, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
			if(!pids){
			perror("mmap failed");
			exit(1);
			}
			memset((void *)pids, 0, (pipes+1)*sizeof(pid_t));
			*/
		for(int i = 0; i < numCommands; i++){
			//here we want to close stdin because we want our input to be coming from the pipe
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
						//execvp(cmdList[i][0], cmdList[i]);
						//printf("this is the command in i==1: %s\n", cmdList[i][0]);
						//printf("this is the args: %s", cmdList[i]);
						//execlp("who", "who", NULL);
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
						syserror("Could not exec");
						break;
					default:
						fprintf(stderr, "when i == 0 The child's pid is: %d\n", pid);
						break;
				}

			}
			//last command, our output should go to stdout, and input from the previous pipe
			else if(i == (numCommands-1)){
				switch ( pid = fork() ) {
					printf("inside first fork\n");
					case -1: 
						syserror("fork failed");
						break;
					case  0:
						/*
						if (close(0) == -1)
							syserror("Could not close stdout");
						*/
						//input coming from previous pipe
						dup2(pfds[i-1][0], 0);

					/*
					   if(dup2(pfd[0], 0) == -1){
					   perror("dup2");
					   exit(1);	
					   }
					   */

						if (close(pfds[i-1][0]) == -1 || close(pfds[i-1][1]) == -1)
							syserror( "Could not close pfds from last child" );
						//printf("this is the command in i==0: %s\n", cmdList[i][0]);
						//printf("this is the args: %s", cmdList[i]);
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
					//execlp("wc", "wc", NULL);
						syserror("Could not exec");
						break;
					default:
						fprintf(stderr, "in i == numCommands The child's pid is: %d\n", pid);
						break;
				}
				//read in from pipe, got to stdout
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
						/*
						if (close(0) == -1 || close(1) == -1)
							syserror("error with closing stdout or stdin");
						*/
							//dup(pfd[0]);
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
						//printf("this is the command in i==0: %s\n", cmdList[i][0]);
						//printf("this is the args: %s", cmdList[i]);
						execvp(ParsedCommands[i].command_words[0], ParsedCommands[i].command_words);
					//execlp("wc", "wc", NULL);
						syserror("Could not exec");
						break;
					default:
						fprintf(stderr, "in the middle command, The child's pid is: %d\n", pid);
						break;
				}
			}
		}
		
		for(int i = 0; i < numCommands-1; i++){
			if (close(pfds[i][0]) == -1 || close(pfds[i][1]) == -1)
				syserror( "Could not close pfds from parent" );
		}
		

		while(wait(NULL) != -1);
		//we are in the parent process here

	}

}
