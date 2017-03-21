/**
* KUSH shell interface program
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_LINE       80 /* 80 chars per line, per command, should be enough. */

// define pipe ends
#define READ_END 0
#define WRITE_END 1

int lastpid = -1;

int parseCommand(char inputBuffer[], char *args[],int *background);
void killChild(pid_t pid);
int checkForPipe(char *buffer[]);
int checkForArrow(char *buffer[]);
int checkForDoubleArrow(char *buffer[]);
void writeInto(int type, char buffer[], char name[]);
void whereiscall(int* pfdout, char* cmd);
void cd(char* args[]);
void trash(char* args[]);
void dizipub(char* args[]);
void schedInfo(char* args[]);

void sigquit( int signo )
{
  if(lastpid >= 0)
    system("sudo rmmod schedInfo");
  exit(0);
}

void init(void) {
  char kushdir[64], cmd[70];
  struct stat sb;

  printf("\e[H\e[2J");
  printf("Welcome to kush.\n");

  strcpy(kushdir, getenv("HOME"));
  strcat(kushdir, "/.kush");

  setenv("KUSHDIR",kushdir,1);

  if (stat(kushdir, &sb) == 0 && S_ISDIR(sb.st_mode))
  {
    // dir exists
  }
  else
  {
    /* Directory does not exist. */
    printf("kush directory does not exist.\n");
    strcpy(cmd,"mkdir ");
    strcat(cmd,kushdir);
    system(cmd);
    printf("kush directory created under home: %s\n", kushdir);
    strcpy(cmd,"cp schedInfo.c Makefile ");
    strcat(cmd,kushdir);
    system(cmd);
    chdir(kushdir);
    strcpy(cmd,"make");
    system(cmd);
    chdir(getenv("PWD"));
  }

  signal(SIGQUIT, sigquit);
  signal(SIGINT, sigquit);
}

