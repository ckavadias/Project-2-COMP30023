
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "sha256.h"
#include "uint256.h"
#include <time.h>
#include <semaphore.h>

//Provided Limitations 
#define MAX_CLIENTS 100
#define MAX_ERR 40
#define MAX_BUF 255
#define MAX_SEED 64
#define MAX_UINT 32

//standard error messages
char* pong = "ERRO PONG is only for the server";
char* okay  = "ERRO OKAY is in fact not ok";
char* erro = "ERRO ERRO is not a joke, please don't";
char* gen = "ERRO Invalid protocol try again";
char* delim = "ERRO delimiting incorrect";
char* almost = "ERRO almost ping, check the spelling";
char* solution = "ERRO solution is incorrect";

//shared parameters
int off = MAX_ERR - 2;
FILE* logfile = NULL;

//mutex paramters
sem_t log_mutex;
sem_t queue_mutex;

//structs for solution and work calls
typedef struct {
	int newsockfd;
	pthread_t thread_id;
	uint32_t IP;
	char* string;
}proof_t;

/*log file functions*/

//open log file
void log_open(){
	logfile = fopen("log.txt", "w");
}

//adapted from http://stackoverflow.com/questions/9596945/how-to-get-
//appropriate-timestamp-in-c-for-logs
void log_write(proof_t* info){
	time_t ltime;
	uint32_t one, two, three, four;
	
    ltime=time(NULL);
    
    //convert IP address to useful format
    one = info->IP>>24;
    two = info->IP<<8;
    two = two>>24;
    three = info->IP<<16;
    three = three>>24;
    four = info->IP<<24;
    four = four>>24;
    
    //set up mutex first to avoid conflicted prints
    sem_wait(&log_mutex);
    
    fprintf(logfile, "%s IP:%u.%u.%u.%u , Socket ID: %d , Message: %s\n ",
    	asctime(localtime(&ltime)),four, three, two, one ,info->newsockfd, 
    		info->string);
    
    //now flush to avoid data loss
    fflush(logfile);
    
    sem_post(&log_mutex);
}

/* Thread functions */
//thread function to handle proof of work for solution messages
void* is_solution(void* param){
	int newsockfd = ((proof_t*)param)->newsockfd, i, n;
	char* buffer = ((proof_t*)param)->string;
	char seed_char[MAX_SEED + 1], error[MAX_ERR + 1];
	BYTE seed[MAX_SEED], input, two_num = 2;
	BYTE two[MAX_UINT], beta_256[MAX_UINT], exp[MAX_UINT], target[MAX_UINT];
	BYTE hash[SHA256_BLOCK_SIZE], hash2[SHA256_BLOCK_SIZE];
	uint32_t diff, alpha, beta, mask;
	uint64_t soln, mask_64;
	SHA256_CTX ctx, ctx2;
	
	//initialise 2 in uint256_t form
	uint256_init(two);
	two[31] = two_num;
	
	uint256_init(beta_256);
	
	
	//scan string and store values appropriately
	sscanf(buffer + 5, "%x %s %llx\r\n", &diff, seed_char, &soln);
	seed_char[32] = '\n';
	
	//scan char into unsigned char
	sscanf(seed_char, "%hhu\n", seed);
	
	//concatenate soln into seed
	for(i = 0; i < 8; i++){
		mask_64 = soln;
		mask_64 = mask_64<<(56 - 8*i);
		mask_64 = mask_64>>56;
		input = (BYTE)mask_64;
		seed[37 - i] = mask_64;
	}
	
	//use right shift to find alpha
	alpha = diff;
	alpha = alpha>>24;
	
	//use left shift followed by right shift to delete alpha and find beta
	//insert into uint256
	beta = diff;
	beta = beta<<8;
	beta = beta>>8;
	
	//add BYTE segments of beta to beta_256
	for(i = 0; i < 4; i++){
		mask = beta;
		mask = mask<<(24 - 8*i);
		mask = mask>>24;
		input = (BYTE)mask;
		beta_256[31 - i] = input;
	}
	
	//take exponential result, multiply by beta
	uint256_exp(exp, two, alpha - 3);
	uint256_mul(target, beta_256, exp);
	
	//hash seed
	sha256_init(&ctx);
	sha256_update(&ctx, seed, 40);
	sha256_final(&ctx, hash);
	
	//hash the hash
	sha256_init(&ctx2);
	sha256_update(&ctx2, hash, SHA256_BLOCK_SIZE);
	sha256_final(&ctx2, hash2);
	
	if( sha256_compare(hash2, target) < 0){
		n = write(newsockfd, "OKAY\r\n" ,6);
	}
	else{
		sprintf(error,"%s%*c\r\n", solution, off-strlen(solution),'#');
					n = write(newsockfd, error ,MAX_ERR);
	}
	return NULL;
}

//thread caller to handle socket connection
void* receptionist(void* proof){
	int newsockfd, n = 1;
	char buffer[MAX_BUF + 1], error[MAX_ERR + 1];
	pthread_t thread;
	
	newsockfd = ((proof_t*)proof)->newsockfd;
	
	while(n != 0){
	if (newsockfd < 0) 
	{
		perror("ERROR on accept");
		exit(1);
	}
	
	bzero(buffer,MAX_BUF + 1);

	/* Read characters from the connection,
		then process */
	
	n = read(newsockfd,buffer,MAX_BUF);

	if (n < 0) 
	{
		perror("ERROR reading from socket");
		exit(1);
	}
	
	//check which protocol is being used and respond appropriately
	//(need to add security check for message lengths)
	if(n > 0){
		buffer[n] = '\0';
		((proof_t*)proof)->string = buffer;
		log_write(proof);
		
	   if(buffer[strlen(buffer) - 2] == '\r'&&buffer[strlen(buffer)-1] == '\n'){
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
					sprintf(error,"%s%*c\r\n", erro, off-strlen(erro),'#');
					n = write(newsockfd, error ,MAX_ERR);
			}
			else if(buffer[0] == 'S' && buffer[1] == 'O' && buffer[2] == 'L'
				&& buffer[3] == 'N'){
				//solve with a new thread
				//send with a struct containing newsockfd and solution string
				pthread_create(&thread, NULL, is_solution, &proof);
				
				}
			else{
				 sprintf(error,"%s%*c\r\n", gen, off-strlen(gen),'#');
				 n = write(newsockfd, error ,MAX_ERR);
			}
		}
		else{
			//error about delimiting 
			sprintf(error,"%s%*c\r\n", delim, off-strlen(delim), '#');
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

