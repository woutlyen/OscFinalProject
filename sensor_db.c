/**
 * \author Wout Lyen
 */

#include <unistd.h>
#include <inttypes.h>
#include "sensor_db.h"

int storage_mgr_fd = -1;
char storage_mgr_log_message[SIZE];

int open_db(){
    //empty the csv file
    if(fclose(fopen("data.csv", "w")) != 0){
        return -1;
    }

    //log that a new file is created
    sprintf(storage_mgr_log_message, "A new data.csv file has been created");
    write(storage_mgr_fd, storage_mgr_log_message, SIZE);
    return 0;
}



void *init_storage_manager(void *vargp){

    //Reading out the provided variables
    storage_mgr_data_t *data = (storage_mgr_data_t*)vargp;
    sbuffer_t *buffer = data->buffer;
    storage_mgr_fd = data->pipe_write_end;
    sensor_data_t *sensor_data = malloc(sizeof(sensor_data_t));

    //start database
    open_db();

    //Reading and adding all the assigned sensor data in the csv file
    while(sbuffer_remove(buffer, sensor_data) != SBUFFER_NO_DATA){

        //open, write and close the file
        FILE *file = fopen("data.csv", "a");
        fprintf(file, "%hu, %f, %ld\n", sensor_data->id, sensor_data->value, sensor_data->ts);
        fclose(file);

        //log that data is inserted
        sprintf(storage_mgr_log_message, "Data insertion from sensor %" PRIu16 " succeeded", sensor_data->id);
        write(storage_mgr_fd, storage_mgr_log_message, SIZE);
    }

    //stop the db and free all malloced data
    close_db();
    free(sensor_data);
    return 0;
}

int close_db(){

    //log that the db stopped
    sprintf(storage_mgr_log_message, "The data.csv file has been closed");
    write(storage_mgr_fd, storage_mgr_log_message, SIZE);
    return 0;
}
