/**
 * \Author: Wout Lyen
 */

#include "datamgr.h"
#include "lib/dplist.h"
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>

typedef struct {
    uint16_t sensorID;
    uint16_t roomID;
    double avg[RUN_AVG_LENGTH];
    time_t last_modified;
    int total_values;
} element_t;

int data_mgr_fd = -1;
char data_mgr_log_message[SIZE];
dplist_t *list;

void * element_copy(void * element) {
    element_t* copy = malloc(sizeof (element_t));
    ERROR_HANDLER(!copy, "Malloc of the element_t copy struct failed");
    //assert(copy != NULL);
    copy->roomID = ((element_t*)element)->roomID;
    copy->sensorID = ((element_t*)element)->sensorID;
    for (int count = 0; count < RUN_AVG_LENGTH; count++) {
        copy->avg[count] = ((element_t*)element)->avg[count];
    }
    copy->last_modified = ((element_t*)element)->last_modified;
    copy->total_values = ((element_t*)element)->total_values;
    return (void *) copy;
}

void element_free(void ** element) {
    free(*element);
    *element = NULL;
}

int element_compare(void * x, void * y) {
    return (((element_t*)x)->sensorID == ((element_t*)y)->sensorID ? 0 : 1);
}


void *init_data_manager(void *vargp){
    return 0;
    data_mgr_data_t *data = (data_mgr_data_t *)vargp;
    sbuffer_t *buffer = data->buffer;
    data_mgr_fd = data->pipe_write_end;
    sensor_data_t *sensor_data = malloc(sizeof(sensor_data_t));

    //Malloc one element_t to temporarily store the data of one line in the room_sensor_map file
    element_t *element = (element_t*)malloc(sizeof(element_t));
    ERROR_HANDLER(!element, "Malloc of the element_t struct failed");

    //Initialise dplist with feedback functions
    list = dpl_create(element_copy, element_free, element_compare);

    //Reading and adding all the assigned sensorIDs to roomIDs in the list
    FILE *fp_sensor_map = fopen("room_sensor.map", "r");
    char line[255];
    while (fgets(line, 255, fp_sensor_map) != NULL){
        sscanf(line,"%hu %hu", &(element->roomID), &(element->sensorID));
        element->total_values = 0;
        dpl_insert_at_index(list, element, dpl_size(list), true);

    }
    fclose(fp_sensor_map);


    //Reading and adding all the assigned sensor data in the list
    while(sbuffer_get_data(buffer, sensor_data) != SBUFFER_NO_DATA) {

        //Print the time_t in human language
        //printf("%s", asctime(gmtime(&timestamp)));

        for (int count = 0; count < dpl_size(list); count++) {
            int index;

            element_t *element_at_index = (element_t*)dpl_get_element_at_index(list, count);

            if (element_at_index->sensorID == sensor_data->id){

                index = element_at_index->total_values % RUN_AVG_LENGTH;
                element_at_index->avg[index] = sensor_data->value;

                element_at_index->total_values = element_at_index->total_values+1;

                if (element_at_index->total_values >= RUN_AVG_LENGTH){
                    double avg = 0;
                    for (int count = 0; count < RUN_AVG_LENGTH; count++) {
                        avg = avg + element_at_index->avg[count];
                    }
                    avg = avg/RUN_AVG_LENGTH;

                    if (avg > SET_MAX_TEMP){
                        sprintf(data_mgr_log_message, "Sensor node %" PRIu16 " reports it’s too hot (avg temp = %f)", sensor_data->id, avg);
                        write(data_mgr_fd, data_mgr_log_message, SIZE);
                    }
                    else if (avg < SET_MIN_TEMP){
                        sprintf(data_mgr_log_message, "Sensor node %" PRIu16 " reports it’s too cold (avg temp = %f)", sensor_data->id, avg);
                        write(data_mgr_fd, data_mgr_log_message, SIZE);
                    }
                }

                element_at_index->last_modified = sensor_data->ts;
                //printf("%hu %f %lld \n", element_at_index->sensorID, element_at_index->avg, (long long)element_at_index->last_modified);
                break;
            }

            if (count == dpl_size(list)-1){
                sprintf(data_mgr_log_message, "Received sensor data with invalid sensor node ID %" PRIu16, sensor_data->id);
                write(data_mgr_fd, data_mgr_log_message, SIZE);
            }
        }
    }

    free(element);
    free(sensor_data);

    return 0;
}


void datamgr_free(){
    dpl_free(&list, true);
}