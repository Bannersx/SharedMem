#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "buffer.h"
#include <string.h>


int main (int argc, char *argv[]){
    
    
    /*--- Checking if the number of arguments is correct ---*/
    if (argc != 3){
        printf("usage: -n [name of buffer] \nPlease use the full arguments\n");
        return -1;
    }

    printf("\033[22;32m*-------------------Working on the Finalizer Process----------------------*\033[22;37m\n\n");


    //printf("The process ID is: %d\n",pid);

    /*---Parsing the arguments & handling the flags---*/
    int count;
    char shm_name[20];
    char * size;
    for (count = 1; count < argc ; count++){
        switch (argv[count][1]) {
            case 'n': strcpy(shm_name,argv[count+1]);
            
        }
    } 
    //shm_unlink(shm_name);


    /**----------------------Working on the semaphores--------------------------**/
    char prod_sem[20] = SEM_PRODUCER_FNAME;
    strcat(prod_sem,shm_name);
    
    char empty_sem[20] = SEM_EMPTY_FNAME;
    strcat(empty_sem,shm_name);
    char full_sem[20] = SEM_FULL_FNAME;
    strcat(full_sem, shm_name);



     /*---Opening all the needed semaphores inside the process---*/
    printf("\n  -> Dealing with the semaphore: %s...\n", prod_sem);
    sem_t * sem_prod = sem_open(prod_sem, 0);
    //Checking if the semaphore was created succesfully
    if (sem_prod == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    //Creating the full semaphore: This is used to count the number of full items in the buffer
    printf("\n  -> Dealing with the semaphore: %s...\n", full_sem);
    sem_t * sem_full = sem_open(full_sem, 0);
    //Checking if the semaphore was created succesfully
    if (sem_full == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    //Creating the empty semaphore: This is used to keep track of the empty number of elements in the buffer.
    printf("\n  -> Dealing with the semaphore: %s...\n", empty_sem);
    sem_t * sem_empty = sem_open(empty_sem,0);
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
    printf("\n    \033[22;33mReceiver mapped address: \033[22;37m%p\n",addr);

    buffer * buff = addr;
    //shm_unlink(shm_name);    
    
    
    printf("\n    \033[22;33mGathering information.... \033[22;37m\n\n");

    //Signaling to stop working
    buff->work = false;

    
    while (buff->cur_prod > 0 || buff->cur_cons > 0)
    {
        if (buff->cur_prod == 0){
            sem_post(sem_prod);
            sem_post(sem_full);
        }else if( buff->cur_cons == 0) {
            sem_post(sem_prod);
            sem_post(sem_empty);
        }else{
            sem_post(sem_prod);
            sem_post(sem_full);
            sem_post(sem_empty);

        }
    }

    
    sleep(3); //Givint time to update information

    /*-------------------Statistics------------------------*/
    print_stats(buff);

    
    
    sleep(1); //Giving the programs time to finish what they are doing
    

    /*------Closing Everything----------*/
    munmap(addr, sizeof(buffer));
    close(fd);
    shm_unlink(shm_name);
    sem_unlink(prod_sem);
    sem_unlink(full_sem);
    sem_unlink(empty_sem);
    sem_close(sem_prod);
    sem_close(sem_full);
    sem_close(sem_empty);
    return EXIT_SUCCESS;
}

