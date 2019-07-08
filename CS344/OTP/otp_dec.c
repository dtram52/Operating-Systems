#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <netdb.h> 

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	int  totalCharsWritten, totalCharsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];
    	FILE * fp;

	long fileSize;
	char * text;
	char * key;
	char * returnMes;

	if (argc < 4) { fprintf(stderr,"USAGE: %s text key port\n", argv[0]); exit(0); } // Check usage & args

 	// open and retrieve text from  text file
	fp = fopen(argv[1], "r");
	if (fp == NULL) { fprintf(stderr, "Error: could not open %s\n", argv[1]); exit(1); } 
	fseek(fp, 0, SEEK_END); // file size/length
	fileSize = ftell(fp); 
	fseek(fp, 0, SEEK_SET);
	text = malloc(fileSize * sizeof(char)); // memory for text
	fread(text, sizeof(char), fileSize, fp);	// read text into string
        text[strcspn(text, "\n")] = '\0'; 
	fclose(fp);		

 	// retrieve tex key file
	fp = fopen(argv[2], "r");
	if (fp == NULL) { fprintf(stderr, "Error: could not open %s\n", argv[2]); exit(1); } 
	fseek(fp, 0, SEEK_END); // check file size/length
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	key = malloc(fileSize * sizeof(char)); //memory for key 
	fread(key, sizeof(char), fileSize, fp);	// read key into string
        key[strcspn(key, "\n")] = '\0'; 
	fclose(fp);		

	// check key length against text
	if (strlen(key) < strlen(text)) { fprintf(stderr, "Error: key '%s', is too short\n", argv[2]); exit(1); }


	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "OTP_DEC: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("OTP_DEC: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
         fprintf(stderr, "Error: could not contact otp_dec_d on port %d\n", portNumber);
	
        // confirm otp_dec id
        int id = htonl(0);//host to network conversion for id
        send(socketFD, &id, sizeof(id), 0);
        if (charsWritten < 0) error("OTP_DEC: ERROR writing to socket\n");

        // Confirm connection
        int return_status = recv(socketFD, &id, sizeof(id), 0);
        if (ntohl(id) != 0) { fprintf(stderr, "Error: otp_dec cannot connect to otp_enc_d!\n"); exit(1); }

	// Send text length to server
	int textLen = htonl(strlen(text));
	send(socketFD, &textLen, sizeof(textLen), 0); // send the length of the txt string
        if (charsWritten < 0) error("OTP_DEC: ERROR writing to socket\n");

        // Send text to server
      	totalCharsWritten = 0;
        while (totalCharsWritten < strlen(text))
	{
		// Load portion of text into buffer
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
		strncpy(buffer, text + totalCharsWritten,  sizeof(buffer) - 1); // Get input from the text string, trunc to buffer - 1 chars, leaving \0
               // Send portion of text
		charsWritten = send(socketFD, buffer, strlen(buffer), 0);
		if (charsWritten < 0) error("OTP_DEC: ERROR writing to socket");
		if (charsWritten < strlen(buffer)) printf("OTP_DEC WARNING: Not all data is  written to socket!\n");
		totalCharsWritten += (charsWritten); // don't count null terminater for totalCharsWritten
	}

        // Send key to server
      	totalCharsWritten = 0;
        while (totalCharsWritten < strlen(text))
	{
		// Load portion of key into buffer
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
		strncpy(buffer, key+totalCharsWritten, sizeof(buffer)-1); // Get input from the key string, trunc to buffer - 1 chars, leaving \0
               // Send portion of text
		charsWritten = send(socketFD, buffer, strlen(buffer), 0);
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
		totalCharsWritten += charsWritten; // don't count null terminater for totalCharsWritten
	}


	// Get return message from server
	returnMes = malloc(sizeof(char)*strlen(text));
        memset(returnMes, '\0', sizeof(returnMes)); 
        totalCharsRead = 0;
        while (totalCharsRead < strlen(text))
	{
          memset(buffer, '\0', sizeof(buffer));
          charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
  	  if (charsRead < 0) error("CLIENT: ERROR reading from socket");
          strcat(returnMes, buffer);
          totalCharsRead += charsRead;
        }
	printf("%s\n", returnMes);

	close(socketFD); // Close the socket
	return 0;
}
