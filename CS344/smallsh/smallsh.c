#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>

pid_t spawnpid =-5;//-5 for  easy tracking
pid_t childpid; //child pid
pid_t bgpids[256]; //arr holding background pids
size_t bufferSize = 0; //for buffer use
char argString[2048]; //2048 char limit
char * args[512]; //hold up to 512 arguments limit
char * inputLine = NULL; //+ n + \0 buffer pointer allocated by getline() 
char * token;
char * command;
char * expandPid;
int i, j, result;
char * statusMsg;
int childExitMethod = -5; //default to -5  for easy tracking
int exitStatus;
int numBGpids =0;
int numArgs =0;
int numCharsEntered = -5;
bool badFile = false;
bool fgMode = false;
bool bgMode = false;
bool normTerm = true;
FILE *fpIn = NULL;
FILE *fpOut = NULL;

struct sigaction SIGINT_catch = {0};
struct sigaction SIGINT_ignore = {0};
struct sigaction SIGINT_default = {0};
struct sigaction SIGTSTP_catch = {0};
struct sigaction SIGTSTP_ignore = {0};

//global varible and reset all variable for easy tracking
void trackVar()
{	bgMode = false;
	badFile = false;
	spawnpid = -5;
	//reset file pointer
	if(fpIn) fclose(fpIn);
	if(fpOut) fclose(fpOut);
	fpIn = NULL;
	fpOut = NULL;
	//free args
	for (i=0; i<numArgs; i++){
		free(args[i]);} 
		numArgs=0;
	for( i =0; i<512; i++) args[i] = NULL;
	//free getline
	if (inputLine) free(inputLine);
	inputLine = NULL;
}

void waitChild()
{
  // wait for child exit referenced lecture 3.1
  waitpid(spawnpid, &childExitMethod, 0);

  // look for and store status
  if (WIFEXITED(childExitMethod))
  {
    exitStatus = WEXITSTATUS(childExitMethod); 
    normTerm = true;
  }
  else
  {
    exitStatus = WTERMSIG(childExitMethod);
    normTerm = false;
  }
}
//SIGSTP handler, catch and handle Ctrl-Z signals
void SIGTSTP_handler(int signo)
{	fgMode = !fgMode;
	if (!fgMode) {
		char* message = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 49);
	}
	else {
		char* message  = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 29);
	}
}
//SIGINT handler
void SIGINT_handler(int signo)
{
	if(spawnpid !=-5)
{		char* termMessage = "\nterminated by signal 2\n";
		write(STDOUT_FILENO, termMessage, 24);
		waitChild();
}
	else
	{
	write(STDOUT_FILENO, "\n", 1);
	}
}

checkBG()

{
  // check background pids
  // use WIFEXITED to check child exit and WEXITSTATUS and WTERMSIG to inform 
  for (i = 0; i < numBGpids; i++)
  {
    fflush(stdout);
    childpid = 
    fflush(stdout);
    if (waitpid(bgpids[i], &childExitMethod, WNOHANG)) // if there is a finished process 
    	{ 	printf("background pid %d is done: ", bgpids[i]);
		fflush(stdout);
      		// use WIFEXITED to catch and store status
        	if (WIFEXITED(childExitMethod))
      		{
        		printf("exit value %d\n", WEXITSTATUS(childExitMethod));
        		fflush(stdout); 
      		}
      		else
      		{
        		printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
        		fflush(stdout); 
     		 }	
      // remove pid
      for (j = i+1; j < numBGpids; j++)
      {
        bgpids[i] = bgpids[j];
      }
      numBGpids--;  
    }
  }      
}


// Gets input from the user, verify comment and empty input, and then check background processes
void verifyInputCheckBG()
{

  while(1)  
  {	int numCharsEnters = 0;
    	checkBG();
    	printf(":");//default colon to get input
    	fflush(stdout);
    	numCharsEntered = getline(&inputLine, &bufferSize, stdin); //getline input 
    	if (numCharsEntered == -1)
      		clearerr(stdin);// clear error from stdin
    else if ((inputLine[0] == '\n') || (inputLine[0] == '#'))
      continue; // continue looping if empty line or a comment 5 pts
    else
      break; // Exit the loop - we've got input
  }
  inputLine[strcspn(inputLine, "\n")] = '\0'; // remove trailing getline's  \n 
}

//this will help parse command and add to arg list
void parseCommand()
{
  command = strtok(inputLine, " ");//use strtok to tokenize string
  args[numArgs] = malloc(sizeof(char) * strlen(command));
  strcpy(args[numArgs], command);
  numArgs++;
}


//parse argument into argsString
//expand pid if necessary
//put them in an array
void parseArg()
{
    	memset(argString, '\0', 2048);
    	sprintf(argString, "%s", token);
 	
    // expand PID if we found $$
	char * expandPid;
	expandPid = strstr(argString, "$$");
   
	if (expandPid)
    	{
      		sprintf(expandPid, "%d", getpid()); // expand $$
      		strcat(argString, token + (expandPid-argString+2)); // re-copy end of argument 
    	}
    // copy into array
    args[numArgs] = malloc(sizeof(char)*strlen(argString));
    strcpy(args[numArgs++], argString); 
}            
// Built-in exit 
void bInExit()
{
  exit(0);
}


