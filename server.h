
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

//standard error messages
char* pong_message = "ERRO PONG is only for the sever\r\n";
char* okay_message = "ERRO OKAY is in fact not ok\r\n";
char* erro_message = "ERRO ERRO this an important please dont transmit\r\n";
char* gen_message = "ERRO Invalide protocol try again\r\n";
char* deli_message = "ERRO delimiting incorrect\r\n";
char* almost_ping = "ERRO just about had ping, check your spelling\r\n";