/* 
*  more_v9.c
*
	redirection problems solved like 
		-getting input from terminal instead of stdin
		-added global variable "redirected" to control program if input is redirected
		-show error if fileName isn't passed and redirection is not done
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "conio.h"

int lines,cols;
long matchedFilePointerLocation;
int redirected = 0;

void updateWindowSize(int *lines,int *cols){
	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	(*lines) = ws.ws_row - 1;  //Excluding one line for prompt
	(*cols) = ws.ws_col + 1;
}

int findAndSetFilePointer(char* pattern, long startPointer, char* fileName){
	FILE * fp;
	fp = fopen(fileName , "r");
	if (fp == NULL){
	 perror("Can't open file");
	 exit (1);
	}
	char buffer[cols];
	int counter = 0;
	int startPointerIsPassed = 0;
	
	while (fgets(buffer, cols, fp)){
		counter = counter+1;
		
		if(startPointerIsPassed == 1 && strstr(buffer,pattern))
			return counter;
			
		if(startPointerIsPassed == 0 && ftell(fp) >= startPointer)
			startPointerIsPassed = 1;
			
		matchedFilePointerLocation = ftell(fp);
	}
	return -1;
}
int moveTo(char* pattern, FILE* fp){
	char buffer[cols];
	int counter = 0;
	
	matchedFilePointerLocation = ftell(fp);
	
	while (fgets(buffer, cols, fp)){
	
		counter = counter+1;
		
		if(strstr(buffer,pattern)){
			fputs(buffer,stdout);
			return counter;
		}

	}
	return -1;
}
int getLineNo(long curr, char* fileName){
	
	FILE * fp;
	fp = fopen(fileName , "r");
	if (fp == NULL){
	 perror("Can't open file");
	 exit (1);
	}
	char buffer[cols];
	int count = 0;
	while (fgets(buffer, cols, fp)){
		count = count+1;
		if(ftell(fp) >= curr){
			return count;
		}
	}
}



void getFileName(FILE* stream,char* file_name){
	char proc_link[1024];

	/* Open a file, get the file descriptor. */
	int fd = fileno(stream);

	/* Read out the link to our file descriptor. */
	sprintf(proc_link, "/proc/self/fd/%d", fd);
	memset(file_name, 0, sizeof(file_name));
	readlink(proc_link, file_name, sizeof(file_name) - 1);

	/* Print the file_name. */
	//printf("%s\n", file_name);
	
}
void do_more(FILE *, char*);
void printPrompt(long,long);
void printSimplePrompt();
int  get_input(FILE*);
long  getFileSize(FILE*);
void clearCurrentLine(){
	printf("\033[2K \033[1G");
}
void clearAboveLine(){
	printf("\033[1A \033[2K \033[1G");
}

int main(int argc , char *argv[])
{
	updateWindowSize(&lines,&cols);

   int i=0;
   if (argc == 1){
   
   		char file_name[1024];
   		getFileName(stdin,file_name);
		if(strstr(file_name,"dev")){
			printf("more: bad usage\nNo Input\n");
			exit(0);
		}
		
		redirected = 1 ;	  
      	do_more(stdin,"");
   }
   FILE * fp;
   while(++i < argc){
      fp = fopen(argv[i] , "r");
      if (fp == NULL){
         perror("Can't open file");
         exit (1);
      }
      do_more(fp,argv[i]);
      fclose(fp);
   }  
   return 0;
}

void do_more(FILE *fp,char* fileName)
{
   long fileSize;
   if(!redirected) fileSize = getFileSize(fp);
   int num_of_lines = 0;
   int rv;
   char buffer[cols];
   FILE* fp_tty = fopen("/dev//tty", "r");
   while (fgets(buffer, cols, fp)){
      fputs(buffer, stdout);
      num_of_lines++;
      
      if (num_of_lines == lines){
      	 if(redirected) printSimplePrompt();
      	 else printPrompt(ftell(fp),fileSize);
      	 GET_INPUT_LABEL:
         rv = get_input(fp_tty);		
         if (rv == 0){//user pressed q
            clearCurrentLine();
            break;//
         }
         else if (rv == 1){//user pressed space bar
            clearCurrentLine();
            updateWindowSize(&lines,&cols);
            num_of_lines = 0;
         }
         else if (rv == 2){//user pressed return/enter
            clearCurrentLine();
            updateWindowSize(&lines,&cols);
		 	num_of_lines = lines-1; //show one more line
         }
         else if (rv == 3){ // user pressed '/' forward slash
         	 clearCurrentLine();
			 updateWindowSize(&lines,&cols);
         	 printf("/");
             char pattern[cols];
             fscanf(fp_tty,"%[^\n]%*c",pattern);
             clearAboveLine();
             int n;
             if(redirected) n = moveTo(pattern,fp);
             else n = findAndSetFilePointer(pattern, ftell(fp), fileName); 
             if(n==-1){
             	printf("\033[7m Pattern Not Found \033[m");
             	if(redirected){
             		printf("\n");
             		exit(0);
             	}
             	else goto GET_INPUT_LABEL;
             }else{
             	if(redirected){
             		num_of_lines = 1;
             	}
             	else{
             		fseek(fp,matchedFilePointerLocation,SEEK_SET);
             		num_of_lines = 0;
             	}
             }
         }else if (rv == 4){ //user pressed v to open file in Vim
         	
         	char temp[100];
         	int loc = getLineNo(ftell(fp),fileName) - (lines/2);
         	sprintf(temp,"vim +%d %s",loc,fileName);
         	//sprintf(temp,"vim %s",fileName);
         	system(temp); 
         	goto GET_INPUT_LABEL;
         }
      }
			char buffer[cols];
  }
  fclose(fp_tty);
}
void printSimplePrompt(){
	printf("\033[7m --More-- \033[m");
}
void printPrompt(long currLocation,long fileSize){
	int percentage = ((double)currLocation / fileSize)*100;
	printf("\033[7m --more--(%d%%) \033[m",percentage);
}
int get_input(FILE* cmdstream)
{
   int c;		
   while(1){
     c = getch(cmdstream);
     if(c == 'q')
	 	return 0;
     if ( c == ' ' )			
	 	return 1;
     if ( c == '\n' )	
	 	return 2;
	 if( c == '/')
	 	return 3;
	 if(!redirected && c == 'v')
	 	return 4;
   }
   return 0;
}
long getFileSize(FILE *fp){
	fseek(fp,0,SEEK_END);
	long size = ftell(fp);
	rewind(fp); //move file pointer to start
	return size;

}