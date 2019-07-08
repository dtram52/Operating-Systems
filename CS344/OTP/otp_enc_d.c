#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>


void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
  int listenSocketFD, establishedConnectionFD, portNumber, charsRead, charsWritten;
	 int  totalCharsRead, totalCharsWritten;
  int  i, childExitMethod, childPID, charsToRead;
  char buffer[256];
  struct sockaddr_in serverAddress, clientAddress;
  pid_t spawnpid = -5;
  socklen_t sizeOfClientInfo;

  char * text;
  char *  key;
  char * chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
 
  if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

  // Set up the address struct for this process (the server)
  memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
  portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
  serverAddress.sin_family = AF_INET; // Create a network-capable socket
  serverAddress.sin_port = htons(portNumber); // Store the port number
  serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

  // Set up the socket
  listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (listenSocketFD < 0) error("ERROR opening socket");

  // Enable the socket to begin listening
  if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
	  error("ERROR on binding");
  listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

  // fork a new child process
  while (1)
  {
      childPID = wait(&childExitMethod); 
        
    // fork a new process
 	spawnpid = fork();
 	switch (spawnpid)
      {
	// child
	case -1:
		perror("Hull breach!");//error creating child process
	case 0:
	    // Accept a connection, blocking if one is not available until one connects
	    sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	    establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
	    if (establishedConnectionFD < 0) error("ERROR on accept");
            
            // get identity of connected process 
            int id = 0;
            recv(establishedConnectionFD, &id, sizeof(id), 0);

            // send integer indicating whether connection is allowed
            if (htonl(id) != 1) id = htonl(-1);
            send(establishedConnectionFD, &id, sizeof(id), 0);
            if (charsWritten < 0) error("OTP_ENC_D: ERROR writing to socket\n");

            // get length of message
            int textLen = 0;
            int return_status = recv(establishedConnectionFD, &textLen, sizeof(textLen), 0);
	    if (return_status > 0) {
              textLen = ntohl(textLen);
      	    } else {
              printf("error");
	      exit(3);
            }

	    // Get text  client 
            totalCharsRead = 0;
            text = malloc(sizeof(char) * textLen); 
	    memset(text, '\0', sizeof(text));
            while (totalCharsRead < textLen)
            {
              memset(buffer, '\0', sizeof(buffer));
              if ((textLen - totalCharsRead) < (sizeof(buffer) - 1) )
                charsToRead = textLen - totalCharsRead;
              else
                charsToRead = sizeof(buffer) - 1; 
              charsRead = recv(establishedConnectionFD, buffer, charsToRead, 0); // Read text from socket
              if (charsRead == 0) { fprintf(stderr, "File read failed"); exit(1); }
              strcat(text, buffer);
              totalCharsRead += charsRead; 
            }

	    // Get key from client 
            totalCharsRead = 0;
            key = malloc(sizeof(char) * textLen); 
	    memset(key, '\0', sizeof(key));
            while (totalCharsRead < textLen)
            {
              memset(buffer, '\0', sizeof(buffer));
	      if ((textLen - totalCharsRead) < (sizeof(buffer) - 1) )
                charsToRead = textLen - totalCharsRead;
              else
                charsToRead = sizeof(buffer) - 1; 
              charsRead = recv(establishedConnectionFD, buffer, charsToRead, 0); // Read key from  socket
              if (charsRead == 0) { fprintf(stderr, "Read failed"); exit(1); }
              strcat(key, buffer);
              totalCharsRead += charsRead; 
            }

            // encrypt the message
            for (i = 0; i < textLen; i++)
            {

              int nChar = text[i] - 64; // convert char to numberic value
              int nKey = key[i] - 64;
              if (nChar < 0) nChar = 0; // replace space value so all characters are contiguous
              if (nKey < 0) nKey  = 0;
              text[i] = chars[(nChar + nKey)  % 27];
            }

	    // Send the message back to the client
            totalCharsWritten = 0;
            while (totalCharsWritten < strlen(text)-1)
            {
               memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
               strncpy(buffer, text + totalCharsWritten,  sizeof(buffer)-1); // Get input from the text string, trunc to buffer - 1 chars, leaving \0
               
               charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
               if (charsWritten < 0) error("OTP_ENC_D: ERROR writing to socket");
               if (charsWritten < strlen(buffer)) printf("OTP_ENC_D  WARNING: Not all data is written to socket\n");
               totalCharsWritten += charsWritten-1;
            }
            
            // make sure send is complete before closing referenced from required readomg
            int checkSend = -5;  // Bytes remaining in send buffer
            do
            {
              ioctl(establishedConnectionFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
            }
            while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
  
            if (checkSend < 0)  // Check if we actually stopped the loop because of an error
              error("ioctl error");
	    close(establishedConnectionFD); // Close the existing socket which is connected to the client
	    exit(0);

	    break;
	// parent
	default:
    	  childPID = wait(&childExitMethod, WNOHANG);
          }
     
    
  }  
  return 0; 
}
