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
    if (argc != 7){
        printf("\nusage: -n [name of buffer] -m [a-> automatic|m-> manual]\nPlease use the full arguments\n");
        return -1;
    }

    /*---Parsing the arguments & handling the flags---*/
    int count;
    char * shm_name;
    char * mode;
    char * mean_time;
    for (count = 1; count < argc ; count++){
        switch (argv[count][1]) {
            case 'n': shm_name = argv[count+1];
            case 'm': mode = argv[count+1];
            case 't': mean_time = argv[count+1];
        }
    } 

    /*----------------------Average mean wait time------------------------------------------*/
    int temp_wait;
    char str[4];
    strcpy(str, mean_time);
    temp_wait = atoi(str);

  
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
            
            printf("\n \033[22;36m*--------------------------------------------------------------------------------*\n *-------------------------Starting a new Production cycle-----------------------*\n *--------------------------------------------------------------------------------*\033[22;0m\n");
            time_t start = time(NULL);  //Taking the start time
            //sleep(wait_time);

            printf("\n      Looking for available space in the buffer...\n");
            sem_wait(sem_empty); //If empty is 0 the buffer is full, we gotta wait.
            printf("\n      Available space found!!\n");
            
            printf("\n      Waiting for buffer to be available\n");
            sem_wait(sem_prod); //We can write once the mutex is unlocked.
            printf("\n      Buffer is available now...\n");

            printf("\n      \033[22;33mPress space to read a message\033[22;0m\n"); //Waiting for the keypress to continue
            while(keypress(0)!= ' ');   //Detecting the keypress
            
            time_t finish = time(NULL);     //Taking the time after we finish waiting

            buff->wait_time += finish - start ;     //Adding the wait time to buffer statistics

            
           

            if(buff->work){        //If we are allowed to work, we proceed with pushing a message.
                
                Message temp = create_message(prod_pid, gen_key());     //Creating a message with the process id and a random key.
                print_message(temp);        //Displaying the message before inserting it
                circ_bbuf_push(buff,temp);      //Pushing the message into the buffer
                
            }
            
            sem_post(sem_prod); //Releasing the mutex to allow a other processes to access memory
            sem_post(sem_full); //Increasing the number of full elements in the buffer


        }
    }else{
        while (buff->work){
            
            printf("\n \033[22;36m*--------------------------------------------------------------------------------*\n *-------------------------Starting a new Production cycle-----------------------*\n *--------------------------------------------------------------------------------*\033[22;0m\n");
            time_t start = time(NULL);
            //sleep(wait_time);

            Message temp = create_message(prod_pid, gen_key()); //Creating a message with the process id and a random key.
            

            sem_wait(sem_empty); //If empty is 0 the buffer is full, we gotta wait.
            printf("\n      Waiting for Buffer to be available\n");
            sem_wait(sem_prod);//We can write once the mutex is unlocked.
            printf("\n      Buffer is available now...\n");

            time_t finish = time(NULL);

            buff->wait_time += finish - start ;
            

            if(buff->work){        //If we are allowed to work, we proceed with pushing a message.
                
                print_message(temp);        //Displaying the message before inserting it
                circ_bbuf_push(buff,temp);  //Pushing the message into the buffer
                
            }
            
            sem_post(sem_prod); //Releasing the mutex to allow a other processes to access memory
            sem_post(sem_full); //Increasing the number of full elements in the buffer

            
        }

    }
    
    printf("\n      \033[22;31mThis producer has been signaled to end\n");

    rem_prod(buff);
/*-------Closing things nicely-------*/
    printf("\n      \033[22;0mMaking things nice and tidy before closing...\n");

    sem_close(sem_prod);
    sem_close(sem_full);
    sem_close(sem_empty);
    printf("\n      Producer pid: %d is now closing\n", prod_pid);
    munmap(addr, sizeof(buffer));
    close(fd);
    printf("\n |*---------------------------End of Producer---------------------------------*|\n");
    return EXIT_SUCCESS;
}
