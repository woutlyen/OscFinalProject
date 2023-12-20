/**
 * \author Wout Lyen
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "connmgr.h"
#include "sensor_db.h"
#include "datamgr.h"
#include "sbuffer.h"

#define READ_END 0
#define WRITE_END 1

sbuffer_t *buffer;

int fd[2];
int pipe_write_end = -1;

int write_to_log_process(char *msg){

    // Initialize the sequence_number
    static int sequence_number = 0;

    // Open the log file
    FILE *file = fopen("gateway.log", "a");

    // Check if file opened properly
    if(file == NULL){
        return -1;
    }

    // Timestamp calculation
    time_t tm;
    time(&tm);
    char* time = ctime(&tm);
    time[strlen(time)-1] = '\0';

    // Print to the log file
    int value = fprintf(file, "%d - %s - %s\n", sequence_number, time, msg);

    // Check if print succeeded
    if(value <=0){
        fclose(file);
        return -1;
    }

    // Close the log file and increment the sequence number
    sequence_number++;
    return fclose(file);
}

int end_log_process(){
    close(fd[READ_END]);
    exit(0);
}

int create_log_process(){

    //create new empty log file
    FILE *file = fopen("gateway.log", "w");
    fclose(file);

    // Initialize the pid variable
    pid_t pid;

    // create the pipe
    if (pipe(fd) == -1){
        if(write_to_log_process("Error while setting up the pipe.") == -1){
            printf("No log file opened: Pipe failed\n");
        }
        return -1;
    }

    // Fork the child
    pid = fork();

    // Fork error
    if (pid < 0){
        if(write_to_log_process("Error while setting up the fork.") == -1){
            printf("No log file opened: Fork failed\n");
        }
        return -1;
    }

    // Parent process
    if (pid > 0){

        // Close the read end of the parent
        close(fd[READ_END]);
    }
    else{ // Child process

        // Close the write end of the child
        close(fd[WRITE_END]);

        char msg[SIZE] = "";
        while (strcmp(msg,"END") != 0){

            long length = read(fd[READ_END], &msg, SIZE);
            if (length <= 0){
                write_to_log_process("Log process ended");
                sprintf(msg,"END");
            }
            else{
                write_to_log_process(msg);
            }
        }
        end_log_process();
    }

    return fd[WRITE_END];
}


int main(int argc, char *argv[]){

    //init thread ids
    pthread_t conn_mgr_thread_id, data_mgr_thread_id, store_mgr_thread_id;

    if(argc < 3) {
        printf("Please provide the right arguments: first the port, then the max nb of clients");
        return -1;
    }

    //Reading out program arguments
    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    //init pipe and sbuffer
    pipe_write_end = create_log_process();
    if (sbuffer_init(&buffer) == SBUFFER_FAILURE){
        return -1;
    }

    //loading data for threads in structs
    conn_mgr_data_t *conn_mgr_data = malloc(sizeof(conn_mgr_data_t));
    conn_mgr_data->max_conn = MAX_CONN;
    conn_mgr_data->port = PORT;
    conn_mgr_data->pipe_write_end = pipe_write_end;
    conn_mgr_data->buffer = buffer;

    storage_mgr_data_t *storage_mgr_data = malloc(sizeof(storage_mgr_data_t));
    storage_mgr_data->buffer = buffer;
    storage_mgr_data->pipe_write_end = pipe_write_end;

    data_mgr_data_t *data_mgr_data = malloc(sizeof(data_mgr_data_t));
    data_mgr_data->buffer = buffer;
    data_mgr_data->pipe_write_end = pipe_write_end;


    //starting the threads
    pthread_create(&conn_mgr_thread_id, NULL, init_connection_manager, conn_mgr_data);
    pthread_create(&store_mgr_thread_id, NULL, init_storage_manager, storage_mgr_data);
    pthread_create(&data_mgr_thread_id, NULL, init_data_manager, data_mgr_data);

    //wait until every thread finished
    pthread_join(conn_mgr_thread_id, NULL);
    pthread_join(store_mgr_thread_id, NULL);
    pthread_join(data_mgr_thread_id, NULL);

    //end buffer ad pipe
    if (sbuffer_free(&buffer) == SBUFFER_FAILURE){
        return -1;
    }
    close(pipe_write_end);

    //free all the malloced structs
    free(conn_mgr_data);
    free(storage_mgr_data);
    free(data_mgr_data);

    return 0;
}