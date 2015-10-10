#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "history.h"

#define MAX_COMMAND_SIZE 1024

#ifndef HIST_FILE_NAME
#define HIST_FILE_NAME ".MyHist"
#endif


typedef struct {
    char* cmds[1024];
    int cmdCount;
    int exitStatus;
} pipedCommand;




void my_handler(int si){
	printf("Ctrl C is pressed\n");
}

void configSignaalHandler(){
	struct sigaction sigIntHandler;

   	sigIntHandler.sa_handler = my_handler;
   	sigemptyset(&sigIntHandler.sa_mask);
   	sigIntHandler.sa_flags = 0;

   	sigaction(SIGINT, &sigIntHandler, NULL);

   	pause();
}




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
    char *rest = (char*)malloc( strlen( cmd.cmds[commandIndex] ) * sizeof(char) ); 
    strcpy( rest, cmd.cmds[commandIndex] );
    
    char *token;
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



/*
 *	Runs the external commands.
 */
int execEngine3(pipedCommand pc){

  	int   p[2];
  	pid_t pid;
  	int   fd_in = 0;

  	for(int i=0; i<pc.cmdCount; i++)
  	{
  		pipe(p);
      	if ((pid = fork()) == -1)
      	{
        	exit(EXIT_FAILURE);
      	}
      	else if (pid == 0)
      	{
    		//configSignaalHandler();
    		if( pc.cmdCount > 1 ){
        		dup2(fd_in, 0); //change the input according to the old one 
        	}
          	if( i< pc.cmdCount-1 )
          		dup2(p[1], 1);
          	close(p[0]);
          	//printf("%s\n", pc.cmds[0]);
          	char **sCmd = parseCommandWithArgs(pc, i);
          	//printf("%s %s\n", sCmd[0], sCmd[1] );
			int status = execvp(sCmd[0], sCmd );
			if( status == -1){
				printf("%s: command not found\n", sCmd[0]);
          		exit(1);
			}
        }
      	else
      	{
        	wait(NULL);
          	close(p[1]);
          	fd_in = p[0]; //save the input for the next command
        }
    }
}


int execEngine2( pipedCommand pc){
	int n = pc.cmdCount;
	int saved_stdout = dup(1), saved_stdin = dup(0);
	int fd[n][2];
	
	for(int i=0; i<n; i++){
		
		if( pipe(fd[i]) != 0){
			perror("pipe");
			return 1;
		}
		
		pid_t pid = fork();
		
		if( pid < 0 ){
			perror("fork");
			return 2;
		}
		if( pid > 0){
			/* Parent process */
			wait(0);
			printf("#%d", i);
			close(fd[i-1][0]);
		}
		else{
			 /* Child process */
			
			if( n>1 && i>0 && i<n ) {
				// duplicate STDIN  
				//close(fd[i-1][1]);
				close(0);
				dup2( fd[i-1][0], 0 );
			}
			if( n>1 && i<n-1 ){
				// duplicate STDOUT
				//close(fd[i][0]);
				close(1);
				dup2( fd[i][1], 1 );
			}
			else if(i==n-1){
				// restore STDOUT
				//dup2( saved_stdout, 1);
			}
			
			char** sCmd = parseCommandWithArgs(pc, i);
			if( execvp(sCmd[0], sCmd ) == -1){
				perror("execvp");
				return 3;
			}
		}
	}
	return 0;
}


int saved_stdout;
int execEngine( pipedCommand pc , int commandIndex, int previousPipe[] ){

	if( commandIndex == pc.cmdCount )
		return 0;

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
			/* duplicate the stdout for multi-pipe */
			close(1);
			close(currentPipe[0]);
			dup2(currentPipe[1], 1);
		}
		if( commandIndex == pc.cmdCount-1){
			close(1);
			close(currentPipe[1]);
			dup2(saved_stdout, 1);
			printf("aaa\n");
		}
		if( pc.cmdCount > 1 && commandIndex > 0 && commandIndex < pc.cmdCount ){
			/* duplicate the stdin for multi-pipe */
			close(0);
			close(previousPipe[1]);
			dup2(previousPipe[0], 0);
		}
		
		char** sCmd = parseCommandWithArgs(pc, commandIndex);
		printf("Child II: %d: %s %s\n", commandIndex, sCmd[0], sCmd[1] );
	    int stat = execvp(sCmd[0], sCmd );
	}
	else{
		printf("Parent: %d: %s %d %d\n", commandIndex, pc.cmds[commandIndex], currentPipe[0], currentPipe[1] );
		if(commandIndex < pc.cmdCount-1)
			execEngine(pc, commandIndex + 1 , currentPipe);
		if(commandIndex==0)
			wait(0);
		printf("Parent %d+\n", commandIndex );
	}
	return 0;
}




/*
 *  If the keyed-in command is shell's built-in, it will be executed.
 *  else, 0 will be returned
 */

int isBuiltinCmd( pipedCommand *pCmd_ptr){

	pipedCommand pCmd = *pCmd_ptr;

	char** sCmd = parseCommandWithArgs(pCmd, 0);
	char *temp = strtrim(sCmd[0]);

	if( strcmp(temp, "cd") == 0 ){
		chdir( sCmd[1] );
		return 1;
	}
	else if( strcmp(temp, "exit") == 0 ){
		persistHistoryToDisk();
		exit(0);
	}
	else if( strncmp(temp, "echo", 4) == 0 ){
		
	}
	else if( strncmp(temp, "history", 7) == 0 ){
		pCmd_ptr->cmds[0] = (char*)malloc(sizeof(char)*128);
		bzero( pCmd_ptr->cmds[0], sizeof(char)*128 );
		strcpy( pCmd_ptr->cmds[0], (char*)"cat -n ");
		strcat( pCmd_ptr->cmds[0], (char*)".MyHist" );
		return 0;
		// displayHist();
	}
	else
		return 0;
}



int main(){
    
    char buff[MAX_COMMAND_SIZE];
    char* name = getlogin();
    
    saved_stdout = dup(1);

    loadHistoryFromDisk();

    while(1){
    	
        bzero(buff, MAX_COMMAND_SIZE);
    	printf("%s: %s$ ", name, getcwd(buff, MAX_COMMAND_SIZE));
    	bzero(buff, MAX_COMMAND_SIZE);
    	fflush(stdout);
        read( 0, buff, MAX_COMMAND_SIZE );

        // printf("11\n");
        if(  strlen(strtrim(buff))>0 &&  buff[0] != ' ' && buff[0] != '!' )
        	addToHist(buff);
        
        if( (strtrim(buff))[0] == '!' ){
        	strcpy(buff, getBangCommandFromHist( strtrim(buff) ) );
        	printf("%s\n", buff);
        	addToHist(buff);
        }

       	pipedCommand pc = parsePipedCommand( buff );

        if( ! isBuiltinCmd( &pc ) ){
        	persistHistoryToDisk();
        	execEngine3(pc);
        	loadHistoryFromDisk();
        	//loadHistoryFromDisk();
        	//execEngine2(pc);
        	//execEngine( pc , 0, NULL );
        }
    }

    return 0;
}