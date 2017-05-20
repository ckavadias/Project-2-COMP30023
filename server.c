/* A simple server in the internet domain using TCP
The port number is passed as an argument 


 To compile: gcc server.c -o server 
*/

#include "server.h"

int main(int argc, char **argv)
{
	int sockfd, newsockfd, portno, clilen, i = 0, j;
	struct sockaddr_in serv_addr, cli_addr;
	proof_t clients[MAX_CLIENTS];
	pthread_t workers;
	
	//open log and initialise semaphores
	log_open();
	sem_init(&log_mutex, 0, 1);
	sem_init(&work_mutex, 0, 1);

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
	
	//initialise work queue to null
	for(j = 0; j < QUEUE_SIZE; j++){
		work_queue[j] = NULL;
	}
	
	pthread_create(&workers, NULL, work_manager, NULL);
	
	while(1){
		/* Listen on socket - means we're ready to accept connections - 
		incoming connection requests will be queued */
	
		listen(sockfd,MAX_CLIENTS);
		
		clilen = sizeof(cli_addr);

		/* Accept a connection - block until a connection is ready to
		be accepted. Get back a new file descriptor to communicate on. */
		newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, 
						&clilen);
		//call thread to handle new connection
		clients[i].newsockfd = newsockfd;
		clients[i].IP = cli_addr.sin_addr.s_addr;
	   pthread_create(&(clients[i].thread_id), NULL, receptionist, clients + i);
	   i++;
	}
	
	/* close socket */
	close(sockfd);
	
	return 0; 
}
