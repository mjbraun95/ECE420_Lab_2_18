/*
*
*
*
* Compile server: gcc server.c -o server -lpthread
* run server: ./server
* Compile client: gcc client.c -o client -lpthread
* Run client: ./client 1024 127.0.0.1 3000
* Compile attacker: gcc attacker.c -o attacker -lpthread
* Run attacker client: ./attacker 1024 127.0.0.1 3000
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

int thread_count;  
char **theArray; 
pthread_mutex_t mutex;

/*-------------------------------------------------------------------*/
void *Operate(void* rank) {
    long my_rank = (long) rank;
    char buffer[STR_LEN]; // buffer to read and write

    ClientRequest request;

    /* Start of critical section*/
    pthread_mutex_lock(&mutex); 
    // write the array string from the client to the server's array

    /* Read client's message from the socket*/
    read(my_rank,buffer,STR_LEN);

    /* Parse the message and save it to the request struct*/
    ParseMsg(buffer, &request);

    if (!request.is_read){ // If the client sent a write request
        printf("Thread %ld: writing \"%s\" at theArray[%d] \n\n", my_rank, request.msg, request.pos);
        setContent(buffer, request.pos, (char**)theArray); // defined in "common.h"
        sprintf(buffer,"SUCCESS: theArray[%d] has been set to: \"%s\"\n", request.pos, request.msg);
        write(my_rank,buffer,sizeof(buffer));
    }

    /* If the client sent a read request */
    else if (request.is_read){
        getContent(buffer, request.pos, (char**)theArray); // defined in "common.h"
        sprintf(buffer,"theArray[%d]: \"%s\"\n\n", request.pos, buffer); // display the result
        write(my_rank,buffer,sizeof(buffer));
    }

    // End of critical section, unlock the mutex
    pthread_mutex_unlock(&mutex);

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
    double start, finish, elapsed;  


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

    sock_var.sin_addr.s_addr=inet_addr("127.0.0.1");
    sock_var.sin_port=3000;
    sock_var.sin_family=AF_INET;
    if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0)
    {
        printf("socket has been created\n");
        listen(serverFileDescriptor,2000); 
        
        while(1)        //loop infinity
        {
            GET_TIME(start); 
            for(i=0;i<COM_NUM_REQUEST;i++)      //can support 20 clients at a time
            {
                clientFileDescriptor=accept(serverFileDescriptor,NULL,NULL);
                //process client request
                printf("Connected to client %d\n",clientFileDescriptor);
                pthread_create(&thread_handles[i],NULL,Operate,(void *)(long)clientFileDescriptor);

            }

            for (i = 0; i < COM_NUM_REQUEST; i++) 
                pthread_join(thread_handles[i], NULL); 
                GET_TIME(finish);
                elapsed = finish - start;   
                printf("The elapsed time is %e seconds\n", elapsed);
        }
        
            
        pthread_mutex_destroy(&mutex);

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