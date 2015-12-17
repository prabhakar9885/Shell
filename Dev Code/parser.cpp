#include <string.h>
#include <unistd.h>

#ifndef SHARED_DS
#include <shared_ds.cpp>
#endif

#ifndef PARSER
#define PARSER 1
#endif

/*
 * Returns the number of times the character "Key" is appearning in the string.
 */
int getCharCount( char *string, char key){
    int count = 0;
    while(*string){
        if(*string==key)
            count++;
        string++;
    }
    return count;
}



/*
 *  Accepts a command string containing pipes, tokenizes the string based on "|" character and 
 *  places each tokenized command in the command.cmds[] array.
 */
pipedCommand parsePipedCommand( char *pipedCmd) {

    pipedCommand cmd;
    cmd.cmdCount = 0;
    cmd.cmds[ cmd.cmdCount ] = strtok( pipedCmd, "|" );

    while( cmd.cmds[cmd.cmdCount] !=NULL )
        cmd.cmds[ ++cmd.cmdCount ] = strtok( NULL, "|" );

    /*
    for(int i=0; i< cmd.cmdCount; i++)
        printf("##%s-", cmd.cmds[i]);
        */

    return cmd;
}


/*
 *  Returns a string array containing the tokens obtained by tokenizing 
 *  cmd.cmds[ commandIndex], based on " " delimitter(space delimitter)
 */
singleCommand parseCommandWithArgs( pipedCommand cmd , int commandIndex ) {

    char *cmdString = cmd.cmds[ commandIndex ];
    int tokenCount = getCharCount(cmdString, ' ') +1;
    char *tokens[ tokenCount ];

    tokens[0] = strtok( cmdString, " ");

    for(int i=1; i< tokenCount; i++)
        tokens[i] = strtok( NULL, " " );

    singleCommand sc;
    sc.tokens = tokens;
    sc.tokenCount = tokenCount;

    /*
    for(int i=0; i< sc.tokenCount; i++){
        printf("@@%s", sc.tokens[i]);
        fflush(stdout);
    }*/

    return sc;
}

