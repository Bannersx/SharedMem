#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include "buffer.h"



/*aux function to generate the wait time
miu represents the average wait time
returns a value of an exponential distribution
double ran_expo(double miu){
    double u;
    u = rand() / (RAND_MAX + 1.0);
    return -log(1- u) * miu;
}*/




//Function to show the information
void show_info(int size, int pid, char * name, void *addr){
    printf("\033[22;32m*------------------------Summary of Init Process-------------------------------*\n\n");
    printf("\033[22;33m * The process ID is: \033[22;37m%d\n",pid);
    printf("\033[22;33m * Shared Memory stored as: \033[22;37m%s\n",name);
    printf("\033[22;33m * Receiver mapped address: \033[22;37m%p\n",addr);
    printf("\033[22;33m * The size of the buffer is: \033[22;37m%d\n",size);
    printf("\033[22;32m\n*-----------------------------End of summary---------------------------------*\n\n");
}

int gen_key(){
    int r = (rand() % (6 + 1 - 0)) + 0;
    return r;
}


int main(int argc, char *argv[]){

    /*--- Checking if the number of arguments is correct ---*/
    if (argc != 5){
        printf("usage: -n [name of buffer] -s [size of buffer]\nPlease use the full arguments\n");
        return -1;
    }

    printf("\033[22;32m*-------------------Working on the Initializer Process----------------------*\n\n");


    /*----Grapping the Process ID----*/
    pid_t init_pid = getpid();

    //printf("The process ID is: %d\n",pid);

    /*---Parsing the arguments & handling the flags---*/
    int count;
    char * shm_name;
    char * size;
    for (count = 1; count < argc ; count++){
        switch (argv[count][1]) {
            case 'n': shm_name = argv[count+1];
            case 's': size = argv[count+1];
        }
    } 
    
/*------------------CASTING STR SIZE TO INT------------------------*/    
    int temp_size;
    char str[2];
    strcpy(str, size);
    temp_size = atoi(str);
    

/*------------------------------------------------------------------------------*/    
    /*---Creating the Shared Memory---*/

    printf("\033[22;0m|--> Creating the Buffer!\n");
    int fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd <0){
        perror("shm_open()");
        return EXIT_FAILURE;
    }

    /*--------------Allocating the space needed for the struct---------------------*/
    printf("\n|--> Allocating the Space needed!\n");
    ftruncate(fd,sizeof(buffer));
    printf("\n|--> Attempting to map the buffer to memory....\n");
    void * addr = mmap(NULL, sizeof(buffer),
                        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    assert(addr != MAP_FAILED);
    printf("\n|--> Buffer has been mapped!!\n");

/*-----------------------------Setting up the semaphores------------------------------------------------------------------*/

    printf("\n|--> Working on the semaphores now...\n");

    printf("\n|--> Releasing unused semaphores with the names: %s, %s, %s, %s. If there are any...\n", SEM_PRODUCER_FNAME, SEM_CONSUMER_FNAME, SEM_EMPTY_FNAME, SEM_FULL_FNAME);
    
    sem_unlink(SEM_PRODUCER_FNAME);
    sem_unlink(SEM_CONSUMER_FNAME);
    sem_unlink(SEM_EMPTY_FNAME);
    sem_unlink(SEM_FULL_FNAME);
    
    printf("|--> Done!\n");

    
  /*----------------Creation----------------------------------------------------------*/

    //Creating the producer semaphore
    printf("\n|--> Creating the semaphore: %s...\n", SEM_PRODUCER_FNAME);
    sem_t * sem_prod = sem_open(SEM_PRODUCER_FNAME, O_CREAT, 0660,1);
    //Checking if the semaphore was created succesfully
    if (sem_prod == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    //Creating the consumer semaphore
    printf("\n|--> Creating the semaphore: %s...\n", SEM_CONSUMER_FNAME);
    sem_t * sem_cons = sem_open(SEM_CONSUMER_FNAME, O_CREAT, 0660,0);
    //Checking if the semaphore was created succesfully
    if (sem_cons == SEM_FAILED){
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }
    
    //Creating the full semaphore: This is used to count the number of full items in the buffer
    printf("\n|--> Creating the semaphore: %s...\n", SEM_FULL_FNAME);
    sem_t * sem_full = sem_open(SEM_FULL_FNAME, O_CREAT, 0660, 2);
    //Checking if the semaphore was created succesfully
    if (sem_full == SEM_FAILED){
        perror("sem_open/producer");
        exit(EXIT_FAILURE);
    }

    //Creating the empty semaphore: This is used to keep track of the empty number of elements in the buffer.
    printf("\n|--> Creating the semaphore: %s...\n", SEM_EMPTY_FNAME);
    sem_t * sem_empty = sem_open(SEM_CONSUMER_FNAME, O_CREAT, 0660,temp_size);
    //Checking if the semaphore was created succesfully
    if (sem_cons == SEM_FAILED){
        perror("sem_open/consumer");
        exit(EXIT_FAILURE);
    }


    printf("\n|--> Semaphores are ready to use!\n");
/*-----------------------------End of semaphore creation-----------------------------------------------------------------------------------------------*/
    /*-------Defining the values for the buffer--------*/ 
    buffer * buff = addr;

    buff->max_size = temp_size;
    buff->work = true;


    Message trymess = create_message(init_pid, gen_key());
    Message trymess2 = create_message(init_pid, 3);

    circ_bbuf_push(buff,trymess);
    
    /*---Displaying the information*/
    show_info(buff->max_size, init_pid, shm_name, addr);

    /*---------- Cleaning up to end process----------*/
    sem_close(sem_prod);
    sem_close(sem_cons);
    sem_close(sem_full);
    sem_close(sem_empty);
    munmap(buff,sizeof(buffer));
    close(fd);

    return EXIT_SUCCESS;
}