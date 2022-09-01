/* 
*  
*  morev6.c: 
		-added feature that it shows proper no of lines in case of window resize or zoom in/out
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "conio.h"

int lines,cols;
void updateWindowSize(int *lines,int *cols){
	struct winsize ws;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
	(*lines) = ws.ws_row - 1;  //Excluding one line for prompt
	(*cols) = ws.ws_col + 1;
}



//#define	LINELEN	512


void do_more(FILE *);
int  get_input(FILE*,int,int);
int  get_no_of_lines(FILE*);
void clearCurrentLine(){
	printf("\033[2K \033[1G");
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
   int total_no_lines = get_no_of_lines(fp);
   int num_of_lines = 0;
   int no_of_lines_has_been_displayed = 0;
   int rv;
   char buffer[cols];
   FILE* fp_tty = fopen("/dev//tty", "r");
   while (fgets(buffer, cols, fp)){
      fputs(buffer, stdout);
      num_of_lines++;
      no_of_lines_has_been_displayed++;
      if (num_of_lines == lines){
         rv = get_input(fp_tty,no_of_lines_has_been_displayed,total_no_lines);		
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
         else if (rv == 3){ //invalid character //disabled so far
            clearCurrentLine();
            updateWindowSize(&lines,&cols);
            break; 
         }
      }
      char buffer[cols];
  }
  fclose(fp_tty);
}

int get_input(FILE* cmdstream,int no_of_lines_has_been_displayed,int total_no_of_lines)
{
	//printf("in get_input");
   int percentage = ((double)no_of_lines_has_been_displayed / total_no_of_lines)*100;
   int c;		
   printf("\033[7m --more--(%d%%) \033[m",percentage);
   while(1){
     c = getch(cmdstream);
      if(c == 'q')
	 return 0;
      if ( c == ' ' )			
	 return 1;
      if ( c == '\n' )	
	 return 2;	
   }
   return 0;
}
int  get_no_of_lines(FILE *fp){
	rewind(fp); //move file pointer to start
	int count = 0;
	char c = getc(fp);
	while(c != EOF){
		if(c == '\n') ++count;
		c = getc(fp);
	}
	rewind(fp); //move file pointer to start
	return count;

}