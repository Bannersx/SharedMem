#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "buffer.h"
#include <time.h>
#include "buffer.h"

int main (int argc, char *argv[]){


    pid_t prod_pid = getpid();

    /*--- Checking if the number of arguments is correct ---*/
    if (argc != 5){
        printf("\nusage: -n [name of buffer] -m [a-> automatic|m-> manual]\nPlease use the full arguments\n");
        return -1;
    }

    /*---Parsing the arguments & handling the flags---*/
    int count;
    char * shm_name;
    char * mode;
    for (count = 1; count < argc ; count++){
        switch (argv[count][1]) {
            case 'n': shm_name = argv[count+1];
            case 'm': mode = argv[count+1];
        }
    } 
/*------------------------Working on the semaphores---------------------------*/



  
  /*---Opening all the needed semaphores inside the process---*/

    sem_t * sem_prod = sem_open(SEM_PRODUCER_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_prod == SEM_FAILED){
        perror("sem_open/producer");
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
    sem_t * sem_empty = sem_open(SEM_EMPTY_FNAME, 0);
    //Checking if the semaphore was created succesfully
    if (sem_empty == SEM_FAILED){
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }
 /*---------------------All semaphores are up and running---------------------*/
/*---------------------------------------------------------------------------------*/

 /*-------------------------Shared Memory-------------------------------------*/
    int fd = shm_open(shm_name, O_RDWR,0666);
    if (fd <0){
        perror("shm_open()");
        return EXIT_FAILURE;
    }

    void * addr = 
        (buffer *) mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("receiver mapped address: %p\n",addr);

    buffer * buff = addr; //Reading the contents of the shared memory

    add_prod(buff); //Increasing the producers count.

/*----------------------------------------------------------------------------------------------*/

/*-----------------------------------Working: Producing a Message-------------------------------*/
       

    if (mode[0] == 'm'){

        while (buff->work){
            printf("\n      Press space to produce a message");
            while(keypress(0)!= ' ');
            printf("\n \033[22;36m*--------------------Starting a new Production cycle------------------------*\033[22;0m");
            time_t start = time(NULL);
            //sleep(wait_time);


            sem_wait(sem_empty); //If full value is 0 there are no messages to be read so we wait.
            printf("\n   Waiting for semaphore to be available\n");
            sem_wait(sem_prod);//We can read once someone has produced messages.

            time_t finish = time(NULL);

            buff->wait_time += finish - start ;

            printf("\n   Semaphore is available now...\n");
           

            if(buff->work){        //If we are allowed to work, we proceed with pushing a message.
                
                Message temp = create_message(prod_pid, gen_key()); 
                print_message(temp);
                circ_bbuf_push(buff,temp);
                
            }
            
            sem_post(sem_prod); //Releasing the mutex to allow a producer to produce
            sem_post(sem_full); //Increasing the number of empty elements in the buffer


        }
    }else{
        while (buff->work){
            
            printf("\n \033[22;36m*--------------------Starting a new Production cycle------------------------*\033[22;0m");
            time_t start = time(NULL);
            //sleep(wait_time);

            Message temp = create_message(prod_pid, gen_key()); 
            

            sem_wait(sem_empty); //If full value is 0 there are no messages to be read so we wait.
            printf("\n   Waiting for semaphore to be available\n");
            sem_wait(sem_prod);//opening semaphore

            time_t finish = time(NULL);

            buff->wait_time += finish - start ;

            printf("\n   Semaphore is available now...\n");
            

            if(buff->work){        //If we are allowed to work, we proceed with pushing a message.
                
                print_message(temp);
                circ_bbuf_push(buff,temp);
                
            }
            
            sem_post(sem_prod); //Releasing the mutex to allow a producer to produce
            sem_post(sem_full); //Increasing the number of empty elements in the buffer

            
        }

    }
/*-------Closing things nicely-------*/
    munmap(addr, sizeof(buffer));
    close(fd);
    //shm_unlink(NAME);
    return EXIT_SUCCESS;
}
