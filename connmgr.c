/**
 * \author Wout L.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/time.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "connmgr.h"

/**
 * Implements a sequential test server (only one connection at the same time)
 */

typedef struct {
    tcpsock_t *client;
    sbuffer_t *buffer;
} thread_data_t;

int init_connection_manager(int max_conn, int port, sbuffer_t *buffer) {
    
    int MAX_CONN = max_conn;
    int PORT = port;

    pthread_t tid[MAX_CONN];
    tcpsock_t *server, *client[MAX_CONN];
    int conn_counter;

    thread_data_t *data = malloc(sizeof(thread_data_t));

    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    for (conn_counter = 0; conn_counter < MAX_CONN; conn_counter++) {
        if (tcp_wait_for_connection(server, &client[conn_counter]) != TCP_NO_ERROR) exit(EXIT_FAILURE);

        data->client=client[conn_counter];
        data->buffer=buffer;
        pthread_create(&tid[conn_counter], NULL, connectionHandler, data);
    }

    for (conn_counter = 0; conn_counter < MAX_CONN; conn_counter++) {
        pthread_join(tid[conn_counter], NULL);
    }

    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Test server is shutting down\n");



    sensor_data_t *sensor_data = malloc(sizeof(sensor_data_t));

    sensor_data->id = 0;
    sensor_data->value = 0;
    sensor_data->ts = 0;

    sbuffer_insert(buffer, sensor_data);

    while(1){

        if(sbuffer_remove(buffer,sensor_data) == SBUFFER_NO_DATA){
            break;
        }

        printf("sensor id = %" PRIu16 " - temperature = %f - timestamp = %ld\n", sensor_data->id, sensor_data->value,
                (long int) sensor_data->ts);

    }
    free(sensor_data);

    return 0;
}


void *connectionHandler(void *vargp){
    int bytes, result;
    sensor_data_t data;

    thread_data_t *thread_data = (thread_data_t *)vargp;
    tcpsock_t *client = thread_data->client;
    sbuffer_t *buffer = thread_data->buffer;

    //struct timeval t1, t2;
    //double elapsedTime;
    //gettimeofday(&t1, NULL);

    //printf("Incoming client connection\n");
    do {
        // read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void *) &data.id, &bytes);
        // read temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void *) &data.value, &bytes);
        // read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void *) &data.ts, &bytes);
        if ((result == TCP_NO_ERROR) && bytes) {

            sbuffer_insert(buffer, &data);

            printf("sensor id = %" PRIu16 " - temperature = %f - timestamp = %ld\n", data.id, data.value,
                   (long int) data.ts);
            //gettimeofday(&t1, NULL);
        }

        /*
        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
        printf("%f\n", elapsedTime);
        if(elapsedTime > 5000){
            break;
        }
         */

    } while (result == TCP_NO_ERROR);

    /*
    if (result == TCP_CONNECTION_CLOSED)
        printf("Peer has closed connection\n");
    else
        printf("Error occured on connection to peer\n");
    */

    tcp_close(&client);
    return 0;
}




