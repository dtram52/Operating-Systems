#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <assert.h>

#define ROOMPLAY 7
#define MAXCONN 6
/*****************************************************
 * This code may throw seg fault due to  mutex *
*if run time it will not throw seg fault and can display warning for incorrect room name input 
* *****************************************/
typedef struct Room{
int id;
char* name;
int roomType;
int numConn;
struct Room* connection[MAXCONN];
} Room;

//shared from buildrooms
const char* rType[3]={"START_ROOM", "MID_ROOM", "END_ROOM"};
//const char* 
int gameOn =1;
Room* roomArr[ROOMPLAY];
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
/**********
 * Function prototypes
 * ************/
Room* helperRoom(int, Room **, char*);
void readrName(char *);
void readrConnection(char *);

//from required reading
//http://web.engr.oregonstate.edu/~brewsteb/THCodeRepository/readDirectory.c
void getNewestDir(char *dirRoom)
{	int newestDirTime = -1;
	char targetDirPrefix[32] = "tramd.rooms.";
	char newestDirName[256];
	memset(newestDirName, '\0', sizeof(newestDirName));
	DIR* dirToCheck; // Holds the directory we're starting in
  struct dirent *fileInDir; // Holds the current subdir of the starting dir
  struct stat dirAttributes; // Holds information we've gained about subdir

  dirToCheck = opendir("."); // Open up the directory this program was run in

  if (dirToCheck > 0) // Make sure the current directory could be opened
  {
    while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
    {
      if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
      {
//        printf("Found the prefex: %s\n", fileInDir->d_name);
        stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry

        if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
        {
          newestDirTime = (int)dirAttributes.st_mtime;
          memset(newestDirName, '\0', sizeof(newestDirName));
          strcpy(newestDirName, fileInDir->d_name);
//          printf("Newer subdir: %s, new time: %d\n",
//                 fileInDir->d_name, newestDirTime);
        }
      }
    }
  }

  closedir(dirToCheck); // Close the directory we opened
strcpy(dirRoom, newestDirName);
  //printf("Newest entry found is: %s\n", newestDirName);
}


void RoomArrInit()
{
	int i;
	for(i =0; i< ROOMPLAY; i++)
		{
			roomArr[i]= malloc(sizeof( struct Room));
			roomArr[i]->id=i;
			roomArr[i]->numConn = 0;
			roomArr[i]->roomType = 1;//  rType[1] = "MID_ROOM"
		}	
	roomArr[0]->roomType = 0;// rType[0] = START_ROOM
	roomArr[6]->roomType = 2;// END_ROOM
	char dirRoom[256];
	memset(dirRoom, '\0', sizeof(dirRoom));// cleans out
	getNewestDir(dirRoom);

	readrName(dirRoom);//red room names from file
	readrConnection(dirRoom);//read connection from file
}

//read room name from room struct by parsing and skipping the mandatory format "ROOM TYPE: " -> 11 char.
void readrName( char *dirRoom)
{
	FILE *rf;
	char *buff = NULL;
	size_t buffsize=0;	
	char fileName[50];
	memset(fileName, '\0', sizeof(fileName));
	int i =0;
for (i =0; i<7; i++){
		sprintf(fileName, "%s/room%d", dirRoom, i);
	rf=fopen(fileName, "r");//read file
	getline(&buff, &buffsize, rf);
	buff[strlen(buff)-1] = '\0';  //strips off the new line from getline
	roomArr[i]->name = malloc(16*sizeof(char));
	strcpy(roomArr[i]->name, buff+11);
fclose(rf);

}
free(buff);
}
	
void readrConnection( char *dirRoom)
{
	FILE *rf;
	char *buff = NULL;
	size_t buffsize=0;
	ssize_t	secondLine;
	char fileName[50];
	memset(fileName, '\0', sizeof(fileName));
	int i =0;
	for (i=0; i<7; i++)
{
	sprintf(fileName, "%s/room%d", dirRoom, i);
	rf = fopen(fileName, "r");
	getline(&buff, &buffsize, rf);
	while ((secondLine = getline(&buff, &buffsize, rf)!=-1))
{
	buff[strlen(buff)-1] = '\0'; //strips \n
	if(buff[0] =='C' && buff[5] == 'C')//use C char in CONNECTION to make sure we grab the room connection correctly 
{
	roomArr[i]->connection[roomArr[i]->numConn] = helperRoom(ROOMPLAY, roomArr, buff+14);
	roomArr[i]->numConn++;
}
}
fclose(rf);
}
free(buff);
}

