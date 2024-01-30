/*
*
*
*
* Compile server: gcc server.c -o server -lpthread
* run server: ./server
* Compile client: gcc client.c -o client -lpthread
* Run client: ./client 1024 127.0.0.1 3000
* Compile attacker: gcc attacker.c -o attacker -lpthread -lm
* Run attacker client: ./attacker 1024 127.0.0.1 3000
* sed -i -e 's/\r$//' scriptname.sh
*
*/

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include "common.h"
#include "timer.h"
#define NUM_STR 1024
#define STR_LEN 1000

#define MAX_REQUESTS 1000  // Assuming COM_NUM_REQUEST is 1000
double latencies[MAX_REQUESTS];  // Array to store latencies of each request

int thread_count;  
char **theArray; 
pthread_mutex_t mutex;
// pthread_rwlock_t rwlock;


/*-------------------------------------------------------------------*/
void *Operate(void* rank) {
    long my_rank = (long) rank;
    long index = (long)rank; // Assuming index is passed as rank
    char buffer[STR_LEN]; // buffer to read and write
    double start, finish, elapsed;
    double start_lock, finish_lock, elapsed_lock;

    ClientRequest request;

    pthread_mutex_lock(&mutex); 

    /* Start of critical section*/
    // GET_TIME(start_lock);
    // GET_TIME(finish_lock);
    // elapsed_lock = finish_lock - start_lock;
    // printf("Mutex lock time: %e seconds\n", elapsed_lock);
    // write the array string from the client to the server's array

    /* Read client's message from the socket*/
    read(my_rank,buffer,STR_LEN);

    /* Parse the message and save it to the request struct*/
    ParseMsg(buffer, &request);

    // Acquire the appropriate lock
    // if (!request.is_read) { 
    //     pthread_rwlock_wrlock(&rwlock); // For write request
    // } else {
    //     pthread_rwlock_rdlock(&rwlock); // For read request
    // }

    GET_TIME(start);

    // Critical section start
    if (request.is_read) {
        getContent(buffer, request.pos, (char**)theArray); // defined in "common.h"
        printf("Thread %ld sent a READ request for theArray[%d]: \"%s\"\n", my_rank, request.pos, buffer);
        write(my_rank,buffer,sizeof(buffer));
    }
    
    else if (!request.is_read) { // If the client sent a write request
        printf("Thread %ld: requests WRITE \"%s\" at theArray[%d] \n\n", my_rank, request.msg, request.pos);
        setContent(request.msg, request.pos, (char**)theArray); // defined in "common.h"

        // The buffer should have the same string that the client requested to be put in theArray[reuquest.pos]
        getContent(buffer, request.pos, (char**)theArray);
        printf("UPDATED! theArray[%d] now contains: \"%s\"", request.pos, buffer);

        write(my_rank,buffer,sizeof(buffer));
    }
    // Critical section end

    GET_TIME(finish); // Finish timing here
    elapsed = finish - start;
    printf("Request handling time: %e seconds\n", elapsed);
    latencies[index] = elapsed; // Store the latency

    // End of critical section, unlock the mutex
    pthread_mutex_unlock(&mutex);
    // pthread_rwlock_unlock(&rwlock); // Release the lock

    close(my_rank); 

    // ServerEcho(rank);
    return NULL;
}


int main(int argc, char* argv[])
{
    struct sockaddr_in sock_var;
    int serverFileDescriptor=socket(AF_INET,SOCK_STREAM,0);
    long clientFileDescriptor;
    int i;
    pthread_t* thread_handles;
    // double start, finish, elapsed;  


    /* Create the memory and fill in the initial values for theArray */
    theArray = (char**) malloc(NUM_STR * sizeof(char*));
    for (i = 0; i < NUM_STR; i ++){
        theArray[i] = (char*) malloc(STR_LEN * sizeof(char));
        sprintf(theArray[i], "theArray[%d]: initial value", i);
        printf("%s\n\n", theArray[i]);
    }

    // Allocate maximum number of requests as the size of the thread_handles
    thread_handles = malloc (COM_NUM_REQUEST*sizeof(pthread_t)); 
    pthread_mutex_init(&mutex, NULL);
    // pthread_rwlock_init(&rwlock, NULL);

    sock_var.sin_addr.s_addr=inet_addr("127.0.0.1");
    sock_var.sin_port=8000;
    sock_var.sin_family=AF_INET;
    if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0)
    {
        printf("socket has been created\n");
        listen(serverFileDescriptor,COM_NUM_REQUEST); 
        
        while(1)        //loop infinity
        {
            // GET_TIME(start); 
            for(i=0;i<COM_NUM_REQUEST;i++)      //can support 20 clients at a time
            {
                clientFileDescriptor=accept(serverFileDescriptor,NULL,NULL);
                //process client request
                printf("----------------------------------------------------------");
                printf("Accepting requests from client %d\n",clientFileDescriptor);
                pthread_create(&thread_handles[i],NULL,Operate,(void *)(long)clientFileDescriptor);

            }

            for (i = 0; i < COM_NUM_REQUEST; i++) 
                pthread_join(thread_handles[i], NULL); 
            // GET_TIME(finish);
            // elapsed = finish - start;   
            // printf("The elapsed time is %e seconds\n", elapsed);
            // After all threads are joined
            printf("All threads joined\n");
            saveTimes(latencies, MAX_REQUESTS);
        }


            
        pthread_mutex_destroy(&mutex);
        // pthread_rwlock_destroy(&rwlock);

        // Free Dynamic arrays
        free(thread_handles);
        for (i=0; i<NUM_STR; ++i){
            free(theArray[i]);
        }
        free(theArray);
        close(serverFileDescriptor);
    }
    else{
        printf("socket creation failed\n");
    }
    return 0;
}