#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

int main(int argc, char *argv[]){
	
	char * chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";//26 characters and a space

	/*verify usage and input*/
	if(argc!=2) {
		fprintf(stderr, "USAGE: %s keylength\n", argv[0]); exit(1);
	}

	int key_len;
	key_len = atoi(argv[1]); //convert key length into int

	/* generate random char*/
	srand(time(NULL)); //seed random numbers
	int i;
	for ( i =0; i < key_len; i++)	{
		printf("%c", chars[rand() % 27]);
		}	
	printf("\n");
	return 0;
}
