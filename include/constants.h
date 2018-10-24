/*
 *Authors: Brandon Williams and Skyler Penna
 *File/Project: constants.h/CS 450 Problem Set 2 
 *Date: 10/23/2018
 *Description: This file defines a header class for 
 * 	       constants that are used in a simple 
 * 	       shell project. also defines a struct 
 * 	       that holds pertinent information
 * 	       for executing a command.   
*/
#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MAX_LINE_CHARS 1024
#define MAX_LINE_WORDS 100

#define INPUT_REDIRECT '<'
#define OUTPUT_REDIRECT '>'
#define PIPE '|'

struct exec_info{
	char* command_words[MAX_LINE_WORDS + 1];
    	int out_redirect, in_redirect;
	char out_file_name[MAX_LINE_CHARS];
	char in_file_name[MAX_LINE_CHARS];
};

#endif
