#include "stdio.h"
#include "stdlib.h"

int main(){
	char *path;

	path = (char*) "PATH";
	char *value = getenv(path);
	printf("%s\n", value? value : (char*)"Not Found\n");

	path = (char*) "path";
	value = getenv(path);
	printf("%s\n", value? value : (char*)"Not Found\n");
	
	return 0;
}