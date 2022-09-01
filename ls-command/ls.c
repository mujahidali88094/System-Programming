/* 
   ls command in linux

   NOTE: compile with -lm (i.e. Math library)
            -lm should be after fileName

*  Course: System Programming with Linux
*  lsv1.c
*  usage: ./a.out [-lR] [fileNames]
*  features included: longlisting, recursive search, colors, display in columns, etc

*/
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

char fileType;
char permissions[10] = {0};
int linkCount;
char owner[50] = {0}, group[50] = {0}, timeString[50] = {0}, fileName[100] = {0};
long fileSize;

int lines, cols;
int longListingOn = 0;
int recursiveSearchOn = 0;

extern int errno;

void color_print(char *file_name, char file_type, int s);
/* properly declared for compatibility with qsort */
static int cmp_str(const void *lhs, const void *rhs);
void printLongListing();
void updateWindowSize(int *lines, int *cols);
void do_ls(char *);
void set_stat_info(char *);
char getFileType(long);
void getPermissions(long mode, char *str);
void getOwnerName(int uid, char *owner);
void getGroupName(int gid, char *group);
void show(char **namesArray, int size);

int main(int argc, char *argv[])
{
   updateWindowSize(&lines, &cols);
   if (argc == 1)
   {
      printf("Directory listing of pwd:\n");
      do_ls(".");
   }
   else
   {
      int i = 0;
      while (++i < argc)
      {
         if (i == 1 && argv[i][0] == '-')
         { // options
            if (strstr(argv[i], "l"))
            {
               longListingOn = 1;
            }
            else
            {
               longListingOn = 0;
            }
            if (strstr(argv[i], "R"))
            {
               recursiveSearchOn = 1;
            }
            else
            {
               recursiveSearchOn = 0;
            }
            if (argc == 2)
            {
               do_ls(".");
            }
         }
         else
         {
            do_ls(argv[i]);
         }
      }
   }
   return 0;
}
void do_ls(char *dir)
{
   if (strcmp(dir, ".") == 0)
      printf("\nDirectory listing of pwd:\n");
   else
      printf("\nDirectory listing of %s:\n", dir);
   struct dirent *entry;
   DIR *dp = opendir(dir);
   chdir(dir);
   char *namesArray[10000];
   int i = 0;
   if (dp == NULL)
   {
      fprintf(stderr, "Cannot open directory:%s\n", dir);
      return;
   }
   errno = 0;
   while ((entry = readdir(dp)) != NULL)
   {
      if (entry == NULL && errno != 0)
      {
         perror("readdir failed");
         exit(1);
      }
      else
      {
         if (entry->d_name[0] == '.')
            continue;
      }
      namesArray[i++] = entry->d_name;
   }
   qsort(namesArray, i, sizeof(char *), cmp_str);
   int c = 0;
   char *dirsArray[10000];
   int dirsCounter = 0;
   for (; c < i; c++)
   {
      set_stat_info(namesArray[c]);
      if (fileType == 'd' && fileName != "." && fileName != "..")
      {
         dirsArray[dirsCounter++] = namesArray[c];
         // printf("dirsCount:%d\n",dirsCounter);
      }
      if (longListingOn == 1)
      {
         printLongListing();
      }
   }
   if (longListingOn == 0)
   {
      show(namesArray, i);
   }
   if (recursiveSearchOn == 1)
   {
      int x = 0;
      for (; x < dirsCounter; x++)
      {
         // printf("\033[1m x=%d, dirsCounter=%d, %s \033[0m\n",x,dirsCounter,dirsArray[x]);
         do_ls(dirsArray[x]);
      }
   }
   chdir("..");
   closedir(dp);
}
void set_stat_info(char *fname)
{
   struct stat info;
   int rv = lstat(fname, &info);
   if (rv == -1)
   {
      perror("stat failed");
      exit(1);
   }
   strcpy(fileName, fname);
   fileType = getFileType(info.st_mode);
   linkCount = info.st_nlink;
   fileSize = info.st_size;
   getPermissions(info.st_mode, permissions);
   getOwnerName(info.st_uid, owner);
   getGroupName(info.st_gid, group);
   // set formatted time in timeString
   struct tm *time_info;
   time_t current_time = info.st_mtime;
   time_info = localtime(&current_time);
   strftime(timeString, sizeof(timeString), "%H:%M %d %B", time_info);
   /*printf("mode: %o\n", info.st_mode);
   printf("link count: %ld\n", info.st_nlink);
   printf("user: %d\n", info.st_uid);
   printf("group: %d\n", info.st_gid);
   printf("size: %ld\n", info.st_size);
   printf("modtime: %ld\n", info.st_mtime);
   printf("name: %s\n", fname );*/
}
char getFileType(long mode)
{
   if ((mode & 0170000) == 0010000)
      return 'p';
   else if ((mode & 0170000) == 0020000)
      return 'c';
   else if ((mode & 0170000) == 0040000)
      return 'd';
   else if ((mode & 0170000) == 0060000)
      return 'b';
   else if ((mode & 0170000) == 0100000)
      return '-';
   else if ((mode & 0170000) == 0120000)
      return 'l';
   else if ((mode & 0170000) == 0140000)
      return 's';
}
void getPermissions(long mode, char *str)
{
   strcpy(str, "---------");
   // owner  permissions
   if ((mode & 0000400) == 0000400)
      str[0] = 'r';
   if ((mode & 0000200) == 0000200)
      str[1] = 'w';
   if ((mode & 0000100) == 0000100)
      str[2] = 'x';
   // group permissions
   if ((mode & 0000040) == 0000040)
      str[3] = 'r';
   if ((mode & 0000020) == 0000020)
      str[4] = 'w';
   if ((mode & 0000010) == 0000010)
      str[5] = 'x';
   // others  permissions
   if ((mode & 0000004) == 0000004)
      str[6] = 'r';
   if ((mode & 0000002) == 0000002)
      str[7] = 'w';
   if ((mode & 0000001) == 0000001)
      str[8] = 'x';
   // special  permissions
   if ((mode & 0004000) == 0004000)
      str[2] = 's';
   if ((mode & 0002000) == 0002000)
      str[5] = 's';
   if ((mode & 0001000) == 0001000)
      str[8] = 't';
}
void getOwnerName(int uid, char *owner)
{
   errno = 0;
   struct passwd *pwd = getpwuid(uid);
   if (pwd == NULL)
   {
      if (errno == 0)
         printf("Record not found in passwd file.\n");
      else
         perror("getpwuid failed");
   }
   else
      strcpy(owner, pwd->pw_name);
}
void getGroupName(int gid, char *group)
{
   struct group *grp = getgrgid(gid);
   errno = 0;
   if (grp == NULL)
   {
      if (errno == 0)
         printf("Record not found in /etc/group file.\n");
      else
         perror("getgrgid failed");
   }
   else
      strcpy(group, grp->gr_name);
}
void show(char **namesArray, int size)
{
   int fileNameMaxSize = 0;
   for (int i = 0; i < size; i++)
   {
      int temp = strlen(namesArray[i]);
      if (temp > fileNameMaxSize)
         fileNameMaxSize = temp;
   }
   int noOfCols = cols / (fileNameMaxSize + 2);
   int offset = ceil((double)size / noOfCols);
   for (int i = 0; i < offset; i++)
   {
      for (int j = i; j < size; j += offset)
      {
         set_stat_info(namesArray[j]);
         color_print(namesArray[j], fileType, (fileNameMaxSize + 2));
      }
      printf("\n");
   }
   printf("\n");
}
void color_print(char *file_name, char file_type, int s)
{
   switch (file_type)
   {
   case 'p':
      printf("%-*s", s, file_name);
      break;
   case 'c':
   case 'b':
      printf("\033[7m%-*s\033[m", s, file_name);
      break;
   case 'd':
      printf("\e[0;34m%-*s\033[0m", s, file_name);
      break;
   case 'l':
      printf("\033[95;6m%-*s\033[0m", s, file_name);
      break;
   case 's':
      printf("%-*s", s, file_name);
      break;
   case '-':
      if (strstr(file_name, ".out") || strstr(file_name, ".out"))
         printf("\e[0;32m%-*s\033[0m", s, file_name);
      else if (strstr(file_name, ".tar"))
         printf("\e[0;31m%-*s\033[0m", s, file_name);
      else
         printf("%-*s", s, file_name);
      break;
   }
}
static int cmp_str(const void *lhs, const void *rhs)
{
   const char *name1 = *(const char **)lhs;
   const char *name2 = *(const char **)rhs;
   return strcmp(name1, name2);
}
void printLongListing()
{
   printf("%c%s\t%d\t%s\t%s\t%ld\t%s\t",
         fileType, permissions, linkCount, owner, group, fileSize, timeString);
   color_print(fileName, fileType, strlen(fileName));
   printf("\n");
}
void updateWindowSize(int *lines, int *cols)
{
   struct winsize ws;
   ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
   (*lines) = ws.ws_row - 1; // Excluding one line for prompt
   (*cols) = ws.ws_col + 1;
}