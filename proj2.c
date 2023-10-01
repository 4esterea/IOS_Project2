// proj2.c
// Řešení IOS PROJEKT 2, 29.4.2023
// Autor: Zhdanovich Iaroslav, FIT
// Přeloženo: gcc 9.4.0

#include <stdio.h>
#include <stdlib.h>
#include <time.h>  
#include <sys/types.h>
#include <unistd.h>     
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <stdbool.h>


#define CLIENT_COUNT atoi(argv[1])
#define WORKER_COUNT atoi(argv[2])
#define CLIENT_DELAY atoi(argv[3])
#define BREAK_TIME atoi(argv[4])
#define CLOSING_TIME atoi(argv[5])
#define OUTPUT "proj2.out"

// message_list & error_list: enum lists for outputting messages to a file or stderr

enum message_list { ZSTART, USTART, ZHOME, UHOME, ZENTER1, ZENTER2, ZENTER3, ZCALLED,
                  UBREAKSTART, UBREAKEND, USERVE1, USERVE2, USERVE3, USERVEF, MCLOSE };

enum error_list { ARGN, ARGL, SEM, SHMEM, FORK, FILEF };

// shared memory structure: contains variables, semaphores and an output file

typedef struct shared_memory {

    FILE *file;

    unsigned global_row_counter;        // counter of lines written to the file
    unsigned cc_total;                  // counter of total number of clients 
    unsigned cc_in_queue1;              // counter of number of clients in 1st queue
    unsigned cc_in_queue2;              // counter of number of clients in 2nd queue
    unsigned cc_in_queue3;              // counter of number of clients in 3rd queue

    sem_t sem_process_done;             // semaphore to control the number of completed processes
    sem_t sem_access;                   // semaphore to control access to shared memory
    sem_t sem_out;                      // semaphore to control the output of data to a file
    sem_t sem_wqueue1;                  // semaphore to control the 1st queue
    sem_t sem_wqueue2;                  // semaphore to control the 2nd queue
    sem_t sem_wqueue3;                  // semaphore to control the 3rd queue
    sem_t sem_post_closed;              // semaphore to control the output of data to a file

    bool post_office_state;             // variable that displays the current state of the post office ( open / closed )
 
}shmemory_t;

// free_all(): the function frees the allocated shared memory, 
// destroys the semaphores, and closes the output file

void free_all(shmemory_t *shmemory){

    fclose(shmemory->file);

    sem_destroy(&shmemory->sem_wqueue1);
    sem_destroy(&shmemory->sem_wqueue2);
    sem_destroy(&shmemory->sem_wqueue3);
    sem_destroy(&shmemory->sem_post_closed);
    sem_destroy(&shmemory->sem_out);
    sem_destroy(&shmemory->sem_access);
    sem_destroy(&shmemory->sem_process_done);

    munmap(shmemory, sizeof(shmemory_t));
}

//print_error(): the function displays an error message and frees the allocated memory,
// depending on which stage of the program execution the error occurred

void print_error(enum error_list error, shmemory_t *shmemory){
    switch (error) {
        case ARGN:
            fprintf(stderr, "Wrong argument number\n");
            break;
        case ARGL:
            fprintf(stderr, "Some of the arguments is out of limits\n");
            break;
        case SHMEM:
            fprintf(stderr, "Shared memory initialization error\n");
            break;
        case FILEF:
            fprintf(stderr, "File opening error\n");
            munmap(shmemory, sizeof(shmemory_t));
            break;
        case SEM:
            fprintf(stderr, "Semaphore initialization error\n");
            fclose(shmemory->file);
            munmap(shmemory, sizeof(shmemory_t));
            break;
        case FORK:
            fprintf(stderr, "Fork failed\n");
            free_all(shmemory);
            break;
    }
    exit(1);
}

// print_sync(): the function outputs the message to the file, the output is controlled by semaphore

