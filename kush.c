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
  char *args[MAX_LINE/2 + 1],*args2[2]; /* command line (of 80) has max of 40 arguments */
  pid_t child, child2;            		/* process id of the child process */
  int status;           		/* result from execv system call*/
  int shouldrun = 1;

  int i, upper;

  int pfd[2],pfd2[2];
  char buffer[1024], buffer2[7];

  while (shouldrun){            		/* Program terminates normally inside setup */
    background = 0;

    shouldrun = parseCommand(inputBuffer,args,&background);       /* get next command */

    if (strncmp(inputBuffer, "exit", 4) == 0)
      shouldrun = 0;     /* Exiting from kush*/

    if (shouldrun) {
      /*
	After reading user input, the steps are
	(1) Fork a child process using fork()
	(2) the child process will invoke execv()
	(3) if command included &, parent will invoke wait()
       */

       if (pipe(pfd) < 0) {
         printf("Failed to create pipe.\n");
         exit(-1);
       }


       if (pipe(pfd2) < 0) {
         printf("Failed to create pipe.\n");
         exit(-1);
       }

       child = fork();

       if (child < 0) {
         printf("Failed to fork.\n");
         exit(-1);
       }


       if (child == 0) {

         child2 = fork();

         if(child2 == 0){
           close(pfd2[READ_END]);
           dup2(pfd2[WRITE_END],fileno(stdout));  // send stdout to the pipe

           args2[0] = "/usr/bin/which";
           args2[1] = args[0];

           execv("/usr/bin/which", args2);
           return 0;

           } else {

            close(pfd2[WRITE_END]);
            read(pfd2[READ_END], buffer2, sizeof(buffer2));
            killChild(child2);
            printf("Child: Starting to execute command %s\n", buffer2);

            args[0] = buffer2;

            close(pfd[READ_END]);
            dup2(pfd[WRITE_END],fileno(stdout));  // send stdout to the pipe
            execv(buffer2, args);

            return 0;
          }
        } else {
         if( background ) {
           printf("Process is in background. Parent keeps running...\n");
           close(pfd[READ_END]);
           close(pfd[WRITE_END]);
         } else {
          //  printf("Parent is waiting...\n");
           close(pfd[WRITE_END]);
           read(pfd[READ_END], buffer, sizeof(buffer));
           killChild(child);
           printf("Parent: %s\n", buffer);
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
	  printf("kush> ");
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
