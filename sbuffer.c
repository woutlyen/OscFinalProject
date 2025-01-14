/**
 * \author Wout Lyen
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "sbuffer.h"

pthread_mutex_t lock;
pthread_cond_t cond;

pthread_mutex_t read_lock;
pthread_cond_t read_cond;

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
};

int condition = 0;

int sbuffer_init(sbuffer_t **buffer) {

    //init mutexes and conditions
    if (pthread_mutex_init(&lock, NULL) != 0) {
        return SBUFFER_FAILURE;
    }

    if (pthread_cond_init(&cond, NULL) != 0) {
        return SBUFFER_FAILURE;
    }

    if (pthread_mutex_init(&read_lock, NULL) != 0) {
        return SBUFFER_FAILURE;
    }

    if (pthread_cond_init(&read_cond, NULL) != 0) {
        return SBUFFER_FAILURE;
    }

    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
    sbuffer_node_t *dummy;
    if ((buffer == NULL) || (*buffer == NULL)) {
        return SBUFFER_FAILURE;
    }
    while ((*buffer)->head) {
        dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }
    free(*buffer);
    *buffer = NULL;

    //destroy all the mutexes and conditions
    pthread_cond_destroy(&read_cond);
    pthread_mutex_destroy(&read_lock);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);

    return SBUFFER_SUCCESS;
}


int sbuffer_get_data(sbuffer_t *buffer, sensor_data_t *data) {

    //check if buffer is empty
    if (buffer == NULL) {
        return SBUFFER_FAILURE;
    }


    pthread_mutex_lock(&read_lock);
    while (condition == 1){
        pthread_cond_wait(&read_cond, &read_lock);
    }
    condition = 1;

    pthread_mutex_lock(&lock);

    while (buffer->head == NULL) {
        pthread_cond_wait(&cond, &lock);
    }

    *data = buffer->head->data;

    pthread_cond_signal(&read_cond);
    if (data->id == 0){
        pthread_mutex_unlock(&read_lock);
        pthread_mutex_unlock(&lock);
        return SBUFFER_NO_DATA;
    }
    pthread_mutex_unlock(&read_lock);
    pthread_mutex_unlock(&lock);

    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data) {

    sbuffer_node_t *dummy;
    if (buffer == NULL) {
        return SBUFFER_FAILURE;
    }

    pthread_mutex_lock(&read_lock);
    while (condition == 0){
        pthread_cond_wait(&read_cond, &read_lock);
    }
    condition = 0;

    pthread_mutex_lock(&lock);

    while (buffer->head == NULL) {
        pthread_cond_wait(&cond, &lock);
    }

    *data = buffer->head->data;

    pthread_cond_signal(&read_cond);
    if (data->id == 0){
        pthread_mutex_unlock(&read_lock);
        pthread_mutex_unlock(&lock);
        return SBUFFER_NO_DATA;
    }

    dummy = buffer->head;
    if (buffer->head == buffer->tail) // buffer has only one node
    {
        buffer->head = buffer->tail = NULL;
    } else  // buffer has many nodes empty
    {
        buffer->head = buffer->head->next;
    }
    free(dummy);

    pthread_mutex_unlock(&read_lock);
    pthread_mutex_unlock(&lock);

    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {

    sbuffer_node_t *dummy;
    if (buffer == NULL) {
        return SBUFFER_FAILURE;
    }
    dummy = malloc(sizeof(sbuffer_node_t));

    if (dummy == NULL) {
        return SBUFFER_FAILURE;
    }

    pthread_mutex_lock(&lock);

    dummy->data = *data;
    dummy->next = NULL;
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL)
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }

    if (data->id == 0){
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&lock);
        return SBUFFER_SUCCESS;
    }

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);

    return SBUFFER_SUCCESS;
}
