/***************

A custom shell utility written as an assignment in the course SP by Arif Butt.

compile: gcc shell.c -lreadline
run: ./a.out

Features included:
1. IO Redirection 
2. Pipes enabled
3. Multiple statements using ';'
4. Run in background using '&'
5. History maintained
6. Tab Completion for paths
7. Store and get variables
8. If Else Control structure
 

***************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "IRON~LAD:- "
#define FILE_NAME_LENGTH 100
#define HISTORY_FILE "myshellhistory.txt"

char * internal_commands[] = {
  "cd",
  "exit",
  "history",
  "!"
};
int internal_commands_count = 4;

/*********************************** defs **************************************/
char * read_cmd(char * , FILE * );
void continue_with_command(char * cmdline);
char ** tokenize(char * cmdline, char delimiter);
int is_redirected(char * start, char * file_name);
void replace_variables(char ** arglist);
char * conditional_structure(char ** arglist);
int is_internal(char * str);
int execute_internal(char * arglist[]);
int execute(char * arglist[], int another_command_exists);
/*************************************************************************/

int pipe_fds[2];
int read_from = 0;
char * prompt = PROMPT;

int main() {

  using_history();
  int rv;
  rv = read_history(HISTORY_FILE);
  if (rv != 0) {
    printf("Error opening history file: %i\n", rv);
  }

  char * cmdline;
  while ((cmdline = readline(prompt)) != NULL) {
    continue_with_command(cmdline);
    free(cmdline);
  }
  printf("\n");

  return 0;
}
void continue_with_command(char * cmdline) {

  char ** arglist1 = NULL; //will be parsed on semicolons
  char ** arglist2 = NULL; //will be parsed on pipes
  char ** arglist3 = NULL; //will be parsed on spaces
  
  if (strlen(cmdline) > 0) add_history(cmdline);
  
  if ((arglist1 = tokenize(cmdline, ';')) != NULL) {
    int i = 0;
    while (arglist1[i]) {
      if ((arglist2 = tokenize(arglist1[i], '|')) != NULL) {
        read_from = 0; //for first command it is stdin
        int j = 0;
        while (arglist2[j]) {
        
        /******************** store variable ****************************/
          if (strstr(arglist2[j], " = ")){
            char ** argv = NULL;
            argv = tokenize(arglist2[j], '=');
            setenv(argv[0], argv[1], 1);

            if (argv[0]) free(argv[0]);
            if (argv[1]) free(argv[1]);
            free(argv);

            free(arglist2[j]);
            j++;
            continue;
          }
          /*************************************************************************/

          int another_command_exists = (arglist2[j + 1] != NULL);
          
          if ((arglist3 = tokenize(arglist2[j], ' ')) != NULL) {
          
            replace_variables(arglist3);
            
            if (strcmp(arglist3[0], "if") == 0) {
              char * str = conditional_structure(arglist3);
              continue_with_command(str);
              exit(0);
            }

            execute(arglist3, another_command_exists);

            //free arglist3
            int k = 0;
            while (arglist3[k]) {
              free(arglist3[k++]);
            }
          }
          //free current arglist2[j] because we dont need it anymore
          free(arglist2[j]);
          j++;
        }
      }
      free(arglist1[i]);
      i++;
    }
    free(arglist1);
    free(arglist2);
    free(arglist3);
  }
}
int execute(char * arglist[], int another_command_exists) {

  if (is_internal(arglist[0]) == 0) {
    execute_internal(arglist);
    return 0;
  }
  //else

  //********** &-handling **************/
  int shouldRunInForeground = 1; //defaults to true
  
  //go to last word
  int i = 0;
  while (arglist[i] != NULL) {i = i + 1;}
  i = i - 1;
  //go to last character of last word
  int j = 0;
  while (arglist[i][j] != '\0') {j = j + 1;}
  j = j - 1;
  //check &
  if (arglist[i][j] == '&') {
    shouldRunInForeground = 0; //run in background
    arglist[i][j] = '\0';

  }
  //*****************************************/

  pipe(pipe_fds);

  int status;
  int cpid = fork();
  int x;
  switch (cpid) {
  case -1:
    perror("fork failed");
    exit(1);
  case 0:
    i = 0;
    while (arglist[i] != NULL) {

      if (!shouldRunInForeground) {
        char fileName[] = "/dev/null";
        int fd = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd == -1) {
          perror("Cannot open file:");
          exit(1);
        } else {
          dup2(fd, 1);
          close(fd);
        }
      }

      char * file_name = (char * ) malloc(sizeof(char) * FILE_NAME_LENGTH);
      memset(file_name, '\0', FILE_NAME_LENGTH);
      int rv = is_redirected(arglist[i], file_name);
      if (rv == 0) {
        int fd = open(file_name, O_RDONLY);
        if (fd == -1) {
          perror("Cannot open file:");
          exit(1);
        } else {
          dup2(fd, 0);
          close(fd);
        }

      } else if (rv == 1) {
        int fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd == -1) {
          perror("Cannot open file:");
          exit(1);
        } else {
          dup2(fd, 1);
          close(fd);
        }
      } else if (rv == 2) {
        int fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd == -1) {
          perror("Cannot open file:");
          exit(1);
        } else {
          dup2(fd, 2);
          close(fd);
        }
      }
      if (rv != -1) {
      // skip this word and move each word backward if redirected
        int j = i;
        while (arglist[j] != NULL) {
          arglist[j] = arglist[j + 1];
          j++;
        }
      } else {
        i = i + 1;
      }
      free(file_name);
    }

    dup2(read_from, 0);                               //read from read_from
    if (another_command_exists) dup2(pipe_fds[1], 1); //write to pipe
    close(pipe_fds[0]);

    execvp(arglist[0], arglist);
    perror("Command not found...");
    exit(1);
  default:
    if (shouldRunInForeground) {
      waitpid(cpid, & status, 0);
      int st = status >> 8;
      if (st != 0)
        printf("child exited with status %d \n", st);
    }
    close(pipe_fds[1]);
    read_from = pipe_fds[0];
    return 0;
  }
}
char ** tokenize(char * cmdline, char delimiter) {
  
  char ** arglist = (char ** ) malloc(sizeof(char * ) * (MAXARGS + 1));
  for (int j = 0; j < MAXARGS + 1; j++) {
    arglist[j] = (char * ) malloc(sizeof(char) * ARGLEN);
    bzero(arglist[j], ARGLEN);
  }
  if (cmdline[0] == '\0') //if user has entered nothing and pressed enter key
    return NULL;
  int argnum = 0; //slots used
  char * cp = cmdline; // pos in string
  char * start;
  int len;
  while ( * cp != '\0') {
    while ( * cp == ' ' || * cp == '\t' || * cp == delimiter) //skip leading spaces
      cp++;
    if ( * cp == '\0') break; //if nothing found after whitespace
    start = cp; //start of the word
    len = 1;
    //find the end of the word
    while ( * ++cp != '\0' && !( * cp == '\t' || * cp == delimiter))
      len++;
    char *temp = cp;
    while (* --temp == ' '){len--;} //skip trailing spaces
    strncpy(arglist[argnum], start, len);
    arglist[argnum][len] = '\0';
    argnum++;
  }
  arglist[argnum] = NULL;
  return arglist;
}

