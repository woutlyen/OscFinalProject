//
// Created by Wout Lyen on 14/11/23.
//
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "sensor_db.h"

int storage_mgr_fd = -1;
char storage_mgr_log_message[SIZE];

int open_db(){
    if(fclose(fopen("data.csv", "w")) != 0){
        return -1;
    }
    sprintf(storage_mgr_log_message, "A new data.csv file has been created");
    write(storage_mgr_fd, storage_mgr_log_message, SIZE);
    return 0;
}



void *init_storage_manager(void *vargp){

    storage_mgr_data_t *data = (storage_mgr_data_t*)vargp;
    sbuffer_t *buffer = data->buffer;
    storage_mgr_fd = data->pipe_write_end;
    sensor_data_t *sensor_data = malloc(sizeof(sensor_data_t));

    open_db();

    while(sbuffer_remove(buffer, sensor_data) != SBUFFER_NO_DATA){
        FILE *file = fopen("data.csv", "a");
        fprintf(file, "%hu, %f, %ld\n", sensor_data->id, sensor_data->value, sensor_data->ts);
        fclose(file);

        sprintf(storage_mgr_log_message, "Data insertion from sensor %" PRIu16 " succeeded", sensor_data->id);
        write(storage_mgr_fd, storage_mgr_log_message, SIZE);
    }

    close_db();
    return 0;
}

int close_db(){
    sprintf(storage_mgr_log_message, "The data.csv file has been closed");
    write(storage_mgr_fd, storage_mgr_log_message, SIZE);
    return 0;
}
