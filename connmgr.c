/**
 * \author Wout Lyen
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "connmgr.h"

/**
 * Implements a sequential test server (only one connection at the same time)
 */

sbuffer_t *buffer_connmgr;
int conn_mgr_fd = -1;

void *init_connection_manager(void *vargp) {

    //Reading out the provided variables
    conn_mgr_data_t *conn_mgr_data = (conn_mgr_data_t *)vargp;
    int MAX_CONN = conn_mgr_data->max_conn;
    int PORT = conn_mgr_data->port;
    conn_mgr_fd = conn_mgr_data->pipe_write_end;
    buffer_connmgr = conn_mgr_data->buffer;

    //init variables
    pthread_t tid[MAX_CONN];
    tcpsock_t *server, *client[MAX_CONN];
    int conn_counter = 0;

    printf("Test server is started\n");

    //open tcp connection
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    //assign a tcp connection to a thread
    do {
        if (tcp_wait_for_connection(server, &client[conn_counter]) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        pthread_create(&tid[conn_counter], NULL, connectionHandler, client[conn_counter]);
        conn_counter++;
    } while (conn_counter < MAX_CONN);
    conn_counter = 0;

    //wait until all threads finished
    do {
        pthread_join(tid[conn_counter], NULL);
        conn_counter++;
    } while (conn_counter < MAX_CONN);

    //close the tcp connection
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Test server is shutting down\n");


    //init end of buffer node
    sensor_data_t *sensor_data = malloc(sizeof(sensor_data_t));
    sensor_data->id = 0;
    sensor_data->value = 0;
    sensor_data->ts = 0;

    //insert end of buffer node
    sbuffer_insert(buffer_connmgr, sensor_data);

    //free malloced data
    free(sensor_data);

    //stop the thread
    pthread_exit(0);
}


void *connectionHandler(void *vargp){
    int bytes, result;
    sensor_data_t data;
    bool first_data = true;

    //Reading out the provided variables
    tcpsock_t *client = (tcpsock_t *)vargp;

    //read out tcp data
    do {
        // read sensor ID and check for timeout
        bytes = sizeof(data.id);
        result = tcp_receive_timeout(client, (void *) &data.id, &bytes, TIMEOUT);
        if (result == TCP_TIMEOUT_ERROR) {
            break;
        }

        // read temperature and check for timeout
        bytes = sizeof(data.value);
        result = tcp_receive_timeout(client, (void *) &data.value, &bytes, TIMEOUT);
        if (result == TCP_TIMEOUT_ERROR) {
            break;
        }

        // read timestamp and check for timeout
        bytes = sizeof(data.ts);
        result = tcp_receive_timeout(client, (void *) &data.ts, &bytes, TIMEOUT);
        if (result == TCP_TIMEOUT_ERROR) {
            break;
        }

        //check if no error occurred
        if ((result == TCP_NO_ERROR) && bytes) {

            //insert tcp data in the buffer
            sbuffer_insert(buffer_connmgr, &data);
            printf("sensor id = %" PRIu16 " - temperature = %f - timestamp = %ld\n", data.id, data.value, (long int) data.ts);
        }

        //check if this is the first data
        if (first_data){
            first_data = false;

            //print that there is a new connection
            char conn_mgr_log_message[SIZE] = "";
            sprintf(conn_mgr_log_message, "Sensor node %" PRIu16 " has opened a new connection", data.id);
            write(conn_mgr_fd, conn_mgr_log_message, SIZE);
        }

    } while (result == TCP_NO_ERROR);


    //log message based of result
    if (result == TCP_TIMEOUT_ERROR){
        printf("Time-out: Peer has closed connection\n");
        char conn_mgr_log_message[SIZE] = "";
        sprintf(conn_mgr_log_message, "Time-out: Sensor node %" PRIu16 " has closed the connection", data.id);
        write(conn_mgr_fd, conn_mgr_log_message, SIZE);
    }
    else if (result == TCP_CONNECTION_CLOSED) {
        printf("Peer has closed connection\n");
        char conn_mgr_log_message[SIZE] = "";
        sprintf(conn_mgr_log_message, "Sensor node %" PRIu16 " has closed the connection", data.id);
        write(conn_mgr_fd, conn_mgr_log_message, SIZE);
    }
    else
        printf("Error occured on connection to peer\n");

    //close tcp connection and stop the thread
    tcp_close(&client);
    pthread_exit(0);
}




