#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int main(){
	char *path;

	path = (char*) "PATH";
	char *value = getenv(path);
	printf("%s\n", value? value : (char*)"Not Found\n");

	strcat( value, (char*)":./" );

	if( setenv(path, value, 1) == 0 ){
		value = getenv(path);
		printf("New val: %s\n", value? value : (char*)"Not Found\n");
	}
	else{
		perror("setenv");
	}


	path = (char*) "path";
	value = getenv(path);
	printf("%s\n", value? value : (char*)"Not Found\n");
	
	if( setenv(path, "Yo!!Yo!!", 1) == 0 ){
		value = getenv(path);
		printf("New val: %s\n", value? value : (char*)"Not Found\n");
	}
	else{
		perror("setenv");
	}

	return 0;
}