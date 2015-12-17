#include "stdio.h"
#include "stdlib.h"
#include <string>
#include "string.h"

using namespace std;

void exportVar(char* exportStr){
	
	int size = strlen(exportStr);
	char *cmd = (char*)malloc(size);
	strcpy(cmd, exportStr);

	char *keyValue = strtok(cmd, " ");

	while(keyValue){
		char *tempKeyValue = keyValue;
		char *key = (char*)malloc(strlen(tempKeyValue));
		bzero(key, strlen(tempKeyValue));
		char *value = (char*)malloc(strlen(tempKeyValue));
		bzero(value, strlen(tempKeyValue));

		char *tempKey = key;
		while( (*tempKeyValue != '\0' && *tempKeyValue != '\n') && *tempKeyValue != '=' ){
			*tempKey = *tempKeyValue;
			tempKey++, tempKeyValue++;
		}
		*tempKey = '\0';
		tempKeyValue++;

		char *tempValue = value;
		while( (*tempKeyValue != '\0' && *tempKeyValue != '\n') && *tempKeyValue != ' ' ){
			*tempValue = *tempKeyValue;
			tempValue++, tempKeyValue++;
		}
		*tempValue = '\0';
		tempKeyValue++;

		if( strlen(value) > 0 ){
			if( setenv(key, value, 1) != 0 ){
				printf("%s: Environment variable could not be set\n", key);
			}
		}

		keyValue = strtok(NULL, " ");
	}
}



const char* processEchoCommand( char *cmd){

	int size = strlen( cmd ), i = 0;
	string output, variable;

	while( i < size ){
		variable.clear();

		if( cmd[i] != '$' ){
			output.append( 1, cmd[i++]);
			continue;
		}
		i++;
		while( i < size && cmd[i] != ' ' && cmd[i] != '\0' && cmd[i] != '\n' )
			variable.append(1, cmd[i++]);
		variable.append(1, '\0');

		const char *val = (const char*) getenv(variable.c_str());
		
		if( val )
			output.append( val );

		output.append( 1, ' ' );
	}

	return output.c_str();
}