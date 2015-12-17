#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "history.h"
#include "EnvVariables.h"


#define MAX_COMMAND_SIZE 1024

#ifndef HIST_FILE_NAME
#define HIST_FILE_NAME ".MyHist"
#endif


typedef struct {
    char* cmds[1024];
    int cmdCount;
    int exitStatus;
} pipedCommand;


typedef struct {
	char *left;
	char *right;
} redirectionPair;



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

int _getRedirectionPair( char *inp,  redirectionPair *rp, char delimitter){

    bzero( rp, sizeof(redirectionPair) );
    //printf("@@@ 1 %s\n", inp);
	
	int len = strlen(inp);
	rp->left = (char*)malloc( len );
	rp->right = (char*)malloc( len );
	char *temp = (char*)malloc( len );
	strcpy(temp, inp);

	int  i = 0;
	while( i<len && temp[i] != delimitter ) {
		rp->left[i] = temp[i];
		i++;
	}
	rp->left[i] = '\0';
    rp->left = strtrim(rp->left);
    //printf("@@@ 2 Left: %s Right: %s\n", rp->left, rp->right);

	if( i == len )
		return 0;

	int base = ++i; // ++ -> to skip th "<" operator
	while( i<len && temp[i]!='\n' ){
		rp->right[i-base] = temp[i];
		i++;
	}
	rp->right[i] = '\0';
	rp->right = strtrim(rp->right);
    //printf("@@@ 3 Left: %s Right: %s\n", rp->left, rp->right);

	return 1;
}

int hasInputRedirectiion( char *inp,  redirectionPair *rp){

    int ret = _getRedirectionPair(inp, rp, '<');
    int rightLen = strlen(rp->right);

    for (int i = 0; i < rightLen; ++i) {
        if( rp->right[i] == '>' ){
            rp->right[i] = '\0';
            break;
        }
    }
    rp->left = strtrim(rp->left);
    rp->right = strtrim(rp->right);

    //printf("----------------------------------------------------\n");
    //printf("Left: -%s- Right: -%s-\n", rp->left, strtrim(rp->right) );

    return ret;
}

int hasOutputRedirectiion( char *inp,  redirectionPair *rp){
	
    //printf("Output redirection\n");
	
    int ret = _getRedirectionPair(inp, rp, '>');
    int leftLen = strlen(rp->left);

    for (int i = 0; i < leftLen; ++i) {
        if( rp->left[i] == '<' ){
            rp->left[i] = '\0';
            break;
        }
    }
    rp->left = strtrim(rp->left);
    rp->right = strtrim(rp->right);

    //printf("Left: -%s- Right: -%s-\n", rp->left, rp->right);

    return ret;
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
        //printf("# %d\n", getpid() );
      	if ((pid = fork()) == -1)
      	{
        	exit(EXIT_FAILURE);
      	}
      	else if (pid == 0)
      	{
            //printf("## %d\n", getpid() );
            // If we have more than 1 command in pipe, dup the STDIN to the previous STDIN i.e., fd_in
    		if( pc.cmdCount > 1 ){
        		dup2(fd_in, 0); 
        	}

            // If the current comman is not the last command in the pipe, dup the STDOUT to the write-end of the pipe
          	if( i< pc.cmdCount-1 )
          		dup2(p[1], 1);
          	close(p[0]);

          	//	If Input-Redirection exist, then Handle it.
          	redirectionPair rp;
            //printf("###-1 %d\n", getpid() );
            int inpRedirectPid = -1;


            int fd_file = -1;
            char *currentCmd = (char*)malloc( strlen(pc.cmds[i]) );
            strcpy(currentCmd, pc.cmds[i]);

      		if( hasInputRedirectiion(currentCmd, &rp)  ){ 
                fd_file = open( rp.right, 0666 );
                //printf("###-2 %d %d\n", getpid() , fd_file);
                dup2(fd_file, 0);
                pc.cmds[i] = rp.left;
      		}
            if( hasOutputRedirectiion(currentCmd, &rp)  ){      
                int fd_file_ = open( rp.right, O_CREAT | O_WRONLY | O_TRUNC, 0644 );
                //printf("###-3 %d %d\n", getpid(), fd_file_ );
                dup2(fd_file_, 1);
                pc.cmds[i] = rp.left;
            }
          	
          	char **sCmd = parseCommandWithArgs(pc, i);
            
            //printf("Exec %d %d :%s: %s-%s\n", getpid(), inpRedirectPid, pc.cmds[i], sCmd[0], sCmd[1]);
          	
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
            //printf("##P %d\n", getpid() );
          	fd_in = p[0]; //save the input for the next command
        }
    }
}



/*
 *	All the single-quotes and double-quotes that are enclosing the "str" 
 *	are be removed and the resultant string will be returned.
 *	The "str" remaines uneffected.
 */
char* stripQuotes(const char *str){

	int len = strlen(str);
	char *res = (char*) malloc( len );
	strcpy(res, str);

	if(strlen(str) >0 && (str[0]=='\"' || str[0]=='\'') )
		res++;
	if(strlen(str) >0 && (str[len-1]=='\"' || str[len-1]=='\'') )
		res[strlen(res)-1] = '\0';

	return res;
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
		printf("Bye...\n");
		exit(0);
	}
	else if( strncmp(temp, "export", 6) == 0 ){
		exportVar(strtrim(pCmd.cmds[0]+6));
		return 1;
	}
	else if( strncmp(temp, "echo", 4) == 0 ){
		printf("%s", processEchoCommand( stripQuotes( strtrim(pCmd.cmds[0]+4) ) ) );
		return 1;
	}
	else if( strncmp(temp, "history", 7) == 0 ){

		// Processes the command of the form "history 3"
		char *bck = sCmd[1];
		if( bck != NULL){
			while( *bck>='0' && *bck<='9')
				bck++;
			
			if( *bck!='\0' && *bck!='\n' ){
				printf("%s : numeric argument required\n", temp);
				return 1;
			}
			if( *bck=='\0' || *bck=='\n' ){
				int countOfCommands = atoi(sCmd[1]);
				displayHist(countOfCommands);
				return 1;
			}
		}

		// Processes the command of the form "history | grep a"
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


/*
 *	If there is no child process running, then the handler displays the promt in a new line.
 *	else, the handler does nothing.
 */
void sig_handler(int signalno){

	pid_t result = waitpid(-1, NULL, WNOHANG);

	// If the child-process has changed the stat( to dead) 
	if( result != 0 ){
		char buff[MAX_COMMAND_SIZE];
		printf("\n%s: %s$ ", getlogin(), getcwd(buff, MAX_COMMAND_SIZE));
		fflush(stdout);
	}
}


int main(){
    
    char buff[MAX_COMMAND_SIZE];
    char* name = getlogin();

	// Register the signal handler with the kernal.    
  	if (signal(SIGINT, sig_handler) == SIG_ERR)
  		printf("Failed to handel the SIGINT signal\n");

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

        if( ! isBuiltinCmd( &pc ) ) {
        	persistHistoryToDisk();
        	execEngine3(pc);
        	loadHistoryFromDisk();
        }
    }

    return 0;
}