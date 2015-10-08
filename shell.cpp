#include <stdio.h>
#include <list>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_COMMAND_SIZE 1024

using namespace std;


typedef struct {
    char* cmds[1024];
    int cmdCount;
    int exitStatus;
}pipedCommand;


typedef struct {
	list<char*> historyList;
	int historySize;
}shellDB;

shellDB sdb;


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
		/* Breaks the command into tokens. e.g., "ls -l"  -> ls, -l 	*/
		singleCmd[i] = (char*)malloc( sizeof(char) * 10 );
		if( token[ strlen(token) -1 ] == '\n' )
			token[ strlen(token) -1 ] = 0;
		strcpy(singleCmd[i++], token);
		//printf("++%s, %d\n", singleCmd[i-1], strlen( singleCmd[i-1] ) );
	}

	return singleCmd;
}



int execEngine( pipedCommand pc , int commandIndex, int previousPipe[] ){

	int currentPipe[2];

	if( pipe(currentPipe) != 0 ){
		perror("pipe error");
		return 1;
	}

	pid_t pid = fork();

	if( pid < 0){
		perror("fork error");
		exit(1);
	}

	if( pid == 0 ){
		printf("Child: %d: %s %d %d\n", commandIndex, pc.cmds[commandIndex], currentPipe[0], currentPipe[1] );

		if( pc.cmdCount > 1 && commandIndex < pc.cmdCount-1){
			/* Manage the stdout for multi-pipe */
			close(1);
			close(currentPipe[0]);
			dup2(currentPipe[1], 1);
		}
		if( pc.cmdCount > 1 && commandIndex > 0 && commandIndex < pc.cmdCount ){
			/* Manage the stdin for multi-pipe */
			close(0);
			close(previousPipe[1]);
			dup2(previousPipe[0], 0);
		}
		char** sCmd = parseCommandWithArgs(pc, commandIndex);
		printf("Child 2: %d: %s %s\n", commandIndex, sCmd[0], sCmd[1] );
	    int stat = execvp(sCmd[0], sCmd );
	}
	else{
		printf("Parent: %d: %s %d %d\n", commandIndex, pc.cmds[commandIndex], currentPipe[0], currentPipe[1] );
		if(commandIndex < pc.cmdCount-1)
			execEngine(pc, commandIndex + 1 , currentPipe);
		wait(0);
	}
	return 0;
}



char *strtrim( char* text){

	char *text2 = (char*) malloc( strlen(text) );
	strcpy( text2, text);

	while(*text2 ==' ') text2++;
	int len= strlen(text2);
	while(text2[len-1]==' '){
		text2[len-1]==0;
		len--;
	}
	return text2;
}


/*
 *  If the keyed-in command is shell's built-in, it will be executed.
 *  else, 0 will be returned
 */

int isBuiltinCmd( pipedCommand pCmd){
return 0;
	char** sCmd = parseCommandWithArgs(pCmd, 0);
	char *temp = strtrim(sCmd[0]);

	if( strcmp(temp, "cd") == 0 ){
		printf("cd..\n");
		chdir( sCmd[1] );
		return 1;
	}
	else if( strcmp(temp, "exit") == 0 ){

	}
	else if( strcmp(temp, "exit") == 0 ){
		exit(0);
	}
	else if( strcmp(temp, "history") == 0 ){
		int count = sdb.historyList.size();
		for(int i=0;i<count; i++){
			char* temp = sdb.historyList.back();
			sdb.historyList.pop_back();
			printf("%s", temp);
			sdb.historyList.push_front(temp);
		}
	}
	else
		return 0;
}


int main(){
    
    char buff[MAX_COMMAND_SIZE];
    sdb.historySize = 5;
    

    while(1){
    	
        bzero(buff, MAX_COMMAND_SIZE);
    	printf("%s: %s$ ", getlogin(), getcwd(buff, MAX_COMMAND_SIZE));
    	bzero(buff, MAX_COMMAND_SIZE);
    	fflush(stdout);
        read( 0, buff, MAX_COMMAND_SIZE );

        if(sdb.historyList.size() == sdb.historySize )
        	sdb.historyList.pop_back();
        sdb.historyList.push_front( buff );


       	pipedCommand pc = parsePipedCommand( buff );

        if( ! isBuiltinCmd( pc ) )
        	execEngine(pc, 0, NULL);
    }

    return 0;
}