// Built-in cd
void bInchDir()
{
  token = strtok(NULL, " ");
  if (token != NULL)
  {
    parseArg();
    if (chdir(args[1]) != 0)// if it's zero then built-in command will take care of it. otherwise,  change directory returns non-zero, the directory name is off
    {
      printf("No directory with such name");
      fflush(stdout);
    }
  }
  else
  {
    chdir(getenv("HOME"));
  }
}

// Built-in status
void bInStatus()
{
  if (normTerm)
  {
    printf("exit value %d\n", exitStatus);
    fflush(stdout); 
  }
  else
  {
    printf("terminated by signal %d\n", exitStatus);
    fflush(stdout); 
  }
}


// open  output file  or  error message if necessary
void openOutFile()
{
  token = strtok(NULL, " ");
  fpOut = fopen(token, "w");
  if (fpOut == NULL)
  {
    printf("cannot open %s for output\n", token);           
    fflush(stdout);
    exitStatus = 1;
    badFile  = true;
  }
}


// open input file,  error message if necessary
void openInFile()
{
  token = strtok(NULL, " ");
  fpIn = fopen(token, "r");
  if (fpIn == NULL)
  {
    printf("cannot open %s for input\n", token);           
    fflush(stdout);
    exitStatus = 1;
    badFile  = true;
  }

}


//initialize background process  redirection to dev null
void InitBG()
{
  bgMode = true;
  if (fpIn != NULL)
  {
    fpIn = fopen("/dev/null", "r");
  }
  if (fpOut != NULL) 
  {
    fpOut = fopen("/dev/null", "w");
  }
}


//initialize  child processes signal handler
void childSignalHandlerInit()
{
  // set signal handlers
  	if (bgMode)
  	{
    sigaction(SIGINT, &SIGINT_ignore, NULL); //background processes ignore SIGINT
	}
  	else {
		signal(SIGINT, SIG_DFL);
	}
   sigaction(SIGTSTP, &SIGTSTP_ignore, NULL);
}


// Calls dup2 using fpIn and fpOut 
void redirections()
{
  // redirections
  if (fpOut != NULL)
  {
    result = dup2(fileno(fpOut), 1);
    if (result == -1) { perror("source dup2"); exit(2); }
    fclose(fpOut);
  }
  if (fpIn != NULL)
  {
    result = dup2(fileno(fpIn), 0); 
    if (result == -1) { perror("source dup2"); exit(2); }
    fclose(fpIn);
  }
}


// exec or  error message if necessary
void executions()
{
  // call exec
  execvp(command, args);
  printf("%s: no such file or directory\n", command);
  fflush(stdout);
  exit(2);
}


int main()
{
  // initialize signal handlers
  SIGINT_catch.sa_handler = SIGINT_handler;
  sigfillset(&SIGINT_catch.sa_mask);
  SIGINT_ignore.sa_handler = SIG_IGN;
  SIGINT_default.sa_handler = SIG_DFL;
  SIGTSTP_catch.sa_handler = SIGTSTP_handler;
  sigfillset(&SIGTSTP_catch.sa_mask);
  SIGTSTP_ignore.sa_handler = SIG_IGN; 
 
  // set parent to catch SIGINT and SIGSTP
  sigaction(SIGINT, &SIGINT_catch, NULL);
  sigaction(SIGTSTP, &SIGTSTP_catch, NULL);


  // iterate until exit process is invoked
  while(1==1)
  {
    trackVar();

    verifyInputCheckBG();
    parseCommand();

    // built-in command
    if (strcmp(command, "exit") == 0)
    {
      bInExit();
    }
    else if (strcmp(command, "cd") == 0)
    {
      bInchDir();
    }
    else if (strcmp(command, "status") == 0)
    {
      bInStatus();
    }
    // other shell commands   
    else
    {
      // parse remaining  command line input
      while( ((token = strtok(NULL, " ")) != NULL) && (!badFile) )
      {
        // stdout redirect
        if (strcmp(token, ">") == 0)
        {
          openOutFile();
        }
        // stdin 
        else if (strcmp(token, "<") == 0)
        {
          openInFile();
        }
        // if user turns background mode on using & 
        else if (strcmp(token, "&") == 0)
        {
          if (((token = strtok(NULL, " ")) == NULL) && !fgMode)
            InitBG();
        }
        // parse command arguments into array
        else
  {
          parseArg();
        }
      }
    
      // fork new process, unless there was an error with input referenced lecture slides
      if (!badFile)
        spawnpid = fork();
      else
        spawnpid = -2;

      switch (spawnpid)
      {
   // child
        case 0:
		childSignalHandlerInit();
          	redirections();
          	executions();
          	break;

	//error
	case -1:
         	perror("Hull Breach!");
        	exit(1);
         	break;
     
       	case -2:
        	break;

        // parent
        default:       
          if (bgMode)
          {  
            // track background pids
            bgpids[numBGpids++] = spawnpid;     
            printf("background pid is %d\n", spawnpid);
            fflush(stdout);
          }
          else
          { 
            waitChild();
          }
      }
   
    }
  }
  return 0;
}

