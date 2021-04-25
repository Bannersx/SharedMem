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
#include <string.h>
#include "PoissonExp.c"


//Function to display a producer statistics
void print_prod_stats(pid_t pid, int messages, float wait, float blocked, float kernel){
    printf("\n          |*--------------- Here are the Statistics ----------------*|\n");
    printf("\n                -> Proccess ID: %d\n", pid);
    printf("                -> Total number of produced messages: %d\n", messages);
    printf("                -> Amount of waited time: %0.4fs\n", wait);
    printf("                -> Amount of time blocked by semaphores: %0.4fs\n", blocked);
    printf("                -> Amount of time in kernel: %0.4fs\n", kernel);
    printf("\n          |*--------------------End of Statistics-------------------*|\n");
}

int main (int argc, char *argv[]){


    pid_t prod_pid = getpid();

    /*--- Checking if the number of arguments is correct ---*/
    if (argc != 7){
        printf("\nusage: -n [name of buffer] -m [a-> automatic|m-> manual]\nPlease use the full arguments\n");
        return -1;
    }

    /*---------------Producer statistics----------------*/
    float total_wait_time = 0;
    float total_block_time = 0;
    float total_kernel_time = 0;
    int all_time_messages = 0;

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



  
  printf("\n |*******************************************************************************|\n |*-----------------------------Producer Process---------------------------------|\n |*******************************************************************************|\n");
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
    printf("\n  -> Opening the semaphore: %s...\n", SEM_EMPTY_FNAME);
    sem_t * sem_empty = sem_open(SEM_EMPTY_FNAME, 0660,0);
    //Checking if the semaphore was created succesfully
    if (sem_empty == SEM_FAILED){
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }

    printf("\n   All Semaphores are open.\n");
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
            
            printf("\n \033[22;36m*--------------------------------------------------------------------------------*\n *-------------------------Starting a new Production cycle------------------------*\n *--------------------------------------------------------------------------------*\033[22;0m\n");
            time_t start = time(NULL);  //Taking the start time

            //int cycle_wait_time = exponencial(temp_wait);
            //total_wait_time = buff->wait_time += cycle_wait_time;
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

            total_block_time = buff->blocked_time += finish - start ;     //Adding the wait time to buffer statistics

            
           

            if(buff->work){        //If we are allowed to work, we proceed with pushing a message.
                
                Message temp = create_message(prod_pid, gen_key());     //Creating a message with the process id and a random key.
                print_message(temp);        //Displaying the message before inserting it
                circ_bbuf_push(buff,temp);      //Pushing the message into the buffer
                all_time_messages += 1;
            }
            
            sem_post(sem_prod); //Releasing the mutex to allow a other processes to access memory
            sem_post(sem_full); //Increasing the number of full elements in the buffer
            
            time_t out_of_kernel = time(NULL);
            float kernel_time = finish -out_of_kernel;
            total_kernel_time +=kernel_time;

        }
    }else{
        while (buff->work){
            
            printf("\n \033[22;36m*--------------------------------------------------------------------------------*\n *-------------------------Starting a new Production cycle-----------------------*\n *--------------------------------------------------------------------------------*\033[22;0m\n");
            ///int cycle_wait_time = exponencial(temp_wait);
            //total_wait_time = buff->wait_time += cycle_wait_time;
            //sleep(wait_time);
            time_t start = time(NULL);
            
            Message temp = create_message(prod_pid, gen_key()); //Creating a message with the process id and a random key.
            

            sem_wait(sem_empty); //If empty is 0 the buffer is full, we gotta wait.
            printf("\n      Waiting for Buffer to be available\n");
            sem_wait(sem_prod);//We can write once the mutex is unlocked.
            printf("\n      Buffer is available now...\n");

            time_t finish = time(NULL);

             buff->blocked_time = total_block_time += finish - start ;
            

            if(buff->work){        //If we are allowed to work, we proceed with pushing a message.
                
                print_message(temp);        //Displaying the message before inserting it
                circ_bbuf_push(buff,temp);  //Pushing the message into the buffer
                all_time_messages += 1;
            }
            
            sem_post(sem_prod); //Releasing the mutex to allow a other processes to access memory
            sem_post(sem_full); //Increasing the number of full elements in the buffer

            time_t out_of_kernel = time(NULL);
            float kernel_time = finish -out_of_kernel;
            total_kernel_time +=kernel_time;
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

    print_prod_stats(prod_pid,all_time_messages,total_wait_time,total_block_time, total_kernel_time);

    munmap(addr, sizeof(buffer));
    close(fd);
    printf("\n |*---------------------------End of Producer------------------------------------*|\n");
    return EXIT_SUCCESS;
}
