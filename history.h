#include "iostream"
#include "fstream"
#include "list"
#include "string"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"

using namespace std;

int histLength=5;
list<string> histBuff;

void addToHist(char *cmd){

	// Replace the \n with \0
	char temp[strlen(cmd)];
	strcpy(temp, cmd);
	int i = 0;
	while( temp[i] != '\n' )
		i++;
	temp[i] = '\0';

	string str(temp);

	if(  histBuff.size() == histLength ){
		histBuff.pop_front();
	}

	histBuff.push_back( str );
}


void displayHist(){

	int count = 0;

	int histBuffSize = histBuff.size();

	for ( list<string>::iterator i = histBuff.begin() ; i != histBuff.end() ; ++i ) {
		cout << ++count << "  " << *i << endl;
	}
}


void persistHistoryToDisk(){

	ofstream out((char*)".MyHist");
	
	for ( list<string>::iterator i = histBuff.begin() ; i != histBuff.end() ; ++i ) {
		out << *i << endl;
	}
	out.close();

}

void loadHistoryFromDisk(){

	int fd = open( (char*)".MyHist", 00666 ), i = 0;
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
