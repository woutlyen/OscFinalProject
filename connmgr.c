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
    conn_mgr_data_t *conn_mgr_data = (conn_mgr_data_t *)vargp;

    int MAX_CONN = conn_mgr_data->max_conn;
    int PORT = conn_mgr_data->port;
    conn_mgr_fd = conn_mgr_data->pipe_write_end;
    buffer_connmgr = conn_mgr_data->buffer;

    pthread_t tid[MAX_CONN];
    tcpsock_t *server, *client[MAX_CONN];
    int conn_counter = 0;

    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    do {
        if (tcp_wait_for_connection(server, &client[conn_counter]) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        pthread_create(&tid[conn_counter], NULL, connectionHandler, client[conn_counter]);
        conn_counter++;
    } while (conn_counter < MAX_CONN);

    conn_counter = 0;

    do {
        pthread_join(tid[conn_counter], NULL);
        conn_counter++;
    } while (conn_counter < MAX_CONN);

    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Test server is shutting down\n");



    sensor_data_t *sensor_data = malloc(sizeof(sensor_data_t));

    sensor_data->id = 0;
    sensor_data->value = 0;
    sensor_data->ts = 0;

    sbuffer_insert(buffer_connmgr, sensor_data);

    /*
    while(1){

        if(sbuffer_remove(BUFFER,sensor_data) == SBUFFER_NO_DATA){
            break;
        }

        printf("sensor id = %" PRIu16 " - temperature = %f - timestamp = %ld\n", sensor_data->id, sensor_data->value,
                (long int) sensor_data->ts);

    }
     */
    free(sensor_data);

    pthread_exit(0);
    return 0;
}


void *connectionHandler(void *vargp){
    int bytes, result;
    sensor_data_t data;
    bool first_data = true;

    tcpsock_t *client = (tcpsock_t *)vargp;

    //printf("Incoming client connection\n");
    do {
        // read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive_timeout(client, (void *) &data.id, &bytes, TIMEOUT);
        //printf("%d\n", result);
        if (result == TCP_TIMEOUT_ERROR) {
            break;
        }

        // read temperature
        bytes = sizeof(data.value);
        result = tcp_receive_timeout(client, (void *) &data.value, &bytes, TIMEOUT);
        if (result == TCP_TIMEOUT_ERROR) {
            break;
        }

        // read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive_timeout(client, (void *) &data.ts, &bytes, TIMEOUT);
        if (result == TCP_TIMEOUT_ERROR) {
            break;
        }

        if ((result == TCP_NO_ERROR) && bytes) {
            sbuffer_insert(buffer_connmgr, &data);
            printf("sensor id = %" PRIu16 " - temperature = %f - timestamp = %ld\n", data.id, data.value, (long int) data.ts);
        }

        if (first_data){
            first_data = false;

            char conn_mgr_log_message[SIZE] = "";
            sprintf(conn_mgr_log_message, "Sensor node %" PRIu16 " has opened a new connection", data.id);
            write(conn_mgr_fd, conn_mgr_log_message, SIZE);
        }

    } while (result == TCP_NO_ERROR);


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

    tcp_close(&client);
    pthread_exit(0);
    return 0;
}




