#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "buffer.h"

#define NAME "buffer"

void show_info(int size, int pid, char * name, void *addr){
    printf("The process ID is: %d\n",pid);
    printf("Shared Memory stored as: %s\n",name);
    printf("Receiver mapped address: %p\n",addr);
    printf("The size of the buffer is: %d\n",size);
}
int main (int argc, char *argv[]){

    sem_t * sem_prod = sem_open(SEM_PRODUCER_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_prod == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }
    
    //Creating the full semaphore: This is used to count the number of full items in the buffer
    sem_t * sem_full = sem_open(SEM_FULL_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_full == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    int fd = shm_open(NAME, O_RDWR,0666);
    if (fd <0){
        perror("shm_open()");
        return EXIT_FAILURE;
    }

    void * addr = 
        (buffer *) mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("\033[22;33mreceiver mapped address: \033[22;37m%p\n",addr);

    buffer * buff = addr;
    //shm_unlink(NAME);
    


    //Signaling to stop working
    buff->work = false;
    sem_post(sem_prod);
    sem_post(sem_full);
    sleep(1); //Giving the programs time to finish what they are doing
    sem_post(sem_prod);
    /*-------------------Statistics------------------------*/


    print_stats(buff);


    /*------Closing Everything----------*/
    munmap(addr, sizeof(buffer));
    close(fd);
    shm_unlink(NAME);
    sem_unlink(SEM_CONSUMER_FNAME);
    sem_unlink(SEM_PRODUCER_FNAME);
    sem_unlink(SEM_FULL_FNAME);
    sem_unlink(SEM_EMPTY_FNAME);
    return EXIT_SUCCESS;
}

