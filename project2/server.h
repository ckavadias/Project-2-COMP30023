/* Project 2 COMP30023 Semester 1, 2017 Constanintos Kavadias, 664790
	ckavadias@unimelb.edu.au */
	
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
#define QUEUE_SIZE 10

//standard error messages
char* pong = "ERRO PONG is only for the server";
char* okay  = "ERRO OKAY is in fact not ok";
char* erro = "ERRO ERRO is not a joke, please don't";
char* gen = "ERRO Invalid protocol try again";
char* delim = "ERRO delimiting incorrect";
char* almost = "ERRO almost ping, check the spelling";
char* solution = "ERRO solution is incorrect";
char* abrt = "ERRO nothing to abort";

//structs for solution and work calls
typedef struct {
	int newsockfd;
	pthread_t thread_id;
	pthread_t* friends;
	uint32_t IP;
	int index;
	int offset;
	char* string;
}proof_t;

//shared parameters
int off = MAX_ERR - 2;
proof_t* work_queue[QUEUE_SIZE];
int next_ready = 0;
FILE* logfile = NULL;

//mutex paramters
sem_t log_mutex;
sem_t work_mutex;

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
	int newsockfd = ((proof_t*)param)->newsockfd, i, n, j = 0;
	char buffer[strlen(((proof_t*)param)->string) + 1];
	char seed_char[MAX_SEED + 1], error[MAX_ERR + 1], store[3];
	BYTE seed[MAX_SEED], input, two_num = 2;
	BYTE two[MAX_UINT], beta_256[MAX_UINT], exp[MAX_UINT], target[MAX_UINT];
	BYTE hash[SHA256_BLOCK_SIZE], hash2[SHA256_BLOCK_SIZE];
	uint32_t diff, alpha, beta, mask;
	uint64_t soln, mask_64;
	SHA256_CTX ctx, ctx2;
	
	strcpy(buffer, ((proof_t*)param)->string);
	store[2] = '\0';
	
	//initialise 2 in uint256_t form
	uint256_init(two);
	two[31] = two_num;
	uint256_init(beta_256);
	
	//scan string and store values appropriately
	sscanf(buffer, "SOLN %x %s %llx\r\n", &diff, seed_char, &soln);
	seed_char[64] = '\0';
	
	//scan char into unsigned char
	for(i = 0; i < strlen(seed_char); i+=2){
		
		store[0] = seed_char[i];
		store[1] = seed_char[i + 1];
		
		seed[j] = (BYTE)strtol(store, NULL, 16);
		j++;
	}
	
	//concatenate soln into seed
	for(i = 0; i < 8; i++){
		mask_64 = soln;
		mask_64 = mask_64<<(56 - 8*i);
		mask_64 = mask_64>>56;
		input = (BYTE)mask_64;
		seed[39 - i] = mask_64;
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
	uint256_exp(exp, two, 8*(alpha - 3));
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

//thread to add to queue to ensure mutex lock doesnt prevent repsonses
void* add_queue(void* param){
	int added = 0;
	proof_t copy = *((proof_t*)param);
	
	//The entry specified by next ready may not be empty yet but we can't
	//hold the lock as it will prevent any spaces from being freed
	//cycle through holding and letting go of the lock to allow other 
	//threads to grab the lock and process entries to free array space
	
	while(!added){
		
		sem_wait(&work_mutex);
		if(work_queue[next_ready] == NULL){
			copy.index = next_ready;
			work_queue[next_ready] = (proof_t*)malloc(sizeof(proof_t));
			*(work_queue[next_ready]) = copy;
			next_ready++;
			next_ready%=QUEUE_SIZE;
			added++;
		}
		sem_post(&work_mutex);
	}
	return NULL;
}

//worker thread to find a solution to a work problem 
void* worker(void* info){
	int newsockfd = ((proof_t*)info)->newsockfd,index = ((proof_t*)info)->index,
	i, n, j = 0, offset = ((proof_t*)info)->offset, num ;
	char buffer[strlen(((proof_t*)info)->string) + 1];
	char seed_char[MAX_SEED + 1], store[3], message[255];
	BYTE seed[MAX_SEED], input, two_num = 2;
	BYTE two[MAX_UINT], beta_256[MAX_UINT], exp[MAX_UINT], target[MAX_UINT];
	BYTE hash[SHA256_BLOCK_SIZE], hash2[SHA256_BLOCK_SIZE];
	uint32_t diff, alpha, beta, mask;
	uint64_t soln, mask_64;
	SHA256_CTX ctx, ctx2;

	strcpy(buffer, ((proof_t*)info)->string);
	store[2] = '\0';
	
	//initialise 2 in uint256_t form
	uint256_init(two);
	two[31] = two_num;
	
	uint256_init(beta_256);
	
	//scan string and store values appropriately
	sscanf(buffer, "WORK %x %s %llx %x\r\n", &diff, seed_char, &soln, &num);
	seed_char[64] = '\0';
	
	//scan char into unsigned char
	for(i = 0; i < strlen(seed_char); i+=2){
		
		store[0] = seed_char[i];
		store[1] = seed_char[i + 1];
		
		seed[j] = (BYTE)strtol(store, NULL, 16);
		j++;
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
	uint256_exp(exp, two, 8*(alpha - 3));
	uint256_mul(target, beta_256, exp);
	
	while(1){
		sem_wait(&work_mutex);
		if(work_queue[index] == NULL){
			return NULL;
		}
		sem_post(&work_mutex);
		
		soln+=offset;
		//fprintf(stderr, "trying soln: %llx\n", soln);
		//concatenate soln into seed
		for(i = 0; i < 8; i++){
			mask_64 = soln;
			mask_64 = mask_64<<(56 - 8*i);
			mask_64 = mask_64>>56;
			input = (BYTE)mask_64;
			seed[39 - i] = mask_64;
		}
	
		//hash seed
		sha256_init(&ctx);
		sha256_update(&ctx, seed, 40);
		sha256_final(&ctx, hash);
	
		//hash the hash
		sha256_init(&ctx2);
		sha256_update(&ctx2, hash, SHA256_BLOCK_SIZE);
		sha256_final(&ctx2, hash2);
		
		if( sha256_compare(hash2, target) < 0){
			//write necessary output
			sem_wait(&work_mutex);
			sprintf(message, "SOLN %x %s %llx\r\n", diff, seed_char, soln);
			n = write(newsockfd, message ,strlen(message));
		
			//remove from queue
			work_queue[index] = NULL;
			sem_post(&work_mutex);
			return NULL;
		}
		soln++;
	}
	return NULL;
}

//persistent worker thread to call worker
void* work_manager(){
	pthread_t current;
	int next = 0, n, i;
	uint32_t x;
	uint64_t y;
	char d[MAX_SEED + 1];
	
	while(1){
		//mutex check
		sem_wait(&work_mutex);
	
		//see if queue empty
		if(work_queue[next] != NULL){
			//if not take next job and call worker
            sscanf(work_queue[next]->string, "WORK %x %s %llx %x", &x,d,&y, &n);
            for( i = 0;  i < n; i++){
            	work_queue[next]->offset = i;
            	pthread_create(&current, NULL, worker, work_queue[next]);
            }
			//increment next_ready and modulus QUEUE_SIZE
			next++;
			next%=QUEUE_SIZE;
		}
		sem_post(&work_mutex);
	}
	
	return NULL;
}
//kill or worker threads relating to socket described by newsockfd
void* kill_them_all(void* num){
	int newsockfd = *((int*)num), i;
	int kills = 0;
	char error[MAX_ERR + 1];
	
	sem_wait(&work_mutex);
	for( i = 0; i < QUEUE_SIZE; i++){
		if(work_queue[i] != NULL && work_queue[i]->newsockfd == newsockfd){
			pthread_cancel(work_queue[i]->thread_id);
			work_queue[i] = NULL;
			kills++;
		}
	}
	if(kills){
		write(newsockfd, "OKAY\r\n", 6);
	}
	else{
		sprintf(error,"%s%*c\r\n", abrt, off-strlen(abrt),'#');
		write(newsockfd, error, MAX_ERR);
	}
	sem_post(&work_mutex);
	return NULL;
}

//thread caller to handle socket connection
void* receptionist(void* proof){
	int newsockfd, n = 1;
	char buffer[MAX_BUF + 1], passing[MAX_BUF + 1], error[MAX_ERR + 1];
	pthread_t thread;
	
	newsockfd = ((proof_t*)proof)->newsockfd;
	
	while(n != 0){
	
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
		strcpy(passing, buffer);
		
		((proof_t*)proof)->string = passing;
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
				pthread_create(&thread, NULL, is_solution, proof);
				}
			else if(buffer[0] == 'W' &&  buffer[1] == 'O' && buffer[2] == 'R'
				&& buffer[3] == 'K'){
				//call add queue to stop a mutex lock from halting thread
				pthread_create(&(((proof_t*)proof)->thread_id), NULL, 
					add_queue, proof);
				}
			else if(buffer[0] == 'A' && buffer[1] == 'B' && buffer[2] == 'R'
				&& buffer[3] == 'T'){
				// call thread to traverse work queue and remove items
				//this ensures other messages an still be recieved will aborting
				pthread_create(&thread, NULL, kill_them_all, &newsockfd);
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
		break;
		//exit(1);
	}
	}
	
	close(newsockfd);
	return NULL;
}

