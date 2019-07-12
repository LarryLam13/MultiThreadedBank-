
#include <pthread.h>


    typedef enum state_ {
        create,
        serve,
        deposit,
        withdraw,
        query,
        end,
        quit,
        cont,
    } state;

    typedef struct account_ {
        char name[255];
        float balance;
        int insession_flag;              // -1: Not created, 1: In service,  0: Not in service
        pthread_mutex_t lock;
    } account;


