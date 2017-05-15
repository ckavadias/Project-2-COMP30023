/* A simple server in the internet domain using TCP
The port number is passed as an argument 


 To compile: gcc server.c -o server 
*/

#include "server.h"

int main(int argc, char **argv)
{
	int sockfd, newsockfd, portno, clilen, client = 0;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t clients[MAX_CLIENTS];
	
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
	while(1){
	newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, 
						&clilen);
	
	//call thread to handle new connection
	pthread_create(clients+client, NULL, receptionist, &newsockfd);
	client++;
	/* close socket */
	}
	close(sockfd);
	
	return 0; 
}
