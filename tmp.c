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


int main() {

    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    // holds separated words based on whitespace
    char* line_words[MAX_LINE_WORDS + 1];
    int pipes, cmds; //pipe counter, command counter
    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    while( fgets(line, MAX_LINE_CHARS, stdin) ) {
        int num_words = split_cmd_line(line, line_words);
        pipes = 0, cmds = 0;
        char* cmdList[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1]; // list of commands
        for (int i=0; i < num_words; i++) {
            //printf("%s\n", line_words[i]);
            const char* tmp = line_words[i];
            if(strcmp(tmp, "|") != 0) { //check if word is not "|'
               cmdList[pipes][cmds] = line_words[i];
               cmds ++;
            } else {
                cmdList[pipes][cmds] = 0;
                cmds = 0;
                pipes ++;
            } 
        }
    }
    return 0;
}