Room* helperRoom(int totalRooms, Room **roomArr, char *name){
int i;
for (i =0; i<totalRooms; i++)
{
	if(strcmp(roomArr[i]->name, name)==0) return roomArr[i];
}
return 0;
}


// Game start with showing the current room and ask for next move
void startGame(Room *cur)
{
	printf("CURRENT LOCATION: %s\n", cur->name);
  	printf("POSSIBLE CONNECTIONS: ");
	int i;

	for (i = 0; i < cur->numConn; i++)
  {
	   	 printf("%s", cur->connection[i]->name);

    	if (i < cur->numConn - 1)
      		printf(", ");
    	else
      		printf(".\nWHERE TO? >");
  } 
}


// call date in shell script and redirect it to  currentTime.txt http://stackoverflow.com/questions/1401482/yyyy-mm-dd-format-date-in-shell-scriptt 
//follow pseudo code from piazza for mutex
void DisplayTime()
{
pthread_mutex_lock(&mymutex);
if (gameOn)
  {
    system("date +\"\%l:\%M\%P, \%A, \%B \%e, \%Y\" > currentTime.txt");
    pthread_mutex_unlock(&mymutex);
  }
}


// free  memory  for room structs
void freeRooms()
{
  int i;
  for (i = 0; i < 7; i++)
  {
    free(roomArr[i]->name);
    free(roomArr[i]);
    roomArr[i] = 0; 
 }
}


int main()
{
	int i;
	int numCharsEntered = 0;
	size_t bufferSize = 0;
	char* line = NULL;

	FILE* fp;

	Room *cur; // tracks player's location
	int numSteps = 0; // number of steps taken
  	char path[1024]; // path history
  	memset(path, '\0', 1024);

	// initialize threads
	pthread_t time_thread;
int result_code;
	pthread_mutex_lock(&mymutex);
	result_code = pthread_create(&time_thread, NULL, (void *) DisplayTime, NULL);
	assert(0 == result_code);

// initalize game
	RoomArrInit();
	cur = roomArr[0];

// play game
	while (cur->roomType != 2)
{
    startGame(cur);
    
    // get user input
    numCharsEntered = getline(&line, &bufferSize, stdin);
    line[strlen(line)-1] = '\0'; // null out newline character

  // if player put time urgh here we go 
    if (strcmp(line, "time") == 0)
    {
 // unlock mutex and wait for time thread to complete
	 pthread_mutex_unlock(&mymutex);
	 result_code = pthread_join(time_thread, NULL);
     	 assert(0 == result_code);
      
      // relock mutex and recreate time thread
      pthread_mutex_lock(&mymutex);
      result_code = pthread_create(&time_thread, NULL, (void *) DisplayTime, NULL);  
      assert(0 == result_code);

      // read from currentTime.txt and out put
      fp = fopen("currentTime.txt", "r");
      numCharsEntered = getline(&line, &bufferSize, fp);
      printf("\n%s\n", line);
      fclose(fp);
    }
    // if input is to move game will continue
    else  if (helperRoom(cur->numConn, cur->connection, line) != 0)
    {
      // go into the next room
      cur = helperRoom(cur->numConn, cur->connection, line);
      // save step to path
      numSteps++;
      strcat(path, cur->name);
      strcat(path, "\n");
      printf("\n");
    }
    //  reprompt if bad input
    else
    {
      printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
    }
}
  
  // winning message
  printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
  printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", numSteps);
  printf("%s", path);
 
 //free up struct inlock mutex to call time again 
	freeRooms();
	free(line);  
	gameOn=0;
	pthread_mutex_unlock(&mymutex);
	pthread_join(time_thread, NULL);
  
return 0;
}	
