
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



//Function to add a new message to the buffer
int circ_bbuf_push(buffer * c, Message message)
{
    int next;

    printf("\n      \033[22;34mCurrent number of running producers: \033[22;0m%d\n",c->cur_prod);
    printf("\n      \033[22;34mMessage inserted in index: \033[22;0m%d\n", c->head);
    
    next = c->head + 1;  // next is where head will point to after this write.
    if (next >= c->max_size)
        next = 0;

    if (next == c->tail)  // if the head + 1 == tail, circular buffer is full
        return -1;

    c->message[c->head] = message;  // Load data and then move.
    c->head = next;             // head to next data offset.
    c->current_size +=1;    // Increasing the current size of elements in the buffer.
    c->tot_mess +=1;    //Updating the total number of messages.
    return 0;  // return success to indicate successful push.
}

//Function to pop a message from the buffer
Message circ_bbuf_pop(buffer *c, Message data)
{
    int next;

    next = c->tail + 1;  // next is where tail will point to after this read.
    if(next >= c->max_size)
        next = 0;

    data = c->message[c->tail];  // Read data and then move
    c->tail = next;              // tail to next offset.
    if(c->current_size > 0){
        c->current_size -=1;
    }
    return data;  // return success to indicate successful push.
}

//Function to show messages when consumming them.
void print_message(Message message){

    printf("\n \033[22;32m    *-----------------------Displaying Message------------------------------*\n\n");
    printf("\033[22;33m        The process ID is: \033[22;37m%d\n",message.pid);
    printf("\033[22;33m        Special key number is: \033[22;37m%d\n",message.key);
    printf("\033[22;33m        Date and time of creation: \033[22;37m%s\n",message.date_time);
    printf("\n \033[22;32m    *-------------------------End of Message-------------------------------*\033[22;0m\n");
    
}

//Utility function to keep track of consumer statistics on creation.
void add_cons(buffer * c){
    c->cur_cons += 1;
    c->tot_cons += 1;
}

//Utility function to keep track of consumer statistics on closure.
void rem_cons(buffer * c){
    c->cur_cons -=1;
}

//Function that keeps track of consumers closed by key.
void rem_key_cons(buffer * c){
    c->cur_cons -=1;
    c->cons_key_elm +=1;
}

//Utility function to keep track of producer statistics
void add_prod(buffer * c){
    c->cur_prod += 1;
    c->tot_prod += 1;
}

void rem_prod(buffer * c){
    c->cur_prod -= 1;
}

//Function to create a message. 
Message create_message(pid_t pid, int key, char * date_time){
    Message temp;
    temp.pid = pid;
    temp.key = key;
    strcpy(temp.date_time,date_time);
    return temp;
}

//Function to show the final stats of the buffer.
void print_stats(buffer * c){
    printf("        |*---------------- Here are the Statistics ----------------*|\n");
    printf("\n            -> All time messages: %d\n", c->tot_mess);
    printf("            -> Messages left in the buffer: %d\n", c->current_size);
    printf("            -> Total number of Producer processes: %d\n", c->tot_prod);
    printf("            -> Total number of Consumer processes: %d\n", c->tot_cons);
    printf("            -> Number of consumer closed by key: %d\n", c->cons_key_elm);
    printf("            -> Amount of time waited: %0.4fs\n", c->wait_time);
    printf("            -> Amount of time blocked by semaphores: %0.4fs\n", c->blocked_time);
    printf("\n        |*--------------------End of Statistics--------------------*|\n");
}


/* sleep until a key is pressed and return value. echo = 0 disables key echo. */
int keypress(unsigned char echo)
{
    struct termios savedState, newState;
    int c;

    if (-1 == tcgetattr(STDIN_FILENO, &savedState))
    {
        return EOF;     /* error on tcgetattr */
    }

    newState = savedState;

    if ((echo = !echo)) /* yes i'm doing an assignment in an if clause */
    {
        echo = ECHO;    /* echo bit to disable echo */
    }

    /* disable canonical input and disable echo.  set minimal input to 1. */
    newState.c_lflag &= ~(echo | ICANON);
    newState.c_cc[VMIN] = 1;

    if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &newState))
    {
        return EOF;     /* error on tcsetattr */
    }

    c = getchar();      /* block (withot spinning) until we get a keypress */

    /* restore the saved state */
    if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &savedState))
    {
        return EOF;     /* error on tcsetattr */
    }

    return c;
}

//Function to generate a random number between 0 and 6
int gen_key(){
    
    time_t t;

    /* Intializes random number generator */
    srand((unsigned) time(&t));
    int r = (rand() % (6 + 1 - 0)) + 0;
    return r;
}

//Function to print the consumers info after reading a message
void print_cons_info(buffer *  c){
    if (c->tail ==0){
        printf("\n      \033[22;34mCurrent number of running consumers: \033[22;0m%d\n", c->cur_cons);
        printf("\n      \033[22;34mCurrent number of running producers: \033[22;0m%d\n",c->cur_prod);
        printf("\n      \033[22;34mReading message in the index: \033[22;0m%d\n", c->max_size);
    }else{
        printf("\n      \033[22;34mCurrent number of running consumers: \033[22;0m%d\n", c->cur_cons);
        printf("\n      \033[22;34mCurrent number of running producers: \033[22;0m%d\n",c->cur_prod);
        printf("\n      \033[22;34mReading message in the index: \033[22;0m%d\n", c->tail-1);
    }
}