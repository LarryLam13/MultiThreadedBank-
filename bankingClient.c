#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#include "functions.h"

int networkFD; // network socket file descriptor


//read messages from the Client and sends them to the Server
void * sender(void * input){
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int netSockFD = *(int *)input;
    int status;

    while( (status = read(netSockFD, buffer, sizeof(buffer)) ) > 0 ){
        printf("%s", buffer);
        memset(buffer, 0, sizeof(buffer));
    }
        printf("The Server is Closed\n");
    free(input);
    return 0;
}

int main(int argc, char *argv[]){

    struct addrinfo reqInfo;
    reqInfo.ai_flags = 0;
    reqInfo.ai_family = AF_INET;
    reqInfo.ai_socktype = SOCK_STREAM;
    reqInfo.ai_protocol = 0;
    reqInfo.ai_addrlen = 0;
    reqInfo.ai_canonname = NULL;
    reqInfo.ai_next = NULL;
   // ptr to to socket info 
    struct addrinfo *reqPtr;

    if(argv[1] ==NULL && gethostbyname(argv[1]) == NULL){
	printf("error: specify machine that hosts the server or bad host!\n");
	exit(1);
	 }

    if(argv[2] == NULL){
	printf("error: need the port number!\n");
	exit(1);
    }
	char * machine = argv[1];
	char * port = argv[2];
	char buffer[256];
        int netSockFD;
    // retrieve structures
    getaddrinfo(machine, port, &reqInfo, &reqPtr );

    void *socketarg = malloc(sizeof(netSockFD));
    // initialize network socket
    netSockFD = socket(reqPtr->ai_family, reqPtr->ai_socktype, reqPtr->ai_protocol);
    memcpy(socketarg, &netSockFD, sizeof(int));
    // connect to server socket
    int status = connect(netSockFD, reqPtr->ai_addr, reqPtr->ai_addrlen);
    while( status < 0 ){
        close(netSockFD);
        netSockFD = socket(reqPtr->ai_family, reqPtr->ai_socktype, reqPtr->ai_protocol);
        printf("Could not find server. Attempting to reconnect in 3 seconds\n");
        sleep(3);
        status = connect(netSockFD, reqPtr->ai_addr, reqPtr->ai_addrlen);
    }

    networkFD = netSockFD;
    printf("MultiThreaded Bank System\n");
	// spawns thread to handle User
    pthread_t server_user;
    pthread_create(&server_user, NULL, &sender, socketarg );
    pthread_detach(server_user);
    memset(buffer, 0, sizeof(buffer));
    while( read(0, buffer, sizeof(buffer)) > 0) {
        if ( (status = write(netSockFD, buffer, strlen(buffer) ) ) < 0){
               printf("Bank Server is closed try to connect again!\n");
               break;
        }
        //clear buffer
        memset(buffer, 0, sizeof(buffer));
        sleep(2);
    }

    return 0;
}
