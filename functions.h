#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "structs.h"


int isFloat(char * amount){
    int periodCount = 0;
    int len = strlen(amount);
    int i;
    for(i = 0 ; i < len ; i++){
        if(isdigit(amount[i]) == 0){
            if(amount[i] == '.'){
                periodCount++;
                if(periodCount > 1){
                    return 0;
                }
            }
            else{
            return 0;
            }
        }
    }
    return 1;
}


state getOption(char * command){
    int i;
    for(i = 0; i < strlen(command); i++){
        command[i] = tolower(command[i]);
    }

    if(strcmp(command,"create") == 0){
        return create;
    }else if(strcmp(command,"serve") == 0){
        return serve;
    }else if(strcmp(command,"deposit") == 0){
        return deposit;
    }else if(strcmp(command,"withdraw") == 0){
        return withdraw;
    }else if(strcmp(command,"query") == 0){
        return query;
    }else if(strcmp(command,"end") == 0){
        return end;
    }else if(strcmp(command,"quit") == 0){
        return quit;
    }else{
      return cont;
    }

    return cont;
}


void initAccounts(account ***mainArr){
    *mainArr = (account ** ) malloc(sizeof(account *)*1000);
    int i;
    for(i = 0 ; i < 1000 ; i++){
        (*mainArr)[i] = (account *) malloc(sizeof(account));
        ((*mainArr)[i])->insession_flag = -1;
        pthread_mutex_init(&(((*mainArr)[i])->lock), NULL);
    }
    return;
}


int searchAccount(account ** mainArr, char * name){
    int i;
    for(i = 0 ; i < 20 ; i++){
        if(strcmp( (mainArr[i])->name ,name) == 0){
          return i;
        }
    }
    return -1;
}


void accCreate(int client_socket_fd, int *numAccount, account ***mainArr, char * name, int * session, pthread_mutex_t * lock){
  
    pthread_mutex_lock(lock);
 
    char outputBuff[256];
    memset(outputBuff, 0, sizeof(outputBuff));
    
    if(strlen(name) <= 0){
      if ( write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: Must specify name of account\n") ) < 0 ){
          pthread_mutex_unlock(lock);
          exit(1);
      }
      pthread_mutex_unlock(lock);
      return;
    }
  
    if(*session >= 0){
        if ( write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: Already in session. Can't open an account\n") ) < 0 ){
            pthread_mutex_unlock(lock);
            exit(1);
        }
        pthread_mutex_unlock(lock);
        return;
    }
 
  
    if(searchAccount(*mainArr,name) >= 0 ){
        if ( write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: Account exists already\n") ) < 0 ){
            pthread_mutex_unlock(lock);
            exit(1);
        }
        pthread_mutex_unlock(lock);
        return;
    }
  
    account * client_account = (*mainArr)[*numAccount];

    strcpy(client_account->name, name);   
    client_account->balance = 0.0;       
    client_account->insession_flag = 0;    

    if ( write(client_socket_fd, outputBuff, sprintf(outputBuff, "Account has been created!\n") ) < 0 ){
        pthread_mutex_unlock(lock);
        exit(1);
    }

    // update # of accounts
    (*numAccount) = (*numAccount) + 1;
    // release lock for other clients to use
    pthread_mutex_unlock(lock);
    return;

}

void accServe(int client_socket_fd,  account ***mainArr, char * name, int *session){
    char outputBuff[256];
    memset(outputBuff, 0, sizeof(outputBuff));
    // check if name is given
    if(strlen(name) <= 0){
       write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: need account name\n"));
      return;
    }
    // session is active. user can't create an account while session is active.
    if(*session >= 0){
    write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: Already in session!\n"));
             return;
    }

    // account does not exist
    if( (*session = searchAccount(*mainArr,name)) < 0 ){
     write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: Account not doesn't exist\n"));   
        return;
    }

    // identify account
    account * client_account = (*mainArr)[*session];

   
    if( pthread_mutex_trylock(&(client_account->lock)) != 0 ){   
       write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error:The account is currently being used"));
        *session = -1;    
          return;
    }

    client_account->insession_flag = 1;
    write(client_socket_fd, outputBuff, sprintf(outputBuff, "Your session is active!\n"));
    return;

}

void accDeposit(int client_socket_fd, account ***mainArr, char * amount, int *session){
    char outputBuff[256];
    bzero(outputBuff,sizeof(outputBuff));
    // check if name is given
    if(strlen(amount) <= 0){
       write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: Must specify amount to deposit.\n")); 
        return;
    }
    if(!isFloat(amount)){
     write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error not a valid float\n"));
        return;
    }

    // check if session exists
    if(*session < 0){
       write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: must be in serve mode \n"));
                           return;
    }

    float depositval = atof(amount);

    account * client_account = (*mainArr)[*session];
    client_account->balance = (client_account->balance) + depositval;

 write(client_socket_fd, outputBuff, sprintf(outputBuff, "You have deposited into your account\n"));
      
    return;

}

void accWithdraw(int client_socket_fd, account ***mainArr, char * amount, int *session){
    char outputBuff[256];
    memset(outputBuff, 0, sizeof(outputBuff));

    if(strlen(amount) <= 0){
       write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: Must specify amount to withdraw.\n"));
        return;
    }
    
    if(!isFloat(amount)){
     write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: number entered must be a double and not negative \n") );
        return;
    }

  
    if(*session < 0){
       write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: must be in serve mode.\n") );    
        return;
    }

    float withdrawval = atof(amount);

    account * client_account = (*mainArr)[*session];

    if(withdrawval > client_account->balance){
      write(client_socket_fd, outputBuff, sprintf(outputBuff, "ERROR: You cannot withdraw more than your balance.\n"));
      return;
    }

    client_account->balance = (client_account->balance) - withdrawval;
    write(client_socket_fd, outputBuff, sprintf(outputBuff, "You have withdrawn from your account\n"));
    return;

}

void accQuery(int client_socket_fd,  account ***mainArr, int *session){
    char outputBuff[256];
    bzero(outputBuff,sizeof(outputBuff));

    if(*session < 0){
        write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: must be in serve mode\n"));
        return;
    }

    float balance = (*mainArr)[*session]->balance;

 write(client_socket_fd, outputBuff, sprintf(outputBuff, "Your balance is: %f\n",balance));
    return;
}


void accEnd(int client_socket_fd, account ***mainArr,  int *session){
    char outputBuff[256];
    memset(outputBuff, 0, sizeof(outputBuff));
    if(*session < 0){
        write(client_socket_fd, outputBuff, sprintf(outputBuff, "Error: must be in serve mode\n") );
        return;
    }

    account * client_account = (*mainArr)[*session];
    char * name = client_account->name;
    write(client_socket_fd, outputBuff, sprintf(outputBuff, "you have exited your account!\n"));   
    client_account->insession_flag = 0;
    *session = -1;
    pthread_mutex_unlock(&(client_account->lock));
    return;

}