static void print_sync(enum message_list message, unsigned idP, shmemory_t *shmemory) {
    sem_wait(&shmemory->sem_out);
    switch(message){
        case ZSTART:
            fprintf(shmemory->file, "%d: Z %d: started\n", ++shmemory->global_row_counter, idP);
            break;
        case USTART:
            fprintf(shmemory->file, "%d: U %d: started\n", ++shmemory->global_row_counter, idP);
            break;
        case ZHOME:
            fprintf(shmemory->file, "%d: Z %d: going home\n", ++shmemory->global_row_counter, idP);
            break;
        case UHOME:
            fprintf(shmemory->file, "%d: U %d: going home\n", ++shmemory->global_row_counter, idP);
            break;
        case ZENTER1:
            fprintf(shmemory->file, "%d: Z %d: entering office for a service 1\n", ++shmemory->global_row_counter, idP);
            break;
        case ZENTER2:
            fprintf(shmemory->file, "%d: Z %d: entering office for a service 2\n", ++shmemory->global_row_counter, idP);
            break;
        case ZENTER3:
            fprintf(shmemory->file, "%d: Z %d: entering office for a service 3\n", ++shmemory->global_row_counter, idP);
            break;
        case ZCALLED:
            fprintf(shmemory->file, "%d: Z %d: called by office worker\n", ++shmemory->global_row_counter, idP);
            break;
        case UBREAKSTART:
            fprintf(shmemory->file, "%d: U %d: taking break\n", ++shmemory->global_row_counter, idP);
            break;
        case UBREAKEND:
            fprintf(shmemory->file, "%d: U %d: break finished\n", ++shmemory->global_row_counter, idP);
            break;
        case USERVE1:
            fprintf(shmemory->file, "%d: U %d: serving a service of type 1\n", ++shmemory->global_row_counter, idP);
            break;
        case USERVE2:
            fprintf(shmemory->file, "%d: U %d: serving a service of type 2\n", ++shmemory->global_row_counter, idP);
            break;
        case USERVE3:
            fprintf(shmemory->file, "%d: U %d: serving a service of type 3\n", ++shmemory->global_row_counter, idP);
            break;
        case USERVEF:
            fprintf(shmemory->file, "%d: U %d: service finished\n", ++shmemory->global_row_counter, idP);
            break;
        case MCLOSE:
            fprintf(shmemory->file, "%d: closing\n", ++shmemory->global_row_counter);
            break;
    }
    fflush(shmemory->file);
    sem_post(&shmemory->sem_out);
    return;
}

// check_args(): the function checks the number of arguments when starting the program and their value

void check_args(int argc, char *argv[]){
    if (argc != 6) print_error(ARGN, NULL);
    if (CLIENT_COUNT == 0 || WORKER_COUNT == 0) print_error(ARGL, NULL);
    if (CLIENT_DELAY < 0 || CLIENT_DELAY > 10000) print_error(ARGL, NULL);
    if (BREAK_TIME < 0 || BREAK_TIME > 100) print_error(ARGL, NULL);
    if (CLOSING_TIME < 0 || CLOSING_TIME > 10000) print_error(ARGL, NULL);
    return;
}

// delay(): the function sleeps the process for a random time in the range < S ; R > ms

void delay(int R, int S, pid_t PID){
    if (R == 0) return;
    srand(PID+rand()+time(NULL));
    usleep((( rand() % R ) + S ) * 1000 );
    return;
}

// client_process(): the client process

