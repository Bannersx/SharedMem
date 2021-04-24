#ifndef BUFFER_H_
#define BUFFER_H_
#include <stdbool.h>
#include <termios.h>
#include <semaphore.h>


#define SEM_PRODUCER_FNAME "/producer"
#define SEM_CONSUMER_FNAME "/consumer"
#define SEM_FULL_FNAME "/full"
#define SEM_EMPTY_FNAME "/empty"

struct Message;
struct buffer ;

typedef struct Message{
    int pid;
    int key;
}Message;

typedef struct buffer
{
    //Buffer related variables
    int max_size;
    int current_size;
    int head;
    int tail;
    int read;
    struct Message message[50];
    

    //Control related variables
    int cur_prod; //Number of producers alive
    int cur_cons; //Number of consumers alive
    int tot_prod; //Total number of producers
    int tot_cons; //Total number of consumer
    int tot_mess; //Total number of messages.
    bool work;

    //Statistics
    float wait_time;
    int cons_key_elm;
}buffer;

void add_cons(buffer * c);
void rem_cons(buffer * c);
void rem_key_cons(buffer * c);
void add_prod(buffer * c);
void rem_prod(buffer * c);

int keypress(unsigned char echo);

int circ_bbuf_push(buffer * c, Message message);
Message circ_bbuf_pop(buffer *c, Message message);
Message create_message(pid_t pid, int key);
void print_message(Message message);
void print_stats(buffer * c);

#endif /* BUFFER_H */ 