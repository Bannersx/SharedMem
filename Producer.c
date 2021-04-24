#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "buffer.h"
#include <time.h>

#define NAME "buffer"


//Structure used to map the shared memory
typedef struct {
    //Buffer related variables
    int max_size;
    int head;
    int tail;
    int read;
    char message[50];

    //Control related variables
    int producers;
    int consumers;
    bool work;

} buffer;


int main (int argc, char *argv[]){


    pid_t prod_pid = getpid();
/*------------------------Working on the semaphores---------------------------*/
  
 

 /*---Opening all the needed semaphores inside the process---*/

    sem_t * sem_prod = sem_open(SEM_PRODUCER_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_prod == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    
    sem_t * sem_cons = sem_open(SEM_CONSUMER_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_cons == SEM_FAILED){
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }

    //Creating the full semaphore: This is used to count the number of full items in the buffer
    printf("\n|--> Creating the semaphore: %s...\n", SEM_FULL_FNAME);
    sem_t * sem_full = sem_open(SEM_FULL_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_full == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    //Creating the empty semaphore: This is used to keep track of the empty number of elements in the buffer.
    printf("\n|--> Creating the semaphore: %s...\n", SEM_EMPTY_FNAME);
    sem_t * sem_empty = sem_open(SEM_CONSUMER_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_cons == SEM_FAILED){
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }
 /*---------------------All semaphores are up and running---------------------*/

    int fd = shm_open(NAME, O_RDWR,0666);
    if (fd <0){
        perror("shm_open()");
        return EXIT_FAILURE;
    }

    void * addr = 
        (buffer *) mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("receiver mapped address: %p\n",addr);

    buffer * buff = addr;

    printf("%d\n",buff->max_size);
    buff->head = 1;
    add_prod(buff);


    //buff->work = true;
       

    while (buff->work){
        
        int fd = shm_open(NAME, O_RDWR,0666);
        if (fd <0){
            perror("shm_open()");
            return EXIT_FAILURE;
        }

        void * addr = 
            (buffer *) mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        printf("receiver mapped address: %p\n",addr);

        buff = addr;
        
        printf("The size is: %d\n",buff->max_size);
        printf("The head is: %d\n\n",buff->head);

        sleep(5);
    }
/*-------Closing things nicely-------*/
    munmap(addr, sizeof(buffer));
    close(fd);
    //shm_unlink(NAME);
    return EXIT_SUCCESS;
}
