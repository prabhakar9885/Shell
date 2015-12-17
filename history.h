#include "iostream"
#include "fstream"
#include "list"
#include "string"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"

#ifndef HIST_FILE_NAME
#define HIST_FILE_NAME ".MyHist"
#endif

using namespace std;

int histLength = 100;
list<string> histBuff;


char *strtrim( const char* text){

	char *out = (char*) malloc( strlen(text) );
	const char *str=text;
	size_t len=sizeof(text);
	const char *end;
	size_t out_size;

	if(len == 0)
		return  (char*)"";

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == '\0')  // All spaces?
    return  (char*)"";

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  end++;

  // Set output size to minimum of trimmed string length and buffer size minus 1
  out_size = (end - str) < len-1 ? (end - str) : len-1;

  // Copy trimmed string and add null terminator
  memcpy(out, str, out_size);
  out[out_size] = '\0';

  return out;
}


void addToHist(char *cmd){

	// Replace the \n with \0
	char temp[strlen(cmd)];
	strcpy(temp, cmd);
	int i = 0;
	while( temp[i] != '\n' )
		i++;
	temp[i] = '\0';

	// If the currnt command and the previous command are the same, don't insert into the history.
	if( histBuff.size()>0 && strcmp(strtrim(temp), histBuff.back().c_str() ) == 0 )
		return;

	string str(temp);

	if(  histBuff.size() == histLength ){
		histBuff.pop_front();
	}

	histBuff.push_back( str );
}


void displayHist(int countOfCommangs=0){

	int count = 0;

	int histBuffSize = histBuff.size();

	for ( list<string>::iterator i = histBuff.begin() ; i != histBuff.end() ; ++i ) {
		if( ++count > histLength-countOfCommangs )
			printf("%6d  %s\n", count, (*i).c_str() );
	}
}


void persistHistoryToDisk(){

	int fd = creat( (char*)".MyHist", 00666 );

	if (fd==-1)
	{
		printf("History file could not be saved.\n");
	}
	ftruncate(fd, 0);
	
	for ( list<string>::iterator i = histBuff.begin() ; i != histBuff.end() ; ++i ) {
		write(fd, (*i).c_str(), strlen( (*i).c_str() ) );
		write(fd, "\n", 1);
	}
	close(fd);

}

void loadHistoryFromDisk(){

	histBuff.clear();

	int fd = open( (char*)".MyHist", 00666 ), i = 0;
	if (fd==-1) {
		return;
	}
	char ch, str[1024];

	while( read( fd, &ch, 1 ) > 0 ){
		if(ch=='\n'){
			string s(str);
			histBuff.push_back( s );
			i = 0;
			bzero(str, 1024 * sizeof(char));
		}
		else
			str[ i++ ] = ch;
	}
	close(fd);
}


char* stringToCharPointer( string str ){

	char *chArr = (char*) malloc( ( str.size() + 1 ) * sizeof(char) );
	copy( str.begin(), str.end(), chArr );
	chArr[ str.size() ] = '\0';

	return chArr;
}

/*
 * returns the command present line number "position" in the history file
 */ 
string getEventAtPosition( int position ){
	int i = 1;
	for( list<string>::iterator it = histBuff.begin(); it != histBuff.end(); it++ ){
		if(i == position)
			return *it;
		else
			i++;
	}
	return (char*)"";
}


char* getBangCommandFromHist( char *bangCommand){


	if( bangCommand[1]=='!' ){

		char *targetCmd = stringToCharPointer( histBuff.back() );
		return targetCmd;
	}
	else if( bangCommand[1]=='-' ){
		// handles the commands of the form !-10abc

		// Overwrite the trailing \n with \0
		bangCommand[ strlen(bangCommand)-1 ] = '\0';

		// Find the location of the command in the history file.
		char serialNum[512];
		bzero(serialNum, 5);
		int indexOfBangCmd=2;
		while ( indexOfBangCmd<strlen(bangCommand) && bangCommand[indexOfBangCmd]>='0' && bangCommand[indexOfBangCmd]<='9' ){
			serialNum[indexOfBangCmd-2] = bangCommand[indexOfBangCmd];
			indexOfBangCmd++;
		}
		serialNum[ indexOfBangCmd ] = '\0';
		
		// Fetch the command at the location
		int serialNumInHist = histBuff.size() - atoi(serialNum) + 1;

		if( serialNumInHist < 0 ){
			char *errmg = (char*)malloc(512);
			strcpy( errmg, (char*)"!-" );
			strcat( errmg, serialNum );
			strcat( errmg, (char*)": event not found" );
			return errmg;
		}

		string temp =  getEventAtPosition( serialNumInHist );
		char *targetCmd = stringToCharPointer( temp );

		char *targetCmdWithArg = (char*)malloc( strlen(targetCmd)+ strlen(bangCommand) - indexOfBangCmd );
		strcpy( targetCmdWithArg, targetCmd );
		int i = strlen(targetCmdWithArg);
		while( indexOfBangCmd < strlen(bangCommand) )
			targetCmdWithArg[ i++ ] = bangCommand[ indexOfBangCmd++ ];
		targetCmdWithArg[ i ] = '\0';

		return targetCmdWithArg;
	}
	else if( bangCommand[1]<='9' && bangCommand[1]>='0' ){
		// handles the commands of the form !10abc

		// Overwrite the trailing \n with \0
		bangCommand[ strlen(bangCommand)-1 ] = '\0';

		char serialNum[512];
		int indexOfBangCmd=1;
		while ( indexOfBangCmd<strlen(bangCommand) && bangCommand[indexOfBangCmd]>='0' && bangCommand[indexOfBangCmd]<='9' ){
			serialNum[indexOfBangCmd-1] = bangCommand[indexOfBangCmd];
			indexOfBangCmd++;
		}
		serialNum[ indexOfBangCmd ] = '\0';
		int serialNumInHist = atoi(serialNum);

		string temp =  getEventAtPosition( serialNumInHist );
		char *targetCmd = stringToCharPointer( temp );

		char *targetCmdWithArg = (char*)malloc( strlen(targetCmd)+ strlen(bangCommand) - indexOfBangCmd );
		strcpy( targetCmdWithArg, targetCmd );
		int i = strlen(targetCmdWithArg);
		while( indexOfBangCmd < strlen(bangCommand) )
			targetCmdWithArg[ i++ ] = bangCommand[ indexOfBangCmd++ ];
		targetCmdWithArg[ i ] = '\0';


		return targetCmdWithArg;
	}
	else if( ( bangCommand[1]>='a' && bangCommand[1]<='z' ) ||  ( bangCommand[1]>='A' && bangCommand[1]<='Z' ) ){
		// handles the commands of the form !-10abc

		int len = strlen( bangCommand + 1 ); // +1, for excludint the '!'' at the begining of bangCommand
		int i;
		char prefix[len];
		for ( i = 1; i < len; ++i) {
				prefix[i-1] = bangCommand[i];
		}
		// Overwrite the trailing '\n' with '\0'
		prefix[i-1] = '\0';

		for( list<string>::reverse_iterator rItr = histBuff.rbegin(); rItr != histBuff.rend(); rItr++ )
			if( strncmp((*rItr).c_str(), prefix, len-1 ) == 0 ){ 
				const char *temp = (*rItr).c_str();
				char *res = (char*)malloc( sizeof(temp));
				strcpy(res, temp);
				return res;
			}

	}

	return (char*)"";
}