int main(void)
{
  char inputBuffer[MAX_LINE]; 	        /* buffer to hold the command entered */
  int background;             	        /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/2 + 1],*args2[2],*args3[2]; /* command line (of 80) has max of 40 arguments */
  pid_t execchild, whereischild, whichchild, pipeChild;            		/* process id of the child process */
  int status;           		/* result from execv system call*/
  int shouldrun = 1;

  char *name[1];
  int i, upper, err, arrowCounter, pipeCounter;

  int pfd[2],pfd2[2],pfd3[2],pfd4[2];
  char outputbuffer[1024*1024], whereisbuffer[128], whichbuffer[128], pipebuffer[128],buff[128];
  int whereislength, commandLength, outLength, pipeLength;
  bool pipeChildIsAlive, belongsParent;



  init();

  while (shouldrun){            		/* Program terminates normally inside setup */
    background = 0;

    shouldrun = parseCommand(inputBuffer,args,&background);       /* get next command */


      if (strcmp(inputBuffer, "exit") == 0) {
            shouldrun = 0;     /* Exiting from kush*/
          } else if (strcmp(inputBuffer, "clear") == 0) {
            printf("\e[H\e[2J");
            continue;
          } else if (strcmp(inputBuffer, "cd") == 0) {
            cd(args);
            continue;
          } else if (strcmp(inputBuffer, "trash") == 0) {
            trash(args);
            continue;
          } else if (strcmp(inputBuffer, "dizipub") == 0) {
            dizipub(args);
            continue;
          } else if (strcmp(inputBuffer, "schedInfo") == 0) {
            schedInfo(args);
            continue;
          }

    if (shouldrun) {

      if (pipe(pfd) < 0 || pipe(pfd2) < 0 || pipe(pfd3) < 0 || pipe(pfd4) < 0) {
        printf("Failed to create pipe.\n");
        exit(-1);
      }

      if((pipeCounter = checkForPipe(args)) !=0){
        pipeChildIsAlive = true;
        if ((pipeChild = fork()) < 0) {
          printf("Failed to fork.\n");
          exit(-1);
        }
        //Devide args
        if(pipeChild == 0){
          belongsParent = false;
          args[pipeCounter] = NULL;
        } else {
          belongsParent= true;
          i = 0;
          pipeCounter++;
          while(args[pipeCounter]!=0){
            args[i] = args[pipeCounter];
            pipeCounter++;
            i++;
          }
          args[i] = NULL;
          args[i+1] = NULL;
        }
      } else {
        pipeChildIsAlive = false;
      }


      if ((execchild = fork()) < 0) {
        printf("Failed to fork.\n");
        exit(-1);
      }


      if (execchild == 0) {

        if(pipeChildIsAlive && belongsParent){
          close(pfd4[WRITE_END]);
          read(pfd4[READ_END], pipebuffer, sizeof(pipebuffer));
          dup2(pfd4[READ_END], STDIN_FILENO);
          close(pfd4[READ_END]);
        }

        if ((whichchild = fork()) < 0) {
          printf("Failed to fork.\n");
          exit(-1);
        }

        if(whichchild == 0){

          if ((whereischild = fork()) < 0) {
            printf("Failed to fork.\n");
            exit(-1);
          }

          if (whereischild == 0) {

            close(pfd3[READ_END]);

            args3[0] = "/usr/bin/whereis";
            args3[1] = args[0];

            args3[2] = NULL;
            args3[3] = NULL;

            dup2(pfd3[WRITE_END],fileno(stdout));  // send stdout to the pipe

            if (execv(args3[0], args3) == -1) {
              perror("whereis error\n");
            }

          } else {
            close(pfd2[READ_END]);
            close(pfd3[WRITE_END]);

            whereislength = read(pfd3[READ_END], whereisbuffer, sizeof(whereisbuffer));

            killChild(whereischild);

            dup2(pfd2[WRITE_END],fileno(stdout));
            wait();
            if (whereisbuffer[whereislength-2] == ':') {
              printf("!F\n");
              return 0;
            } else {
              args2[0] = "/usr/bin/which";
              args2[1] = args[0];
              if (execv(args2[0], args2) == -1) {
                perror("which error\n");
              }
            }
          }
        } else {

          close(pfd2[WRITE_END]);

          commandLength = read(pfd2[READ_END], whichbuffer, sizeof(whichbuffer));

          killChild(whichchild);


          for(i = 0; i < commandLength; i++) {
            if (whichbuffer[i] == '\n'){
              whichbuffer[i] = '\0';
              break;
            }
          }
          wait();


          if (whichbuffer[0] == '!') {
            printf("%s: command not found\n", args[0]);
          } else {

            if(pipeChildIsAlive && !belongsParent){
              args[0] = whichbuffer;
              close(pfd4[READ_END]);
              dup2(pfd4[WRITE_END], fileno(stdout));  // send stdout to the pipe
              close(pfd4[WRITE_END]);
              printf("This is a message\n");
              if (execv(args[0], args) == -1) {
                perror("Pipechild: exec error\n");
              }
            } else {
              args[0] = whichbuffer;

              if(!pipeChildIsAlive){
              close(pfd[READ_END]);
              dup2(pfd[WRITE_END],fileno(stdout));
              }

              if((arrowCounter = checkForArrow(args)) != 0){
                args[arrowCounter] = NULL;
                if (execv(args[0], args) == -1) {
                  perror("Single Arrow Error\n");
                }
              } else if((arrowCounter = checkForDoubleArrow(args)) != 0){
                args[arrowCounter] = NULL;
                if (execv(args[0], args) == -1) {
                  perror("Doule Arrow Error\n");
                }
              } else {
                if (execv(args[0], args) == -1) {
                  perror("exec error\n");
                }
              }
            }
          }
        }
        return 0;
      } else {
        if( background ) {
          printf("Process is in background. Parent keeps running...\n");
          close(pfd[READ_END]);
          close(pfd[WRITE_END]);
        } else {
          if(pipeChildIsAlive){
          close(pfd[WRITE_END]);
          close(pfd4[READ_END]);
          close(pfd4[WRITE_END]);
          waitpid(-1,&status,0);
          waitpid(0,&status,0);
        }
          outLength = read(pfd[READ_END], outputbuffer, sizeof(outputbuffer));
          killChild(execchild);
          outputbuffer[outLength-1] = '\0';

          pipeChildIsAlive = false;

          if (outLength > 0){

            if((arrowCounter = checkForArrow(args)) != 0){
              writeInto(0,outputbuffer, args[arrowCounter+1]);
            } else if((arrowCounter = checkForDoubleArrow(args)) != 0){
              writeInto(1,outputbuffer, args[arrowCounter+1]);
            } else {
              printf("%s\n",outputbuffer);
            }
          }
        }
      }
    }
  }
  sigquit(0);
  return 0;
}

