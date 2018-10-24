/*
 *Authors: Brandon Williams and Skyler Penna
 *File/Project: parsetools.c/CS 450 Problem Set 2 
 *Date: 10/23/2018
 *Description: this file holds the implementation for 
 *	       parsing a command to have it in a proper 
 *	       format to pass along to 'execvp' as well
 *	       as holding information about I/O redirection
 *	       if there is any. 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"
#include "parsetools.h"

/*
 *Function Name: split_cmd_line
 *Parameters: a char* (a string) called line and char** that stores the
 * 	      parsed version of the line 
 *Return Value: an integer for knowing the number of words in a split line
 *Description: the line will be parsed based on whitespace as the delimiter, 
 *	       The new parsed version will be stored in the parameter 
 *	       list_to_populate 
*/
int split_cmd_line(char* line, char** list_to_populate) {
	char* saveptr;  // for strtok_r; see http://linux.die.net/man/3/strtok_r
	char* delimiters = " \t\n"; // whitespace
	int i = 0;

	list_to_populate[0] = strtok_r(line, delimiters, &saveptr);

	while(list_to_populate[i] != NULL && i < MAX_LINE_WORDS - 1)  {
		list_to_populate[++i] = strtok_r(NULL, delimiters, &saveptr);
	};

	return i;
}

/*
 *Function Name: split_line_at_pipes
 *Parameters: a char* and a char**
 *Return Value: void
 *Description: This function works the exact same was as split_cmd_line, 
 *	       but whose delimiter is a pipe "|", rather than whitespace 
*/
int split_line_at_pipes(char* line, char** list_to_populate){
	char* saveptr;  // for strtok_r; see http://linux.die.net/man/3/strtok_r
	char* delimiters = "|";
	int i = 0;

	list_to_populate[0] = strtok_r(line, delimiters, &saveptr);
	while(list_to_populate[i] != NULL && i < MAX_LINE_WORDS - 1)  {
		list_to_populate[++i] = strtok_r(NULL, delimiters, &saveptr);
	};

	return i;
}

/*
 *Function Name: RemoveSpaces
 *Parameters: a char* 
 *Return Value: a char*
 *Description: This function removes any leading or trailing whitespace in a char *
*/
char* RemoveSpaces(char* source){
	char* end;

	//trim leading 0s
	while(*source == ' ') source++;
	
	if(*source == 0)
		return source;

	//trim trailing 0s
	end = source + strlen(source) - 1;
	while(end > source && *end == ' ') end--;

	//add a new null terminator
	end[1] = '\0';

	return source;
}

/*
 *Function Name: Parse
 *Parameters: a char ** that holds the split up commands, an exec_info* which is
 *	      a struct that holds information to execute a command. and finally the
 *	      total number of commands that we are parsing
 *Return Value: void
 *Description: This function is the main logic handler for parsing commands for 
 *	       information such as i/o redirection and putting the command into 
 *	       a format that is needed for the 'execvp' function. See the "Execute" 
 *	       function in main.c for the execution logic. 
*/
void Parse(char ** commands, struct exec_info* ParsedCommands, int numCommands){
	int size_of_cmd = 0;
	for(int i = 0; i< numCommands; i++){
		//allocating memory for my struct array
		for(int j = 0; j < MAX_LINE_WORDS +1; j++){
			ParsedCommands[i].command_words[j] = (char *)malloc(MAX_LINE_CHARS * sizeof(char));
		}
		ParsedCommands[i].out_redirect = 0;
		ParsedCommands[i].in_redirect = 0;
		//removing leading and trailing spaces, to make sure the command is in a formate
		//recognizable for the 'execvp' function
		commands[i] = RemoveSpaces(commands[i]);
		//for every command, split at white spaces and store in the parsedCommands struct array
		size_of_cmd = split_cmd_line(commands[i], ParsedCommands[i].command_words);

		for (int j = 0; j < size_of_cmd ; j++){
			if(!strcmp(ParsedCommands[i].command_words[j], ">")){//strcmp returns 0 if the two strings are equal
				//printf("we are in > or >> \n");	
				ParsedCommands[i].out_redirect = 1;

				strcpy(ParsedCommands[i].out_file_name, ParsedCommands[i].command_words[j+1]); 
				
				size_of_cmd -= 2;	
				ParsedCommands[i].command_words[j] = '\0';
				ParsedCommands[i].command_words[j+1] = '\0';
			} 
			else if(!strcmp(ParsedCommands[i].command_words[j], ">>")){
				ParsedCommands[i].out_redirect = 2;

				strcpy(ParsedCommands[i].out_file_name, ParsedCommands[i].command_words[j+1]); 
				
				size_of_cmd -= 2;	
				ParsedCommands[i].command_words[j] = '\0';
				ParsedCommands[i].command_words[j+1] = '\0';				
			}
			else if(!strcmp(ParsedCommands[i].command_words[j], "<")){
				ParsedCommands[i].in_redirect = 1;
				
				strcpy(ParsedCommands[i].in_file_name, ParsedCommands[i].command_words[j+1]); 
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
			}
		}
	}
}

/*
 * Function Name: removeQuotes
 * Parameters: char*
 * Return Type: void
 * Description: this function removes any \" or \' in a char*
 * 	        we use this function, because execvp does not 
 * 	        work with quotes in the string
*/
void removeQuotes(char * line){
	for(int i = 0; line[i] != '\0'; i++){
		if(line[i] == '\"' || line[i] == '\''){
			line[i] = ' ';
		}
	}
}




