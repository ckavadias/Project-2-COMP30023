
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
#define MAX_ERR 40

//standard error messages
char* pong = "ERRO PONG is only for the server";
char* okay  = "ERRO OKAY is in fact not ok";
char* erro = "ERRO ERRO is not a joke, please don't";
char* gen = "ERRO Invalid protocol try again";
char* delim = "ERRO delimiting incorrect";
char* almost = "ERRO almost ping, check the spelling";

//thread caller to handle socket connection
void* receptionist(void* param){
	int newsockfd, n = 1, off = MAX_ERR - 2;
	char buffer[256], error[MAX_ERR + 1];
	
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
	if(n > 0){
		if(buffer[4] == '\r' && buffer[5] == '\n'){
			if(buffer[0] == 'P' && buffer[2] == 'N' && buffer[3] == 'G'){
		
				if(buffer[1] == 'I'){
					n = write(newsockfd, "PONG\r\n" , 6);
				}
				else if(buffer[1] == 'O'){
					sprintf(error,"%s%*c\r\n", pong, off-strlen(pong),'#');
					n = write(newsockfd, error ,MAX_ERR);
				}
				else{
					sprintf(error,"%s%*c\r\n", almost, off-strlen(almost),'#');
					n = write(newsockfd, error ,MAX_ERR);
				}
			}
			else if(buffer[0] == 'O' && buffer[1] == 'K' && buffer[2] == 'A'
				&& buffer[3] == 'Y'){
					sprintf(error,"%s%*c\r\n", okay, off-strlen(okay),'#');
					n = write(newsockfd, error ,MAX_ERR);
				}
				else if(buffer[0] == 'E' && buffer[1] == 'R' && buffer[2] == 'R'
					&& buffer[3] == 'O'){
					sprintf(error,"%s%*c", erro, off-strlen(erro),'#');
					n = write(newsockfd, error ,MAX_ERR);
					}
					else{
						sprintf(error,"%s%*c", gen, off-strlen(gen),'#');
						n = write(newsockfd, error ,MAX_ERR);
					}
		}
		else{
			//error about delimiting 
			sprintf(error,"%s%*c", delim, off-strlen(delim), '#');
			n = write(newsockfd, error ,MAX_ERR);
		}
	}

	
	if (n < 0) 
	{
		perror("ERROR writing to socket");
		exit(1);
	}
	}
	
	close(newsockfd);
	return NULL;
}