void writeInto(int type, char buffer[], char name[]){

  if(type){
    FILE *f = fopen(name, "a");
    if (f == NULL){
      printf("Error opening file!\n");
      exit(1);
    }
    fprintf(f, "%s\n", buffer);
    fclose(f);
  } else {
    FILE *f = fopen(name, "w");
    if (f == NULL){
      printf("Error opening file!\n");
      exit(1);
    }
    fprintf(f, "%s\n", buffer);
    fclose(f);
  }
}

int checkForPipe(char *buffer[]){
  int i;
  for(i = 0; i < sizeof(buffer); i++){
    if(!buffer[i])
    return 0;
    if(strcmp(buffer[i],"|") == 0)
    return i;
  }
  return 0;
}


int checkForArrow(char *buffer[]){
  int i;
  for(i = 0; i < sizeof(buffer); i++){
    if(!buffer[i])
    return 0;
    if(strcmp(buffer[i],">") == 0)
    return i;
  }
  return 0;
}

int checkForDoubleArrow(char *buffer[]){
  int i;
  for(i = 0; i < sizeof(buffer) - 1; i++){
    if(!buffer[i])
    return 0;
    if(strcmp(buffer[i],">>") == 0)
    return i;
  }
  return 0;
}

void schedInfo(char* args[]) {
  char* cmd;
  int pid, pol, pri;

    if (args[3] != NULL) {
      pid = atoi(args[1]);
      pol = atoi(args[2]);
      pri = atoi(args[3]);
    } else {
      printf("schedInfo: please provide 3 arguments for pid, policy, priority.\n" );
      return;
    }

    if (lastpid == pid) {
      printf("schedInfo: module for pid=%d is already loaded.\n", pid);
    } else {
      if(lastpid >= 0)
      system("sudo rmmod schedInfo");
      cmd = malloc(256);
      sprintf(cmd, "sudo insmod %s/schedInfo.ko processID=%d processSPolicy=%d processPrio=%d", getenv("KUSHDIR"), pid, pol, pri);
      system(cmd);
      lastpid = pid;
    }

  return;
}

void dizipub(char* args[]) {
  FILE *file, *file2;
  char filename[128], line[256];
  char *name, *s, *e, *lastname, *lasts, *laste, url[128];
  pid_t child;
  int ie;

  name = malloc(128);
  lastname = malloc(128);
  s = malloc(2);
  e = malloc(2);
  lasts = malloc(2);
  laste = malloc(2);

  strcpy(filename,getenv("HOME"));
  strcat(filename,"/.kush/dizipublist");


  if (args[1] != NULL) {
    if (strcmp(args[1],"-l") == 0) {

      file = fopen(filename,"a");
      fclose(file);
      file = fopen(filename,"r");
      while(fgets(line, 256, file) != NULL)
      {
        if (strcmp(line,"") == 0) break;
        strcpy(name, strtok(line," "));
        strcpy(s,strtok(NULL," "));
        strcpy(e,strtok(NULL," "));
        if (args[2] == NULL || strcmp(args[2], name) == 0)
        printf("%s s%s e%s", name, s, e);
      }
      fclose(file);
    } else {
      if (args[2] == NULL) {
        printf("dizipub: Usage:\t'dizipub <series> <season> <episode>'\n\t\t'dizipub <series> next'\n");
        return;
      }
      if(strcmp(args[2],"next") == 0) {

        file = fopen(filename,"a");
        fclose(file);
        file = fopen(filename,"r");

        lastname[0] = '\0';
        while(fgets(line, 256, file) != NULL)
        {
          if (strcmp(line,"") == 0) break;
          strcpy(name, strtok(line," "));
          if (strcmp(args[1], name) == 0) {
            strcpy(s,strtok(NULL," "));
            strcpy(e,strtok(NULL," "));
            strcpy(lastname, name);
            strcpy(lasts, s);
            strcpy(laste, e);
          }
        }

        if (lastname[0] == '\0' || lastname == NULL) {
          printf("dizipub: next: Couldn't find a history of %s\n",args[1]);
          fclose(file);
          return;
        }

        fclose(file);

      } else {
        if (args[2] == NULL || args[3] == NULL) {
          printf("dizipub: Usage:\t'dizipub <series> [<season> <episode>]'\n\t\t'dizipub <series> [next]'\n");
          printf("dizipub: Please give both season and episode information for %s\n",args[1]);
          return;
        } else {
            strcpy(s, args[2]);
            strcpy(e, args[3]);
        }
      }
      strcpy(name,args[1]);
      if(strcmp(args[2],"next") == 0) {
        ie = atoi(laste);
        ie = ie+1;
        sprintf(e, "%d", ie);
      }
      sprintf(url, "http://dizipub.com/%s-%s-sezon-%s-bolum/", name, s, e);
      printf("Opening %s s%s e%s from %s...\n",name, s, e, url);
      file2 = fopen(filename,"a");
      fprintf(file2,"%s ", name);
      fprintf(file2,"%s ", s);
      fprintf(file2,"%s\n", e);
      fclose(file2);
      if ((child = fork()) < 0) {
        perror("Fork fails");
      }
      if(child ==0){
        execv("/usr/bin/firefox", (char*[]){"/usr/bin/firefox",url,NULL});
      }
    }
  } else {
    printf("dizipub: Usage:\t'dizipub <series> [<season> <episode>]'\n\t\t'dizipub <series> [next]'\n");
    return;
  }
  free(lastname);
}

