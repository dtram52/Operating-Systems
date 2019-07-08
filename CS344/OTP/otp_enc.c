#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	int i,  totalCharsWritten, totalCharsRead, charsToRead;//to keep track of text length
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];
    	FILE * fp;
	char * text;
	char * key;
	char * returnMes;
	long fileSize;
	
	if (argc < 4) { fprintf(stderr,"USAGE: %s text key port\n", argv[0]); exit(0); } // Check usage & args

	//referenced from https://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffert

  	// Get key content
	fp = fopen(argv[2], "r");
	if (fp == NULL) { fprintf(stderr, "Error: could not open %s\n", argv[2]); exit(1); } 
	fseek(fp, 0, SEEK_END); // file size/length
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	key = malloc(fileSize * sizeof(char)); 
	fread(key, sizeof(char), fileSize, fp);		
        key[strcspn(key, "\n")] = '\0'; 
	fclose(fp);		

	
 	
	// get text file content
	fp = fopen(argv[1], "r");
	if (fp == NULL) { fprintf(stderr, "Error: could not open %s\n", argv[1]); exit(1); } 
	fseek(fp, 0, SEEK_END); // identify file size/length
	fileSize = ftell(fp); 
	fseek(fp, 0, SEEK_SET);
	text = malloc(fileSize * sizeof(char)); 
	fread(text, sizeof(char), fileSize, fp);	
        text[strcspn(text, "\n")] = '\0'; 
	fclose(fp);		

	//validate text and key length 
	if (strlen(key) < strlen(text)) { fprintf(stderr, "Error: key '%s' is too short\n", argv[2]); exit(1); }

        // throw error for invalid text
        for (i = 0; i < strlen(text); i++)
        {
          if (!( (text[i] > 64 && text[i] < 91) || (text[i] == 32) ))//check to see if its the right range of characters
            {
              fprintf(stderr, "Error: invalid characters in file\n");
              exit(1);
            }
        }	

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "OTP_ENC: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("OTP_ENC: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", portNumber);
        
        // check to see if it's  otp_enc (not otp_dec)
        

	int id = htonl(1);//host to net for long conversion
        send(socketFD, &id, sizeof(id), 0);
        if (charsWritten < 0) error("OTP_ENC: ERROR writing to socket\n");
        
        int return_status = recv(socketFD, &id, sizeof(id), 0);//confirm the identity
        if (ntohl(id) != 1) { fprintf(stderr, "Error: otp_enc cannot connect to otp_dec_d!\n"); exit(1); }
          

	// Send text length to server
	int textLen = htonl(strlen(text));
	send(socketFD, &textLen, sizeof(textLen), 0); // send the length of the text string
        if (charsWritten < 0) error("CLIENT: ERROR writing to socket\n");

        // Send text to server
      	totalCharsWritten = 0;
        while (totalCharsWritten < strlen(text))
	{
		// Load portion of text into buffer
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
		strncpy(buffer, text + totalCharsWritten,  sizeof(buffer) - 1); // Get input from the text string, trunc to buffer - 1 chars, leaving \0
               // Send portion of text
		charsWritten = send(socketFD, buffer, strlen(buffer), 0);
		if (charsWritten < 0) error("OTP_ENC: ERROR writing to socket");
		if (charsWritten < strlen(buffer)) printf("OTP_ENC: WARNING: Not all data written to socket!\n");
		totalCharsWritten += (charsWritten); // don't count null terminater for totalCharsWritten
	}

        // Send key to server
      	totalCharsWritten = 0;
        while (totalCharsWritten < strlen(text))
	{
		// Load chunk of key into buffer
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
		strncpy(buffer, key+totalCharsWritten, sizeof(buffer)-1); // Get input from the key string, trunc to buffer - 1 chars, leaving \0
               // Send portion of text
		charsWritten = send(socketFD, buffer, strlen(buffer), 0);
		if (charsWritten < 0) error("OTP_ENC: ERROR writing to socket");
		if (charsWritten < strlen(buffer)) printf("OTP_ENC WARNING: Not all data is  written to socket!\n");
		totalCharsWritten += charsWritten; // totalCharsWritten excludes NULL terminator
	}


	// Get return message from server
	returnMes = malloc(sizeof(char)*strlen(text));
        memset(returnMes, '\0', sizeof(returnMes)); 
        totalCharsRead = 0;
        while  (totalCharsRead < strlen(text))
	{

          memset(buffer, '\0', sizeof(buffer));
          if ((strlen(text) - totalCharsRead) < (sizeof(buffer) - 1) )
            charsToRead = strlen(text)- totalCharsRead;
          else
            charsToRead = sizeof(buffer) - 1;
          charsRead = recv(socketFD, buffer, charsToRead, 0); // Read data from the socket, leaving \0 at end
          if (charsRead < 0) error("OTP_ENC: ERROR reading from socket");
          strcat(returnMes, buffer);
          totalCharsRead += charsRead;
        }
	printf("%s\n", returnMes);

	close(socketFD); 
	return 0;
}
