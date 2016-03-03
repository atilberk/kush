/**
* KUSH shell interface program
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#define MAX_LINE       80 /* 80 chars per line, per command, should be enough. */

// define pipe ends
#define READ_END 0
#define WRITE_END 1

int parseCommand(char inputBuffer[], char *args[],int *background);
void killChild(pid_t pid);

int main(void)
{
  char inputBuffer[MAX_LINE]; 	        /* buffer to hold the command entered */
  int background;             	        /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/2 + 1],*args2[2],*args3[2]; /* command line (of 80) has max of 40 arguments */
  pid_t execchild, whereischild, whichchild;            		/* process id of the child process */
  int status;           		/* result from execv system call*/
  int shouldrun = 1;

  int i, upper, err;

  int pfd[2],pfd2[2],pfd3[2];
  char outputbuffer[1024*1024], whereisbuffer[128], whichbuffer[128], *oldpwdbuffer, *pwdbuffer;
  int whereislength, commandLength, outLength;

  while (shouldrun){            		/* Program terminates normally inside setup */
    background = 0;

    shouldrun = parseCommand(inputBuffer,args,&background);       /* get next command */

    if (strcmp(inputBuffer, "exit") == 0) {
      shouldrun = 0;     /* Exiting from kush*/
    } else if (strcmp(inputBuffer, "clear") == 0) {
      printf("\e[H\e[2J");
      continue;
    } else if (strcmp(inputBuffer, "cd") == 0) {
      if (args[1] == NULL) {
        oldpwdbuffer = malloc(128);
        strcpy(oldpwdbuffer,"OLDPWD=");
        strcat(oldpwdbuffer,getenv("PWD"));
        putenv(oldpwdbuffer);
        pwdbuffer = malloc(128);
        strcpy(pwdbuffer,"PWD=/");
        putenv(pwdbuffer);
        chdir("/");
      } else {
        oldpwdbuffer = malloc(128);
        strcpy(oldpwdbuffer,"OLDPWD=");
        strcat(oldpwdbuffer,getenv("PWD"));
        putenv(oldpwdbuffer);
        pwdbuffer = malloc(128);
        strcpy(pwdbuffer,"PWD=");
        if (args[1][0] != '/') {
          strcat(pwdbuffer,getenv("PWD"));
          strcat(pwdbuffer,"/");
        }
        strcat(pwdbuffer,args[1]);
        putenv(pwdbuffer);
        chdir(getenv("PWD"));
      }
      continue;
    }

    if (shouldrun) {
      /*
      After reading user input, the steps are
      (1) Fork a child process using fork()
      (2) the child process will invoke execv()
      (3) if command included &, parent will invoke wait()
      */

      if (pipe(pfd) < 0 || pipe(pfd2) < 0 || pipe(pfd3) < 0) {
        printf("Failed to create pipe.\n");
        exit(-1);
      }

      //printf("Parent\n");
      if ((execchild = fork()) < 0) {
        printf("Failed to fork.\n");
        exit(-1);
      }


      if (execchild == 0) {
        //printf("Execchild\n");
        if ((whichchild = fork()) < 0) {
          printf("Failed to fork.\n");
          exit(-1);
        }

        if(whichchild == 0){
          //printf("Whichchild\n");
          if ((whereischild = fork()) < 0) {
            printf("Failed to fork.\n");
            exit(-1);
          }

          if (whereischild == 0) {
            //printf("Wherischild\n");
            close(pfd3[READ_END]);

            args3[0] = "/usr/bin/whereis";
            args3[1] = args[0];

            //printf("[whereis]: %s []\n", args[0]);

            dup2(pfd3[WRITE_END],fileno(stdout));  // send stdout to the pipe

            if (execv(args3[0], args3) == -1) {
              perror("whereis error\n");
            }

          } else {
            close(pfd2[READ_END]);
            close(pfd3[WRITE_END]);
            //printf("waiting to read whereis result\n");
            whereislength = read(pfd3[READ_END], whereisbuffer, sizeof(whereisbuffer));
            whereislength = whereislength / 2;
            killChild(whereischild);
            //printf("Whereislength: %d\n", whereislength);
            //printf("Whereis2ndchar: %c\n", whereisbuffer[4]);
            //printf("Whereis output: %s endo\n", whereisbuffer);
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
          //printf("waiting to read which result\n");
          commandLength = read(pfd2[READ_END], whichbuffer, sizeof(whichbuffer));
          //commandLength = commandLength /2;
          killChild(whichchild);
          //printf("Commandlength: %d.\n", commandLength);

          for(i = 0; i < commandLength; i++) {
            if (whichbuffer[i] == '\n'){
              //printf("Command is %d long\n", i);
              whichbuffer[i] = '\0';
              break;
            }
          }
          wait();
          //printf("Child: Starting to execute command: %s :child\n", whichbuffer);

          close(pfd[READ_END]);
          dup2(pfd[WRITE_END],fileno(stdout));  // send stdout to the pipe
          if (whichbuffer[0] == '!') {
            printf("%s: command not found\n", args[0]);
          } else {
            args[0] = whichbuffer;
            if (execv(args[0], args) == -1) {
              perror("exec error\n");
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
          //  printf("Parent is waiting...\n");
          close(pfd[WRITE_END]);
          outLength = read(pfd[READ_END], outputbuffer, sizeof(outputbuffer));
          killChild(execchild);
          outputbuffer[outLength-1] = '\0';
          //printf("outLength: %d\n",outLength);
          if (outLength > 0)
            printf("%s\n",outputbuffer);
          wait();

          //  printf("%s \n", "Parent: Killing child");
        }
      }
    }
  }
  return 0;
}

void killChild(pid_t pid)
{
  kill(pid, SIGUSR1);
}

/**
* The parseCommand function below will not return any value, but it will just: read
* in the next command line; separate it into distinct arguments (using blanks as
* delimiters), and set the args array entries to point to the beginning of what
* will become null-terminated, C-style strings.
*/

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
