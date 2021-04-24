#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "buffer.h"


int main (int argc, char *argv[]){

    
    /*--- Checking if the number of arguments is correct ---*/
    if (argc != 3){
        printf("usage: -n [name of buffer] -s [size of buffer]\nPlease use the full arguments\n");
        return -1;
    }

    printf("\033[22;32m*-------------------Working on the Finalizer Process----------------------*\n\n");


    //printf("The process ID is: %d\n",pid);

    /*---Parsing the arguments & handling the flags---*/
    int count;
    char * shm_name;
    char * size;
    for (count = 1; count < argc ; count++){
        switch (argv[count][1]) {
            case 'n': shm_name = argv[count+1];
            
        }
    } 


     /*---Opening all the needed semaphores inside the process---*/
    printf("\n  -> Opening the semaphore: %s...\n", SEM_PRODUCER_FNAME);
    sem_t * sem_prod = sem_open(SEM_PRODUCER_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_prod == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    //Creating the full semaphore: This is used to count the number of full items in the buffer
    printf("\n  -> Opening the semaphore: %s...\n", SEM_FULL_FNAME);
    sem_t * sem_full = sem_open(SEM_FULL_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_full == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    //Creating the empty semaphore: This is used to keep track of the empty number of elements in the buffer.
    printf("\n|--> Creating the semaphore: %s...\n", SEM_EMPTY_FNAME);
    sem_t * sem_empty = sem_open(SEM_EMPTY_FNAME,0);
    //Checking if the semaphore was created succesfully
    if (sem_empty == SEM_FAILED){
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }

    int fd = shm_open(shm_name, O_RDWR,0666);
    if (fd <0){
        perror("shm_open()");
        return EXIT_FAILURE;
    }

    void * addr = 
        (buffer *) mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("    \033[22;33mreceiver mapped address: \033[22;37m%p\n",addr);

    buffer * buff = addr;
    //shm_unlink(NAME);
    


    //Signaling to stop working
    buff->work = false;
    sem_post(sem_prod);
    sem_post(sem_full);
    sem_post(sem_empty);
    
    sleep(1); //Giving the programs time to finish what they are doing
    
    /*-------------------Statistics------------------------*/


    print_stats(buff);


    /*------Closing Everything----------*/
    munmap(addr, sizeof(buffer));
    close(fd);
    shm_unlink(shm_name);
    sem_unlink(SEM_CONSUMER_FNAME);
    sem_unlink(SEM_FULL_FNAME);
    sem_unlink(SEM_EMPTY_FNAME);
    return EXIT_SUCCESS;
}