void trash(char* args[]) {
  FILE *file, *file2;
  char filename[128], line[256], linecpy[256], *path, refill[2048];
  char *m, *h;
  m = malloc(2);
  h = malloc(2);
  path = malloc(128);
  pid_t child;

  if (args[1] != NULL) {
    if (strcmp(args[1],"-l") == 0) {
      strcpy(filename,getenv("HOME"));
      strcat(filename,"/.kush/crontabjobs");
      file = fopen(filename,"a");
      fclose(file);
      file = fopen(filename,"r");
      while(fgets(line, 256, file) != NULL)
      {
        if (strcmp(line,"") == 0) break;
        m = strtok(line," ");
        h = strtok(NULL," ");
        strtok(NULL," "); // day
        strtok(NULL," "); // month
        strtok(NULL," "); // year
        strtok(NULL," "); // rm
        strtok(NULL," "); // -rf
        path = strtok(NULL," "); //path
        strtok(NULL," "); // end

        printf("%s.%s %s",h,m,path);
      }
      fclose(file);
    } else if (strcmp(args[1],"-r") == 0) {
      strcpy(filename,getenv("HOME"));
      strcat(filename,"/.kush/crontabjobs");
      strcat(args[2],"/*\n");
      strcpy(refill,"");
      file = fopen(filename,"r");
      while(fgets(line, 256, file) != NULL)
      {
        if (strcmp(line,"") == 0) break;
        strcpy(linecpy,line);
        m = strtok(line," ");
        h = strtok(NULL," ");
        strtok(NULL," "); // day
        strtok(NULL," "); // month
        strtok(NULL," "); // year
        strtok(NULL," "); // rm
        strtok(NULL," "); // -rf
        path = strtok(NULL," "); //path
        strtok(NULL," "); // end

        if ((strcmp(path,args[2]) != 0)) {
          strcat(refill,m);
          strcat(refill," ");
          strcat(refill,h);
          strcat(refill," * * * rm -rf ");
          strcat(refill,path);
        } else {
          printf("%s is removed\n", path);
        }
      }
      fclose(file);
      //strcat(refill,'\0');d
      file2 = fopen(filename,"w");
      fprintf(file2, "%s", refill);
      fclose(file2);

      if ((child = fork()) < 0) {
        perror("Fork fails");
      }
      if (child == 0) {
        if (execv("/usr/bin/crontab", (char*[]){"/usr/bin/crontab","-r",NULL}) == -1) {
          perror("crontab\n");
        }
      } else {
        wait();
        if ((child = fork()) < 0) {
          perror("Fork fails");
        }
        if (child == 0) {
          if (execv("/usr/bin/crontab", (char*[]){"/usr/bin/crontab",filename,NULL}) == -1) {
            perror("crontab\n");
          }
        } else {
          wait();
        }
      }

    } else {
      strcpy(filename,getenv("HOME"));
      strcat(filename,"/.kush/crontabjobs");
      file = fopen(filename,"a");
      h = strtok(args[1],".");
      m = strtok(NULL,".");
      fprintf(file,"%s %s * * * rm -rf %s/*\n", m, h, args[2]);
      fclose(file);
      if ((child = fork()) < 0) {
        perror("Fork fails");
      }
      if (child == 0) {
        if (execv("/usr/bin/crontab", (char*[]){"/usr/bin/crontab",filename,NULL}) == -1) {
          perror("crontab\n");
        }
      } else {
        wait();
      }
    }
  }
}

