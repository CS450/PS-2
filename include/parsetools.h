/*
 *Authors: Brandon Williams and Skyler Penna
 *File/Project: parsetools.h/CS 450 Problem Set 2 
 *Date: 10/23/2018
 *Description: This file defines a header class for 
 * 	       function used for parsing.  
*/

#ifndef PARSETOOLS_H
#define PARSETOOLS_H

int split_cmd_line(char* line, char** list_to_populate); 
int split_line_at_pipes(char* line, char** list_to_populate);
char* RemoveSpaces(char* source);
void Parse(char ** commmands, struct exec_info * ParsedCommands, int numCommands);
void removeQuotes(char * line);
void Execute(struct exec_info * ParsedCommands, int numCommands);
#endif
