
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

//Provided Limitations 
#define MAX_CLIENTS 100

//standard error messages
char* pong = "ERRO PONG is only for the server";
char* okay  = "ERRO OKAY is in fact not ok\r\n";
char* erro = "ERRO ERRO is not a joke, please refrain";
char* gen = "ERRO Invalide protocol try again";
char* delim = "ERRO delimiting incorrect";
char* almost = "ERRO just about had ping, check your spelling";

//thread caller to handle socket connection
void* receptionist(void* param){
	int newsockfd, n = 1;
	char buffer[256], error[40];
	
	newsockfd = *((int*)param);
	
	while(n != 0){
	if (newsockfd < 0) 
	{
		perror("ERROR on accept");
		exit(1);
	}
	
	bzero(buffer,256);

	/* Read characters from the connection,
		then process */
	
	n = read(newsockfd,buffer,255);

	if (n < 0) 
	{
		perror("ERROR reading from socket");
		exit(1);
	}
	
	//check which protocol is being used and respond appropriately
	//(need to add security check for message lengths)
	if(buffer[4] == '\r' && buffer[5] == '\n'){
		if(buffer[0] == 'P' && buffer[2] == 'N' && buffer[3] == 'G'){
		
			if(buffer[1] == 'I'){
				n = write(newsockfd, "PONG\r\n" , 6);
			}
			else if(buffer[1] == 'O'){
				sprintf(error,"%s%*.*s\r\n", pong, 38-strlen(pong));
				n = write(newsockfd, error ,strlen(error));
			}
			else{
				sprintf(error,"%s%*.*s\r\n", almost, 38-strlen(almost));
				n = write(newsockfd, error ,strlen(error));
			}
		}
		else if(buffer[0] == 'O' && buffer[1] == 'K' && buffer[2] == 'A'
			&& buffer[3] == 'Y'){
			sprintf(error,"%s%*.*s\r\n", okay, 38-strlen(okay));
			n = write(newsockfd, error ,strlen(error));
		}
		else if(buffer[0] == 'E' && buffer[1] == 'R' && buffer[2] == 'R'
			&& buffer[3] == 'O'){
			sprintf(error,"%s%*.*s", erro, 38-strlen(erro));
			n = write(newsockfd, error ,strlen(error));
		}
		else{
			sprintf(error,"%s%*.*s", gen, 38-strlen(gen));
			n = write(newsockfd, error ,strlen(error));
		}
	}
	else{
		//error about delimiting 
		sprintf(error,"%s%*.*s", delim, 38-strlen(delim));
		n = write(newsockfd, error ,strlen(error));
	}

	
	if (n < 0) 
	{
		perror("ERROR writing to socket");
		exit(1);
	}
	}
	
	return NULL;
}