void killChild(pid_t pid)
{
  kill(pid, SIGUSR1);
}

void whereiscall(int* pfdout, char* cmd) {
  char* argw[3];

  close(pfdout[READ_END]);

  argw[0] = "/usr/bin/whereis";
  argw[1] = cmd;
  argw[2] = NULL;

  dup2(pfdout[WRITE_END],fileno(stdout));  // send stdout to the pipe

  if (execv(argw[0], argw) == -1) {
    perror("whereis error\n");
  }
}

void cd(char* args[]) {
  char path[1024], pwdbuffer[1024];
  struct stat s;
  int err;

  if (args[1] == NULL) {
    strcpy(path, getenv("HOME"));
  } else if (args[1][0] == '/') {
    strcpy(path, args[1]);
  } else if (args[1][0] == '~') {
    strcpy(path, getenv("HOME"));
    strcat(path, args[1]+1);
  } else {
    strcpy(path, getenv("PWD"));
    strcat(path, "/");
    strcat(path, args[1]);
  }
  getcwd(pwdbuffer, sizeof(pwdbuffer));
  //printf("cwd: %s\n", pwdbuffer);
  //printf("path: %s\n", path);

  err = stat(path, &s);
  if(-1 == err) {
    if(ENOENT == errno) {
      printf("cd: No such file or directory: %s\n", args[1]);
    } else {
      perror("stat");
      exit(1);
    }
  } else if(! S_ISDIR(s.st_mode)) {
    printf("cd: Not a directory: %s\n", args[1]);
  }
  chdir(path);
  pwdbuffer[0] = '\0';
  getcwd(pwdbuffer, sizeof(pwdbuffer));
  if (strcmp(pwdbuffer, getenv("PWD")) != 0) {
    setenv("OLDPWD", getenv("PWD"), 1);
    setenv("PWD", pwdbuffer, 1);
  }
}



int parseCommand(char inputBuffer[], char *args[],int *background)
{
  int length,		/* # of characters in the command line */
  i,		/* loop index for accessing inputBuffer array */
  start,		/* index where beginning of next command parameter is */
  ct,	        /* index of where to place the next parameter into args[] */
  command_number;	/* index of requested command number */

  ct = 0;

  /* read what the user enters on the command line */
  do {
    printf("kush[%s]\\> ",getenv("PWD"));
    fflush(stdout);
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);
  }
  while (inputBuffer[0] == '\n'); /* swallow newline characters */

  /**
  *  0 is the system predefined file descriptor for stdin (standard input),
  *  which is the user's screen in this case. inputBuffer by itself is the
  *  same as &inputBuffer[0], i.e. the starting address of where to store
  *  the command that is read, and length holds the number of characters
  *  read in. inputBuffer is not a null terminated C-string.
  */
  start = -1;
  if (length == 0)
  exit(0);            /* ^d was entered, end of user command stream */

  /**
  * the <control><d> signal interrupted the read system call
  * if the process is in the read() system call, read returns -1
  * However, if this occurs, errno is set to EINTR. We can check this  value
  * and disregard the -1 value
  */

  if ( (length < 0) && (errno != EINTR) ) {
    perror("error reading the command");
    exit(-1);           /* terminate with error code of -1 */
  }

  /**
  * Parse the contents of inputBuffer
  */

  for (i=0;i<length;i++) {
    /* examine every character in the inputBuffer */

    switch (inputBuffer[i]){
      case ' ':
      case '\t' :               /* argument separators */
      if(start != -1){
        args[ct] = &inputBuffer[start];    /* set up pointer */
        ct++;
      }
      inputBuffer[i] = '\0'; /* add a null char; make a C string */
      start = -1;
      break;

      case '\n':                 /* should be the final char examined */
      if (start != -1){
        args[ct] = &inputBuffer[start];
        ct++;
      }
      inputBuffer[i] = '\0';
      args[ct] = NULL; /* no more arguments to this command */
      break;

      default :             /* some other character */
      if (start == -1)
      start = i;
      if (inputBuffer[i] == '&') {
        *background  = 1;
        inputBuffer[i-1] = '\0';
      }
    } /* end of switch */
  }    /* end of for */

  /**
  * If we get &, don't enter it in the args array
  */

  if (*background)
  args[--ct] = NULL;

  args[ct] = NULL; /* just in case the input line was > 80 */

  return 1;

} /* end of parseCommand routine */