char * read_cmd(char * prompt, FILE * fp) {
  printf("%s", prompt);
  int c; //input character
  int pos = 0; //position of character in cmdline
  char * cmdline = (char * ) malloc(sizeof(char) * MAX_LEN);
  while ((c = getc(fp)) != EOF) {
    if (c == '\n')
      break;
    cmdline[pos++] = c;
  }
  //these two lines are added, in case user press ctrl+d to exit the shell
  if (c == EOF && pos == 0)
    return NULL;
  cmdline[pos] = '\0';
  
  return cmdline;
}

int is_internal(char * str) {
  int i = 0;
  for (; i < internal_commands_count; i++) {
    if (strcmp(str, internal_commands[i]) == 0) {
      return 0;
    }
  }
  return -1;
}

int execute_internal(char * arglist[]) {
  if (strcmp(arglist[0], "cd") == 0) {
    if (arglist[1] == 0) {
      printf("Destination not found\n");
    } else {
      int rv = chdir(arglist[1]);
      if (rv != 0) perror("Unable to change directory");
    }
    return 0;
  } 
  else if (strcmp(arglist[0], "exit") == 0) {
    int rv;
    rv = write_history(HISTORY_FILE);
    if (rv != 0) {
      printf("Error: %i\n", rv);
    }
    rv = history_truncate_file(HISTORY_FILE, 100);
    if (rv != 0) {
      printf("Error: %i\n", rv);
    }

    exit(0);
  } 
  else if (strcmp(arglist[0], "history") == 0) {
    for (int i = 1; i <= history_length; i++) {
      printf("%s\n", history_get(i) -> line);
    }
  } 
  else if (strcmp(arglist[0], "!") == 0) { //usage: "! [index]"
    int j = 0;
    char index_string[100];
    while (arglist[j + 1]) {
      index_string[j] = * (arglist[j + 1]);
      j++;
    }
    int index = atoi(index_string);
    index = history_length - index; //reverse order (Note: skips current command)
    if (index > history_length || index < 1)
      printf("Index out of range\n");
    else
      printf("%s\n", history_get(index) -> line);
    return 0;
  }
  return -1;

}
void replace_variables(char ** arglist) {
  for (int i = 0; arglist[i]; i++) {
    if (arglist[i][0] == '$') {
      char * val = getenv(arglist[i] + 1);
      if (val) {
        free(arglist[i]);
        arglist[i] = malloc(sizeof(strlen(val) + 1) * sizeof(char));
        strcpy(arglist[i], val);
      }
    }
  }

  return;
}
char * conditional_structure(char ** arglist) {

  char condition[1000] = {
    '\0'
  };
  char then_command[1000] = {
    '\0'
  };
  char else_command[1000] = {
    '\0'
  };
  char fi_command[1000] = {
    '\0'
  };
  char * str;

  for (int i = 1; arglist[i]; i++) {
    strcat(condition, arglist[i]);
  }
  //strcat(condition, "&");

  for (int i = 1; arglist[i]; i++)
    free(arglist[i]), arglist[i] = NULL;
  free(arglist);

  char * temp;
  temp = read_cmd("then> ", stdin);
  strcpy(then_command, temp);
  temp = read_cmd("else> ", stdin);
  strcpy(else_command, temp);
  printf("fi\n");

  int rv = system(condition);
  str = (rv == 0) ? then_command : else_command;

  return str;
}

//returns to be redirected file descriptor i.e. 0,1,2 or -1(if no redirection)
int is_redirected(char * start, char * file_name) {
  int len = 0;
  while (start[len] != '\0') {
    len += 1;
  } //calculate length
  if (len >= 3) {
    if ( * start == '0' && * (start + 1) == '<') {
      strncpy(file_name, start + 2, len - 2);
      return 0; //input redirected signal
    } else if ( * start == '1' && * (start + 1) == '>') {
      strncpy(file_name, start + 2, len - 2);
      return 1; //output redirected signal
    } else if ( * start == '2' && * (start + 1) == '>') {
      strncpy(file_name, start + 2, len - 2);
      return 2; //error redirected signal
    }
  }
  if (len >= 2) {
    if ( * start == '<') {
      strncpy(file_name, start + 1, len - 1);
      return 0; //input redirected signal
    } else if ( * start == '>') {
      strncpy(file_name, start + 1, len - 1);
      return 1; //output redirected signal
    }
  }
  return -1;
}
