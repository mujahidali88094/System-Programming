/* 
*  TODO: add Description
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "conio.h"

#define	LINELEN	512

int lines,cols;
long matchedFilePointerLocation;
void updateWindowSize(int *lines,int *cols){
	struct winsize ws;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
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
		//printf("fp %ld,curr %ld,count %d \n",ftell(fp),curr,count);
		if(ftell(fp) >= curr){
			return count;
		}
	}
}




void do_more(FILE *);
void printPrompt(long,long);
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
      do_more(stdin);
   }
   FILE * fp;
   while(++i < argc){
      if(argv[i][0] == '0' && argv[i][1] == '<'){
      	char* nameOfFile;
      	strcpy(nameOfFile,&argv[i][2]);
      	printf("%s\n",nameOfFile);
      	exit(1);
      }
      else{
      	fp = fopen(argv[i] , "r");
      }
      if (fp == NULL){
         perror("Can't open file");
         exit (1);
      }
      do_more(fp);
      fclose(fp);
   }  
   return 0;
}

void do_more(FILE *fp)
{
   long fileSize = getFileSize(fp);
   int num_of_lines = 0;
   int rv;
   char buffer[cols];
   FILE* fp_tty = fopen("/dev//tty", "r");
   while (fgets(buffer, cols, fp)){
      fputs(buffer, stdout);
      num_of_lines++;
      if (num_of_lines == lines){
      	 printPrompt(ftell(fp),fileSize);
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
             scanf("%[^\n]%*c",pattern);
             clearAboveLine();
             int n = findAndSetFilePointer(pattern, ftell(fp), "temp.txt"); //todo: remove hard-coded fileName
             //printf("%s found at line no: %d\n",pattern,n);
             if(n==-1){
             	printf("\033[7m Pattern Not Found \033[m");
             	goto GET_INPUT_LABEL;
             }else{
             	fseek(fp,matchedFilePointerLocation,SEEK_SET);
             	num_of_lines = 0;
             }
         }else if (rv == 4){ //user pressed v to open file in Vim
         	
         	char temp[100];
         	int loc = getLineNo(ftell(fp),"temp.txt") - (lines/2);
         	//printf("location t vim: %d ;",loc);
         	sprintf(temp,"vim +%d temp.txt",loc); //TODO: remove hard-coding
         	system(temp);
         	goto GET_INPUT_LABEL;
         }
      }
			char buffer[cols];
  }
  fclose(fp_tty);
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
	 if( c == 'v')
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