int client_process(int idZ, int D, shmemory_t *shmemory, pid_t PID){

    print_sync(ZSTART, idZ, shmemory);                              // start of the process
    
    delay(D, 0, PID);                                               // client waits for random time in range < 0 ; D >
    
    srand(PID+rand()+time(NULL));                                   //--
    int queue = rand() % 3 + 1;                                     //  | client selects the type of service for which he came to the mail
    sem_wait(&shmemory->sem_access);                                //--
    
    if (shmemory->post_office_state == true) {                      // checks if mail is closed or not 
        
        switch(queue){                                              //--
                                                                    //  |
            case 1:                                                 //  |
                shmemory->cc_total++;                               //  |
                shmemory->cc_in_queue1++;                           //  |
                print_sync(ZENTER1, idZ, shmemory);                 //  |
                sem_post(&shmemory->sem_access);                    //  |
                sem_wait(&shmemory->sem_wqueue1);                   //  |
                break;                                              //  |
                                                                    //  |
            case 2:                                                 //  |
                shmemory->cc_total++;                               //  |
                shmemory->cc_in_queue2++;                           //  |
                print_sync(ZENTER2, idZ, shmemory);                 //  | if the post office is open, client enters the selected queue
                sem_post(&shmemory->sem_access);                    //  |
                sem_wait(&shmemory->sem_wqueue2);                   //  |
                break;                                              //  |
                                                                    //  |
            case 3:                                                 //  |
                shmemory->cc_total++;                               //  |
                shmemory->cc_in_queue3++;                           //  |
                print_sync(ZENTER3, idZ, shmemory);                 //  |
                sem_post(&shmemory->sem_access);                    //  |
                sem_wait(&shmemory->sem_wqueue3);                   //  |
                break;                                              //  |
                                                                    //--
        }                                                       
    } else {                                                        //--
        print_sync(ZHOME, idZ, shmemory);                           //  |
        sem_post(&shmemory->sem_access);                            //  | if the post office is closed, the client goes home (the process terminates)
        sem_post(&shmemory->sem_process_done);                      //  |
        exit(0);                                                    //--
    }
    print_sync(ZCALLED, idZ, shmemory);                             //--
    delay(10, 0, idZ);                                              //  |
    print_sync(ZHOME, idZ, shmemory);                               //  | the client is called by the postal worker, waits 10ms, then goes home (the process terminates)
    sem_post(&shmemory->sem_process_done);                          //  |
    exit(0);                                                        //--
}

// worker_process(): the worker process 

int worker_process(int idU, int B, shmemory_t *shmemory, pid_t PID){

    print_sync(USTART, idU, shmemory);                              // start of the process

    while (true){                                                   // [ start of the worker cykle ]
        
        sem_wait(&shmemory->sem_access);
        
        if ( shmemory->cc_total == 0 && shmemory->post_office_state == true ){  //--
            sem_post(&shmemory->sem_access);                                    //  |
            print_sync(UBREAKSTART, idU, shmemory);                             //  | if the mail is open and there are no clients in any of the queues,
            delay(B, 0, PID);                                                   //  | the post office worker goes on a break
            print_sync(UBREAKEND, idU, shmemory);                               //  |
            continue;                                                           //--
        } 
        
        if ( shmemory->cc_total == 0 && shmemory->post_office_state == false ){ //--
            sem_post(&shmemory->sem_access);                                    //  | if the mail is open and there are no clients in any of the queues,
            break;                                                              //  | the worker cykle ends
        }                                                                       //--
        
        if ( shmemory->cc_in_queue1 > 0 ){                                      //--
            shmemory->cc_in_queue1--;                                           //  |
            shmemory->cc_total--;                                               //  |
            sem_post(&shmemory->sem_access);                                    //  |
            print_sync(USERVE1, idU, shmemory);                                 //  | if there is a customer in queue 1, the post office worker serves him
            sem_post(&shmemory->sem_wqueue1);                                   //  |
            delay(10, 0, idU);                                                  //  |
            print_sync(USERVEF, idU, shmemory);                                 //  |
            continue;                                                           //--
        }
        
        if ( shmemory->cc_in_queue2 > 0 ){                                      //--
            shmemory->cc_in_queue2--;                                           //  |
            shmemory->cc_total--;                                               //  |
            sem_post(&shmemory->sem_access);                                    //  |
            print_sync(USERVE2, idU, shmemory);                                 //  | if there is a customer in queue 2, the post office worker serves him
            sem_post(&shmemory->sem_wqueue2);                                   //  |
            delay(10, 0, idU);                                                  //  |
            print_sync(USERVEF, idU, shmemory);                                 //  |
            continue;                                                           //--
        }
        
        if ( shmemory->cc_in_queue3 > 0 ){                                      //--
            shmemory->cc_in_queue3--;                                           //  |
            shmemory->cc_total--;                                               //  |
            sem_post(&shmemory->sem_access);                                    //  |
            print_sync(USERVE3, idU, shmemory);                                 //  | if there is a customer in queue 3, the post office worker serves him
            sem_post(&shmemory->sem_wqueue3);                                   //  |
            delay(10, 0, idU);                                                  //  |
            print_sync(USERVEF, idU, shmemory);                                 //  |
            continue;                                                           //--
        }
        sem_post(&shmemory->sem_access);
    }                                                                           //--
    sem_wait(&shmemory->sem_post_closed);                                       //  |
    print_sync(UHOME, idU, shmemory);                                           //  | after the end of the worker cykle post office worker goes home (the process terminates)
    sem_post(&shmemory->sem_process_done);                                      //  |
    exit(0);                                                                    //--
}

