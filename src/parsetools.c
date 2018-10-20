#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "parsetools.h"



// Parse a command line into a list of words,
//    separated by whitespace.
// Returns the number of words
//
int split_cmd_line(char* line, char** list_to_populate) {
	char* saveptr;  // for strtok_r; see http://linux.die.net/man/3/strtok_r
	char* delimiters = " \t\n"; // whitespace
	int i = 0;

	list_to_populate[0] = strtok_r(line, delimiters, &saveptr);

	//printf("token = %s\n", list_to_populate[0]);

	while(list_to_populate[i] != NULL && i < MAX_LINE_WORDS - 1)  {
		list_to_populate[++i] = strtok_r(NULL, delimiters, &saveptr);
		//printf("token = %s\n", list_to_populate[i]);
	};

	return i;
}

//

int split_line_at_pipes(char* line, char** list_to_populate){
	char* saveptr;  // for strtok_r; see http://linux.die.net/man/3/strtok_r
	char* delimiters = "|";
	int i = 0;

	//token = checkDelimiters();

	list_to_populate[0] = strtok_r(line, delimiters, &saveptr);
	while(list_to_populate[i] != NULL && i < MAX_LINE_WORDS - 1)  {
		list_to_populate[++i] = strtok_r(NULL, delimiters, &saveptr);
	};

	return i;

}

char* RemoveSpaces(char* source){
	//char *i = source;
	//char *j = source;
	//int k = 0;
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
			


	/*
	while (*j != '\0'){
		*i = *j++;
		if(*i!= ' ' || k == 0 || *j+1 == '\0')
			i++;
			k++;
	}
	*i = '\0';
	*/
}



