#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <stdbool.h> can't use bool C99
//typedef int bool;
//#define true 1
//#define false 0

/*pool of room names*/
#define NUM_ROOMNAME 10
#define ROOMPLAY 7
#define MAXCON 6
#define MINCON 3

#define START_ROOM 0
#define MID_ROOM 1
#define END_ROOM 2
/*************
 * create directory for room files combined with writeRoom
 * **********/

//void createDir(char *roomDir){
//int pid = getpid();
//sprintf(roomDir, "%s%d", "tramd.rooms.", pid);
//mkdir(roomDir, 0755);
//
/*************
 *Create the room struct
********************/
 


typedef struct Room
 {
int id;
char* name;
int roomType;
int numConn; 
struct Room* connection[MAXCON];
}Room;

const char*  rType[3]= {"START_ROOM", "MID_ROOM", "END_ROOM"};
const char* roomNames[NUM_ROOMNAME] = {"Facebook", "Amazon", "Apple", "Netflix", "Google", "Microsoft", "Linkedin", "Stripe", "Twitter", "Tesla"};
Room* roomArr[ROOMPLAY];

/*****************
 * Function Prototypes Used from readings and changed bool to int
 * **************/
int IsGraphFull();
void AddRandomConnection();
Room* GetRandomRoom();
void ConnectRoom(Room*, Room*);
int CanAddConnectionFrom(Room*);
int IsSameRoom(Room*, Room*);
int ConnectionAlreadyExists(Room*, Room*);



//initialize an array of rooms
void RoomArrInit()
{//assign START_ROOM and END_ROOM to the first and last room in roomArr
//loop to  allocate and initialize the array  
	int i;
	for( i =0; i < ROOMPLAY; i++)
		{
			roomArr[i] = malloc(sizeof(struct Room));
			roomArr[i]->id =i;
			roomArr[i]->name = calloc(100, sizeof(char));
			//printf("%s\n" , roomNames[i]);
			//fflush(stdout);
			strcpy(roomArr[i]->name, roomNames[i]);
			roomArr[i]->roomType = MID_ROOM; //size rType[1]= "MID_ROOM"
			roomArr[i]->numConn = 0;	
	}

	roomArr[0]->roomType = START_ROOM;
	roomArr[6]->roomType = END_ROOM;

}	
//struct room list to create file for writing and reading
//saw suggestion to shuffle array on piazza and applied this link https://stackoverflow.com/questions/6127503/shuffle-array-in-c
void RoomArrShuffle()
{
srand(time(NULL)); //seed 
const char* temp;
int i;
for( i =0; i<NUM_ROOMNAME -1; i++)
{
size_t k = i + rand() / (RAND_MAX/ (NUM_ROOMNAME -i) +1);
temp = roomNames[k];
roomNames[k] = roomNames[i];
roomNames[i] = temp;
}
}
//Reference from required readig 2.2 Program Outliing in Program 2
//Changed some of the function and using 0, 1 instead of true false as I remove <stdbool.h>
int  IsGraphFull()
{
int i;
for( i =0; i<7; i++)//iterate through the array of 7 rooms to check if the number of connection 
{
if(roomArr[i]->numConn <=3) return 0;
}
return 1;
}

void AddRandomConnection()  
{
  Room* A;  // Maybe a struct, maybe global arrays of ints
  Room* B;

  while(1)
  {
    A = GetRandomRoom();


    if (CanAddConnectionFrom(A) == 1)
      break;
  }

  do
  {
    B = GetRandomRoom();
  }
  while(CanAddConnectionFrom(B) == 0 || IsSameRoom(A, B) == 1 || ConnectionAlreadyExists(A, B) == 1);

  ConnectRoom(A, B);  // TODO: Add this connection to the real variables, 
  ConnectRoom(B, A);  //  because this A and B will be destroyed when this function terminates
}

//Returns a random Room
Room* GetRandomRoom()
{
return roomArr[rand()%7];
}

int CanAddConnectionFrom(Room *x)
{
	int bool = 0;
	if(x->numConn <6) bool =1;
return bool;
}


int IsSameRoom(Room *x, Room *y)
{
	int bool = 0;
	if(x->id == y->id) bool = 1;
return bool;
}
		 

void ConnectRoom(Room *x, Room*y)
{
x->connection[x->numConn] = y;
x->numConn++;
}

int ConnectionAlreadyExists(Room *x, Room *y)
{

	int bool =0;
	int i;
	for (i =0; i< x->numConn; i++)
	{
		if(x->connection[i]->id == y->id)
		bool =1;
	}
	return bool;
} 
//struct Room roomFiles[ROOMPLAY];

void writeRoom()
{	FILE* rf;
	char dirRoom[50];
	char fileName[55];
	
	int pid = getpid();
	memset(dirRoom, '\0', 50);
	//play with creaing name for dir
	sprintf(dirRoom, "%s%d", "tramd.rooms.", pid);
	//create diretory
	mkdir(dirRoom, 0755);
	int i;
	int j;
	for( i =0; i < 7; i++){
		//create file name as room0 to room6  in order to easily manipulate 
		sprintf(fileName, "%s/room%d", dirRoom, i);	
		//open and write room file
		rf = fopen(fileName, "w");
		fprintf(rf, "ROOM NAME: %s\n", roomArr[i]->name);
		//use loop to print connections into file
		for ( j =0; j < roomArr[i]->numConn; j++)
			{
				fprintf(rf, "CONNECTION %d: %s\n", j+1, roomArr[i]->connection[j]->name);
			} 
		//print room type
			fprintf(rf, "ROOM TYPE: %s\n", rType[roomArr[i]->roomType]);
			fclose(rf);
				}		
}

void freeRoom()
{	int i;
	for( i =0; i<7; i++)
		{
			free(roomArr[i]->name);
			free(roomArr[i]);
		}
}

int main()
 { 	RoomArrShuffle();
	RoomArrInit();
	while(IsGraphFull() == 0)
	{
		AddRandomConnection();
	}
	writeRoom();
	freeRoom();
	return 0;
}
	
		