// main_process(): the main process

void main_process(char *argv[], shmemory_t *shmemory) {
    
    pid_t PID;

    for (int i = 1; i <= CLIENT_COUNT; i++){                            //--     
        PID = fork();                                                   //  |
        switch (PID){                                                   //  |
            case -1:                                                    //  |
                print_error(FORK, shmemory);                            //  |
                break;                                                  //  |
            case 0:                                                     //  |  client process generator
                continue;                                               //  |  creates the client processes in the amount of CLIENT_COUNT 
            default:                                                    //  |
                client_process(i, CLIENT_DELAY, shmemory, PID);         //  |
                break;                                                  //  |
        }                                                               //  |
    }                                                                   //--

    for (int i = 1; i <= WORKER_COUNT; i++){                            //--     
        PID = fork();                                                   //  |
        switch (PID){                                                   //  |
            case -1:                                                    //  |
                print_error(FORK, shmemory);                            //  |
                break;                                                  //  |
            case 0:                                                     //  |  worker process generator
                continue;                                               //  |  creates the worker processes in the amount of WORKER_COUNT 
            default:                                                    //  |
                worker_process(i, BREAK_TIME, shmemory, PID);           //  |
                exit(0);                                                //  |
        }                                                               //  |
    }                                                                   //--

    delay((CLOSING_TIME/2), (CLOSING_TIME/2), PID);                     // process is waiting for random time in range < CLOSING_TIME/2 ; CLOSING_TIME > ms

    sem_wait(&shmemory->sem_access);                                    //--
    shmemory->post_office_state = false;                                //  |   post office is closing
    print_sync(MCLOSE, 1, shmemory);                                    //  |   
    sem_post(&shmemory->sem_access);                                    //--

    for (int i = 0; i < WORKER_COUNT; i++) sem_post(&shmemory->sem_post_closed);    // sends post office closure signals to worker processes

    for (int i = 0; i < WORKER_COUNT + CLIENT_COUNT; i++) sem_wait(&shmemory->sem_process_done); // waits for all processes to be terminated

    free_all(shmemory);                                                 // frees the allocated memory

    exit(0);                                                            // the process terminates
}

// set_shared_memory(): the function allocates shared memory, 
// initializes semaphores, variables, and opens the output file in mode "w+"

void set_shared_memory(shmemory_t *shmemory){

    shmemory->file = fopen(OUTPUT, "w+");
    if (!(shmemory->file)) print_error(FILEF, shmemory);

    shmemory->cc_total = 0;
    shmemory->global_row_counter = 0;
    shmemory->cc_in_queue1 = 0;
    shmemory->cc_in_queue2 = 0;
    shmemory->cc_in_queue3 = 0;
    shmemory->post_office_state = true;

    if (sem_init(&shmemory->sem_wqueue1, 1, 0) < 0) print_error(SEM, shmemory);
    if (sem_init(&shmemory->sem_wqueue2, 1, 0) < 0) print_error(SEM, shmemory);
    if (sem_init(&shmemory->sem_wqueue3, 1, 0) < 0) print_error(SEM, shmemory);
    if (sem_init(&shmemory->sem_post_closed, 1, 0) < 0) print_error(SEM, shmemory);
    if (sem_init(&shmemory->sem_out, 1, 1) < 0) print_error(SEM, shmemory);
    if (sem_init(&shmemory->sem_access, 1, 1) < 0) print_error(SEM, shmemory);
    if (sem_init(&shmemory->sem_process_done, 1, 0) < 0) print_error(SEM, shmemory);

    return;
}

//main(): start process of the program

int main(int argc, char *argv[]){
    check_args(argc, argv);
    shmemory_t *shmemory = (shmemory_t *)mmap(NULL, sizeof(shmemory_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shmemory == MAP_FAILED) print_error(SHMEM, NULL);
    set_shared_memory(shmemory);
    main_process(argv, shmemory);
    return 0;
}