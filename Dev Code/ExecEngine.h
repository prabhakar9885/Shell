#include <unistd.h>

#ifndef EXECENGINE3_H
#define EXECENGINE3_H

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
        	dup2(fd_in, 0); //change the input according to the old one 
          	if( i< pc.cmdCount-1)
          		dup2(p[1], 1);
          	close(p[0]);
          	char** sCmd = parseCommandWithArgs(pc, i);
			execvp(sCmd[0], sCmd );
          	exit(EXIT_FAILURE);
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
			if(i==0){
				dup2( saved_stdout, 1);
				dup2( saved_stdin, 0);
			}
			break;
		}
		else{
			 /* Child process */
			
			if( n>1 && i>0 && i<n ) {
				// duplicate STDIN  
				close(fd[i-1][1]);
				close(0);
				dup2( fd[i-1][0], 0 );
			}
			if( n>1 && i<n-1 ){
				// duplicate STDOUT
				close(fd[i][0]);
				close(1);
				dup2( fd[i][1], 1 );
			}
			else if(i==n-1){
				// restore STDOUT
				dup2( saved_stdout, 1);
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
