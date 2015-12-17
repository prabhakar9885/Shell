#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/*
 * Returns the number of times the character "Key" is appearning in the string.
 */
int getCharCount( char *str, char key){
    int count = 0;
    while(*str){
        if(*str==key)
            count++;
        str++;
    }
    return count;
}



/*
 *  Accepts a command string containing pipes, tokenizes the string based on "|" character and 
 *  places each tokenized command in the command.cmds[] array.
 */
pipedCommand parsePipedCommand( char *pipedCmd) {

    pipedCommand cmd;

    int len = strlen( pipedCmd );
    int lenOfCmdBeforePipe = 0, indexOfCmdBeforePipe = 0, cmdCount = 0;

    for (int i = 0; i < len; ++i) {
        lenOfCmdBeforePipe = 0;
        for ( indexOfCmdBeforePipe = i; 
                indexOfCmdBeforePipe<len && pipedCmd[indexOfCmdBeforePipe] != '|' ;
                 ++indexOfCmdBeforePipe) 
            lenOfCmdBeforePipe++;

        cmd.cmds[cmdCount] = (char*) malloc( sizeof(char) * (lenOfCmdBeforePipe+1) );
        bzero(cmd.cmds[cmdCount], sizeof(char) * (lenOfCmdBeforePipe+1) );
        for (int j = i; j < i+lenOfCmdBeforePipe; ++j)
            cmd.cmds[cmdCount][j-i] = pipedCmd[j];

        cmd.cmds[lenOfCmdBeforePipe] = 0;
        //printf("+%s\n", cmd.cmds[cmdCount]);

        i += lenOfCmdBeforePipe;
        cmdCount++;
    }

    cmd.cmdCount = cmdCount;
    return cmd;
}



/*
 *  Returns a string array containing the tokens obtained by tokenizing 
 *  cmd.cmds[ commandIndex], based on " " delimitter(space delimitter)
 */
char** parseCommandWithArgs( pipedCommand cmd , int commandIndex ) {

    char **singleCmd = (char**)malloc(sizeof(char*)*32);
    char *rest = cmd.cmds[ commandIndex ], *token;
    int i = 0;

    while( token = strtok_r(rest, " ", &rest) ){
        /* Breaks the command into tokens. e.g., "ls -l"  -> ls, -l     */
        singleCmd[i] = (char*)malloc( sizeof(char) * 10 );
        if( token[ strlen(token) -1 ] == '\n' )
            token[ strlen(token) -1 ] = 0;
        strcpy(singleCmd[i++], token);
        //printf("++%s, %d\n", singleCmd[i-1], strlen( singleCmd[i-1] ) );
    }
    singleCmd[i++]= NULL;

    return singleCmd;
}
