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

//void Execute(char** parsedLine, char** cmdList, int numPipes);
void Execute(char*** cmdList, int numPipes, int* size_of_commands);
void setup();
int pipeCount(char** parsedLine, int num_words, char** cmdList);
void syserror(const char *);


int main() {

	// Buffer for reading one line of input
	char line[MAX_LINE_CHARS];
	// holds separated words based on whitespace
	char* line_words[MAX_LINE_WORDS + 1];
	char* cmdList[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1]; // list of commands
	int pipes, cmds; //pipe counter, command counter
	char *singleCommand[MAX_LINE_WORDS +1];
	char *commands[MAX_LINE_WORDS+1];
	char cpy[MAX_LINE_CHARS];
	int size_of_command[MAX_LINE_WORDS+1];

	setup();
	// Loop until user hits Ctrl-D (end of input)
	// or some other input error occurs
	while( fgets(line, MAX_LINE_CHARS, stdin) ) {
		for(int i = 0; line[i] != '\0'; i++){
			if(line[i] == '\"'){
				line[i] = ' ';
			}
		}


		//int num_words = split_cmd_line(line, line_words);
		int num_cmds = split_line_at_pipes(line, commands);	

		//make a copy of commands
		//char * cmd_words[MAX_LINE_WORDS + 1][MAX_LINE_WORDS +1];
		char ** cmd_words[MAX_LINE_WORDS + 1];
		char temp[MAX_LINE_CHARS];

		
		for(int i = 0; i< num_cmds+1; i ++){
			cmd_words[i] = malloc(MAX_LINE_CHARS);
		}


		for(int i = 0; i < num_cmds; i++){
			commands[i] = RemoveSpaces(commands[i]);
			//RemoveSpaces(commands[i]);
			strcpy(temp, commands[i]);
			
			int size = split_cmd_line(temp, cmd_words[i]);
			size_of_command[i] = size;
		
			
			/*
			for(int j = 0; j < size; j++){
				printf("cmd_words %d = %s\n", i, cmd_words[i][j]);

			}
			*/
			
			//printf("cmd_word %d = %s\n", i, cmd_words[i]);
			//printf("command %d = %s\n", i, commands[i]);
			//printf("%s\n", line_words[i]);
		}

		Execute(cmd_words ,num_cmds, size_of_command);

	}
	return 0;
}

//void Execute(char** parsedLine, char** cmdList, int numPipes){	
void Execute(char*** cmdList, int numPipes, int* size_of_commands){
	int pfds[numPipes][2];
	int pfd[2];
	pid_t pid;
	//only 1 command
	if(numPipes == 1){	
		pid = fork();

		if(pid == 0){
			execvp(cmdList[0][0], cmdList[0]);
		}
		else{
			wait(NULL);
		}
	}
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
		if(pipe(pfd) == -1){
			syserror("Could not create a pipe\n");
		}
		for(int i = 0; i < numPipes+1; i++){
			if(i == 0){
				switch ( pid = fork() ) {
					case -1: 
						syserror("fork failed");
						break;
					case  0: 
						if (close(0) == -1)
							syserror("Could not close stdout");
						dup(pfd[0]);
						if (close(pfd[0]) == -1 || close(pfd[1]) == -1)
							syserror( "Could not close pfds from child" );
						execvp(cmdList[i][0], cmdList[i]);
						//execlp("wc", "wc", NULL);
						syserror("Could not exec");
						break;
					default:
						fprintf(stderr, "in i == numPipes The child's pid is: %d\n", pid);
						break;
				}
				//execute
			}
			else if(i == numPipes){
				//printf("I reached this\n");
				switch ( pid = fork() ) {
					case -1: 
						syserror("fork failed");
						break;
					case  0: 
						if (close(1) == -1)
							syserror("Could not close stdout");
						dup(pfd[1]);
						if (close(pfd[0]) == -1 || close(pfd[1]) == -1)
							syserror( "Could not close pfds from child" );
						execvp(cmdList[i][0], cmdList[i]);
						//execlp("who", "who", NULL);
						syserror("Could not exec");
						break;
					default:
						fprintf(stderr, "when i == 0 The child's pid is: %d\n", pid);
						break;
				}

				//read in from pipe, got to stdout
			}
			else{
				//read in from pipe and output to pipe
				//TODO: for multiple pipes
			}

		}
	}
}

void setup()
{
	printf("This is a sameple shell\npress ctrl d to exit\n");
}

int pipeCount(char** parsedLine, int num_words, char** cmdList){
	int pipes = 0, cmds = 0;
	for (int i=0; i < num_words; i++) {
		//printf("%s\n", line_words[i]);
		const char* tmp = parsedLine[i];
		if(strcmp(tmp, "|") != 0) { //check if word is not "|"
			cmdList[pipes][cmds] = parsedLine[i];
			cmds ++;
		} else {
			cmdList[pipes][cmds] = 0;
			cmds = 0;
			pipes ++;
		}
	}
	printf("pipes = %d\n", pipes);
	return pipes;
}

void syserror(const char *s){
	extern int errno;
	fprintf(stderr, "%s\n", s);
	fprintf(stderr, " (%s)\n", strerror(errno));
	exit(1);
}
