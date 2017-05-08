/* A simple server in the internet domain using TCP
The port number is passed as an argument 


 To compile: gcc server.c -o server 
*/

#include "server.h"


int main(int argc, char **argv)
{
	int sockfd, newsockfd, portno, clilen;
	char buffer[256], message[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	
	if (argc < 2) 
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	 /* Create TCP socket */
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) 
	{
		perror("ERROR opening socket");
		exit(1);
	}

	
	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);
	
	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for 
	 this machine */
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	 /* Bind address to the socket */
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}
	
	/* Listen on socket - means we're ready to accept connections - 
	 incoming connection requests will be queued */
	
	listen(sockfd,5);
	
	clilen = sizeof(cli_addr);

	/* Accept a connection - block until a connection is ready to
	 be accepted. Get back a new file descriptor to communicate on. */

	newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, 
						&clilen);

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
	if(buffer[4] == '\r' && buffer[5] == '\n'){
		if(buffer[0] == 'P' && buffer[2] == 'N' && buffer[3] == 'G'){
		
			if(buffer[1] == 'I'){
				n = write(newsockfd, "PONG\r\n" , 6);
			}
			else if(buffer[1] == 'O'){
				n = write(newsockfd, pong_message ,strlen(pong_message));
			}
			else{
				n = write(newsockfd, almost_ping ,strlen(almost_ping));
			}
		}
		else if(buffer[0] == 'O' && buffer[1] == 'K' && buffer[2] == 'A'
			&& buffer[3] == 'Y'){
			n = write(newsockfd, okay_message ,strlen(okay_message));
		}
		else if(buffer[0] == 'E' && buffer[1] == 'R' && buffer[2] == 'R'
			&& buffer[3] == 'O'){
			n = write(newsockfd, erro_message ,strlen(erro_message));
		}
		else{
			n = write(newsockfd, erro_message ,strlen(erro_message));
		}
	}
	else{
		//error about delimiting 
		n = write(newsockfd, deli_message, strlen(deli_message));
	}

	
	if (n < 0) 
	{
		perror("ERROR writing to socket");
		exit(1);
	}

	/* close socket */
	
	close(sockfd);
	
	return 0; 
}
