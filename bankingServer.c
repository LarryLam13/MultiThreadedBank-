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
 #include "functions.h"


 account **mainArr;
 int numAccount;
 pthread_mutex_t lock;


 void * clientHandler(void * socket){
      printf("Server has just accepted a client connection.\n");
      int client_socket_fd = *(int *) socket;
      // declare buffers
      char option[10];
      bzero(option,sizeof(option));

      char name[255];
      bzero(name,sizeof(name));

      char inputBuff[256];
      bzero(inputBuff,sizeof(inputBuff));

      char outputBuff[256];
      bzero(outputBuff,sizeof(outputBuff));
      // option state
      state mode;
     
      int session = -1;
  
   write(client_socket_fd, outputBuff, sprintf(outputBuff, "%s\n","OPTIONS: create|serve|withdraw|deposit|query|end|quit"));
       bzero(outputBuff,sizeof(outputBuff));
      // read in argurments in client 
      while(read(client_socket_fd, inputBuff, 255) > 0){
          sscanf(inputBuff, "%s %s",option, name);

          mode = getOption(option);

          switch(mode){
              case create:
                  accCreate(client_socket_fd,&numAccount,&mainArr,name,&session,&lock);
                  break;
              case serve:
                  accServe(client_socket_fd,&mainArr,name,&session);
                  break;
              case deposit:
                  accDeposit(client_socket_fd,&mainArr,name,&session);
                  break;
              case withdraw:
                  accWithdraw(client_socket_fd,&mainArr,name,&session);
                  break;
              case query:
                  accQuery(client_socket_fd,&mainArr,&session);
                  break;
              case end:
                  accEnd(client_socket_fd,&mainArr,&session);
                  break;
              case quit:     // exiting the session
                  if(session < 0){
                       write(client_socket_fd, outputBuff, sprintf(outputBuff, "Exited\n"));
                      printf("Server has closed a client connection\n");
                      close(client_socket_fd);
                      free(socket);
                      return 0;
                    }   


                    mainArr[session]->insession_flag = 0;
                    pthread_mutex_unlock(&(mainArr[session]->lock));
                    session = -1;
                      write(client_socket_fd, outputBuff, sprintf(outputBuff, "Exited\n") );
                          
                    printf("Server has closed a client connection\n");
                    close(client_socket_fd);
                    free(socket);
                  return 0;
              default:
                write(client_socket_fd, outputBuff, sprintf(outputBuff, "ERROR: Invalid argument"));
                  break;

          }
          write(client_socket_fd, outputBuff, sprintf(outputBuff, "%s\n","OPTIONS: create|serve|withdraw|deposit|query|end|quit"));
            
          // clear buffers
          bzero(inputBuff,sizeof(inputBuff));
          bzero(outputBuff,sizeof(outputBuff));
          bzero(option,sizeof(option));
          bzero(name,sizeof(name));
      }

      free(socket);
      return 0;
 }

 void *diagnostic(void * args){

      while(1){
          if(numAccount == 0){
              printf("No Account have been opened yet\n");
          }
	  else{
              int i;
              for(i = 0 ; i < 1000 ; i++){
                  account * diag = mainArr[i];
                  if(diag->insession_flag >= 0){
                      if(diag->insession_flag == 0){
                          printf("Account name: %s\t Balance: %f\t Service? %s\n",diag->name,diag->balance,"Not in use");
                      }else{
                          printf("Account name: %s\t Balance: %f\t Service? %s\n",diag->name,diag->balance,"currently in use");
                      }
                  }
              }
          }
          sleep(15);
      }
    }
 

 int main(int argc,char **argv) {
    // server socket file descriptor
    int server_socket_fd;
    // client socket file descriptor (that server receives)
    int client_socket_fd;
    // request structs we need for socket communication
    struct addrinfo request;
    request.ai_flags = AI_PASSIVE;
    request.ai_family = AF_INET;
    request.ai_socktype = SOCK_STREAM;
    request.ai_protocol = 0;
    request.ai_addrlen = 0;
    request.ai_canonname = NULL;
    request.ai_next = NULL;
    // will point to results
    struct addrinfo *result;
	if(argv[1] == NULL){
	printf("missing port number\n");
	exit(1);
	}
	char* port = argv[1];
    // retrieve structures
    getaddrinfo(0, port, &request, &result );

    // create socket
    server_socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
       
     // sets options 
	int optval = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR , &optval, sizeof(int));
    // binds sockets to a port 
    bind(server_socket_fd, result->ai_addr, result->ai_addrlen);

    initAccounts(&mainArr);
    pthread_mutex_init(&lock, NULL);
	//spaws a thread that will give a diagnostic on the server
    pthread_t get_account_info;
    pthread_create(&get_account_info,NULL,&diagnostic,NULL);

    listen(server_socket_fd,10); 

    void * threadArg;   
    pthread_t clientUser;
    
	while(1){

	client_socket_fd = accept(server_socket_fd, NULL, NULL);
           
        threadArg = malloc(sizeof(int));
        memcpy(threadArg, &client_socket_fd, sizeof(int));

        // create thread for every connection 
      pthread_create(&clientUser, NULL, &clientHandler,threadArg );
      pthread_detach(clientUser);



    }
 }
