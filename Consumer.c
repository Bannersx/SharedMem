#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "buffer.h"
#include <time.h>
#include <string.h>
#include "PoissonExp.c"




//Function to display a producer statistics
void print_cons_stats(pid_t pid, int messages, float wait, float blocked, float kernel, bool key){
    char * reason;
    if(key){
        reason = "This consumer stopped because the \n                 Process ID module 6 was equal to the key";
    }else{
        reason = "The consumer was signaled by the\n                   finalizer to end";
    }

    printf("\n          |*------------ Here are the Consumer Statistics -----------*|\n");
    printf("\n                -> Proccess ID: %d\n", pid);
    printf("                -> Total number of read messages: %d\n", messages);
    printf("                -> Amount of waited time: %0.4fs\n", wait);
    printf("                -> Amount of time blocked by semaphores: %0.4fs\n", blocked);
    printf("                -> Amount of time in kernel: %0.9fs\n", kernel);
    printf("                -> Reason of halt: %s\n",reason);
    printf("\n          |*--------------------End of Statistics-------------------*|\n");
}
int main (int argc, char *argv[]){

    pid_t cons_pid = getpid();

    /*--- Checking if the number of arguments is correct ---*/
    if (argc != 7){
        printf("\nusage: -n [name of buffer] -m [a-> automatic|m-> manual] -t [mean wait time]\nPlease use the full arguments\n");
        return -1;
    }

    /*---Parsing the arguments & handling the flags---*/
    int count;
    char shm_name[20];
    char * mode;
    char * mean_time;
    for (count = 1; count < argc ; count++){
        switch (argv[count][1]) {
            case 'n': strcpy(shm_name, argv[count+1]);
            case 'm': mode = argv[count+1];
            case 't': mean_time = argv[count+1];
        }
    } 

/*----------------------Average mean wait time------------------------------------------*/
    int temp_wait;
    char str[4];
    strcpy(str, mean_time);
    temp_wait = atoi(str);

/*----------------------Consumer Statistics------------------------------*/

    float total_wait_time = 0;
    float total_block_time = 0;
    float total_kernel_time = 0;
    int all_time_messages = 0;
    bool key_eliminated = false;


/*---------------------Working on the semaphores------------------------*/
  
    char prod_sem[20] = SEM_PRODUCER_FNAME;
    strcat(prod_sem,shm_name);
    char empty_sem[20] = SEM_EMPTY_FNAME;
    strcat(empty_sem,shm_name);
    char full_sem[20] = SEM_FULL_FNAME;
    strcat(full_sem, shm_name);


    printf("\n |*******************************************************************************|\n |*-----------------------------Consumer Process---------------------------------|\n |*******************************************************************************|\n");
 /*---Opening all the needed semaphores inside the process---*/
    printf("\n  -> Opening the semaphore: %s...\n", prod_sem);
    sem_t * sem_prod = sem_open(prod_sem, 0);
    //Checking if the semaphore was created succesfully
    if (sem_prod == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }


    //Creating the full semaphore: This is used to count the number of full items in the buffer
    printf("\n  -> Opening the semaphore: %s...\n", full_sem);
    sem_t * sem_full = sem_open(full_sem, 0);
    //Checking if the semaphore was created succesfully
    if (sem_full == SEM_FAILED){
        perror("sem_open/full");
        exit(EXIT_FAILURE);
    }

    //Creating the empty semaphore: This is used to keep track of the empty number of elements in the buffer.
    printf("\n  -> Opening the semaphore: %s...\n", empty_sem);
    sem_t * sem_empty = sem_open(empty_sem, 0660,0);
    //Checking if the semaphore was created succesfully
    if (sem_empty == SEM_FAILED){
        perror("sem_open/empty");
        exit(EXIT_FAILURE);
    }

    printf("\n   All Semaphores are open.\n");
 /*---------------------All semaphores are up and running---------------------*/
/*-----------------------------------------------------------------------------*/

/*---------------------------Shared memory----------------------------------------------*/
    int fd = shm_open(shm_name, O_RDWR,0666);
    if (fd <0){
        perror("shm_open()");
        return EXIT_FAILURE;
    }

    void * addr = 
        (buffer *) mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    

    buffer * buff = addr; //Reader the buffer contents
    add_cons(buff); //Increasing the consumers counter

/*------------------- Working: consuming message----------------------------------*/
    
    if (mode[0] == 'm'){

        while (buff->work){
            
            printf("\n \033[22;36m*--------------------------------------------------------------------------------*\n *-------------------------Starting a new Consumption cycle-----------------------*\n *--------------------------------------------------------------------------------*\033[22;0m\n");
        

            int cycle_wait_time = poisson(temp_wait);   //RNGeesus take the wheel again!
            printf("\n      Wait time for this cycle: %d\n", cycle_wait_time);
            
            total_wait_time += cycle_wait_time; //updating the total wait time of the consumer.
            sleep(cycle_wait_time);

            time_t start = time(NULL);

            printf("\n      Looking for messages....\n");
            sem_wait(sem_full); //If full is 0 there are no messages to be read, we gotta wait.
            printf("\n      Messages found!!\n");
            printf("\n      Waiting for the buffer to be available\n");
            sem_wait(sem_prod); //We can read once the mutex is unlocked.
            printf("\n      Buffer is available now...\n");

            printf("\n      \033[22;33mPress space to read a message\033[22;0m\n"); //Waiting for the keypress to continue
            while(keypress(0)!= ' ');   //Detecting the keypress

            time_t finish = time(NULL);     //Taking the time after we finish waiting

            
            total_block_time += finish - start;

            //----------------Trying to measure user/wall time-----------------//

            clock_t start2, end;
            double cpu_time_used;

            start2 = clock();

            Message temp = circ_bbuf_pop(buff,temp);    //Popping a message from the buffer
                        

            sem_post(sem_prod);     //Releasing the mutex to allow a producer to produce
            sem_post(sem_empty);    //Increasing the number of empty elements in the buffer

            end = clock();
            cpu_time_used = ((double) (end - start2)) / CLOCKS_PER_SEC;
            
            if(buff->work){        //If we are allowed to work, we proceed with consuming a message.
                
                all_time_messages += 1;
                total_kernel_time += cpu_time_used;
                print_cons_info(buff);
                print_message(temp);    //Consuming the message
                
                if (temp.key == temp.pid || temp.key == (cons_pid % 6)){ //checking for the special condition to stop the process

                    key_eliminated = true;
                    printf("\n   -> Special key condition is met. \n   -> Process %d is now finalizing.\n",cons_pid);
                    rem_key_cons(buff);
                    break;
                }
            }

        }
    }else{
        while (buff->work){
            
            printf("\n \033[22;36m *------------------------------------------------------------------------------*\n *-------------------------Starting a new Consumption cycle----------------------*\n *-------------------------------------------------------------------------------*\033[22;0m\n");
            
            int cycle_wait_time = poisson(temp_wait);   //RNGeesus take the wheel again!
            printf("\n      Wait time for this cycle: %d\n", cycle_wait_time);
            
            total_wait_time += cycle_wait_time; //updating the total wait time of the consumer.
            sleep(cycle_wait_time);

            time_t start = time(NULL);

            printf("\n      Looking for messages....\n");
            sem_wait(sem_full); //If full is 0 there are no messages to be read, we gotta wait.
            printf("\n      Messages found!!\n");
            printf("\n      Waiting for the buffer to be available\n");
            sem_wait(sem_prod); //We can read once the mutex is unlocked.
            printf("\n      Buffer is available now...\n");

            time_t finish = time(NULL);     //Taking the time after we finish waiting

            
            total_block_time += finish - start;

            //----------------Trying to measure user/wall time-----------------//

            clock_t start2, end;
            double cpu_time_used;

            start2 = clock();

            Message temp = circ_bbuf_pop(buff,temp);    //Popping a message from the buffer
                        

            sem_post(sem_prod);     //Releasing the mutex to allow a producer to produce
            sem_post(sem_empty);    //Increasing the number of empty elements in the buffer

            end = clock();
            cpu_time_used = ((double) (end - start2)) / CLOCKS_PER_SEC;
            
            if(buff->work){        //If we are allowed to work, we proceed with consuming a message.
                
                
                total_kernel_time += cpu_time_used;
                print_cons_info(buff);
                print_message(temp);    //Consuming the message
                
                if (temp.key == temp.pid || temp.key == (cons_pid % 6)){ //checking for the special condition to stop the process

                    key_eliminated = true;
                    printf("\n   -> Special key condition is met. \n   -> Process %d is now finalizing.\n",cons_pid);
                    rem_key_cons(buff);
                    break;
                }
            }
            all_time_messages += 1;

            
        }

    }
    

    buff->tot_kernel+=total_kernel_time;
    buff->wait_time += total_wait_time; //updating the total wait time of the buffer.
    buff->blocked_time += total_block_time;     //Adding the block time to buffer statistics
    printf("\n      \033[22;31mThis consumer has been signaled to end\n");

    rem_cons(buff);

    /*------Closing things nicely-----------*/
    printf("\n      \033[22;0mMaking things nice and tidy before closing...\n");

    sem_close(sem_prod);
    sem_close(sem_full);
    sem_close(sem_empty);
    printf("\n      Consumer pid: %d is now closing\n", cons_pid);
    print_cons_stats(cons_pid, all_time_messages,total_wait_time, total_block_time, total_kernel_time, key_eliminated);
    munmap(addr, sizeof(buffer));
    close(fd);
    printf("\n |*---------------------------End of Consumer---------------------------------*|\n");
    return EXIT_SUCCESS;
}