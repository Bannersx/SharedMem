#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "buffer.h"
#include <time.h>




void show_info(int size, int pid, char * name, void *addr){
    printf("The process ID is: %d\n",pid);
    printf("Shared Memory stored as: %s\n",name);
    printf("Receiver mapped address: %p\n",addr);
    printf("The size of the buffer is: %d\n",size);
}
int main (int argc, char *argv[]){

    pid_t cons_pid = getpid();

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


/*---------------------Working on the semaphores------------------------*/
  
 
    printf("\n |*-----------------------------Consumer Process------------------------------*|");
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
            printf("\n      Press space to read a message");
            while(keypress(0)!= ' ');
            printf("\n \033[22;36m*--------------------Starting a new Consumption cycle------------------------*\033[22;0m");
            time_t start = time(NULL);
            //sleep(wait_time);


            sem_wait(sem_full); //If full value is 0 there are no messages to be read so we wait.
            printf("\n   Waiting for semaphore to be available\n");
            sem_wait(sem_prod);//We can read once someone has produced messages.

            time_t finish = time(NULL);

            buff->wait_time += finish - start ;

            printf("\n   Semaphore is available now...\n");
            int fd = shm_open(shm_name, O_RDWR,0666);
            if (fd <0){
                perror("shm_open()");
                return EXIT_FAILURE;
            }
            
            sem_post(sem_prod); //Releasing the mutex to allow a producer to produce
            sem_post(sem_empty); //Increasing the number of empty elements in the buffer

            if(buff->work){        //If we are allowed to work, we proceed with consuming a message.
                
                Message temp = circ_bbuf_pop(buff,temp); 
                print_message(temp);
                
                if (temp.key == temp.pid || temp.key == (cons_pid % 6)){ //checking for the special condition to stop the process

                    printf("\n |--> Special key condition is met. \n |--> Process %d is now finalizing.\n",cons_pid);
                    rem_key_cons(buff);
                    break;
                }
            }

        }
    }else{
        while (buff->work){
            
            printf("\n \033[22;36m*--------------------Starting a new Consumption cycle------------------------*\033[22;0m");
            time_t start = time(NULL);
            //sleep(wait_time);


            sem_wait(sem_full); //If full value is 0 there are no messages to be read so we wait.
            printf("\n   Waiting for semaphore to be available\n");
            sem_wait(sem_prod);//opening semaphore

            time_t finish = time(NULL);

            buff->wait_time += finish - start ;

            printf("\n   Semaphore is available now...\n");
            int fd = shm_open(shm_name, O_RDWR,0666);
            if (fd <0){
                perror("shm_open()");
                return EXIT_FAILURE;
            }
            /*
            void * addr = 
                (buffer *) mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        

            buff = addr;
            */           
            
            
            sem_post(sem_prod);
            sem_post(sem_empty);
            if(buff->work){
                Message temp = circ_bbuf_pop(buff,temp); 
                print_message(temp);
                if (temp.key == temp.pid || temp.key == (cons_pid % 6)){
                    printf("\n |--> Special key condition is met. \n |--> Process %d is now finalizing.\n",cons_pid);
                    rem_key_cons(buff);
                    break;
                }
            }

            
        }

    }
    


    printf("\n   \033[22;31mThis consumer has been signaled to end\n");

    rem_cons(buff);

    /*------Closing things nicely-----------*/
    printf("\n   \033[22;0mMaking things nice and tidy before closing...\n");

    sem_close(sem_prod);
    sem_close(sem_full);
    sem_close(sem_empty);
    printf("\n   Consumer pid: %d is now closing\n", cons_pid);
    munmap(addr, sizeof(buffer));
    close(fd);
    printf("\n |*---------------------------End of Consumer---------------------------------*|\n");
    return EXIT_SUCCESS;
}