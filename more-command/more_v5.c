/* 

*  Video Lecture 11

*  morev5.c: 

		-added percentage feature

		-stopped echoing of input character and avoiding of enter press

 */



#include <stdio.h>

#include <stdlib.h>

#include "conio.h"



#define	PAGELEN	18

#define	LINELEN	512



void do_more(FILE *);

int  get_input(FILE*,int,int);

int  get_no_of_lines(FILE*);

int main(int argc , char *argv[])

{

   int i=0;

   if (argc == 1){

      do_more(stdin);

   }

   FILE * fp;

   while(++i < argc){

      fp = fopen(argv[i] , "r");

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

   char buffer[LINELEN];

   FILE* fp_tty = fopen("/dev//tty", "r");

   while (fgets(buffer, LINELEN, fp)){

      fputs(buffer, stdout);

      num_of_lines++;

      no_of_lines_has_been_displayed++;

      if (num_of_lines == PAGELEN){

         rv = get_input(fp_tty,no_of_lines_has_been_displayed,total_no_lines);		

         if (rv == 0){//user pressed q

            printf("\033[2K \033[1G");

            break;

         }

         else if (rv == 1){//user pressed space bar

            printf("\033[2K \033[1G");

            num_of_lines -= PAGELEN;

         }

         else if (rv == 2){//user pressed return/enter

            printf("\033[2K \033[1G");

            num_of_lines -= 1; //show one more line

         }

         else if (rv == 3){ //invalid character //disabled so far

            printf("\033[2K \033[1G");

            break; 

         }

      }

  }

  fclose(fp_tty);

}



int get_input(FILE* cmdstream,int no_of_lines_has_been_displayed,int total_no_of_lines)

{

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