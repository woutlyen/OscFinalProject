/**
 * \author Bert Lagaisse
 */

#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <stdbool.h>
#include "sbuffer.h"

typedef struct {
    sbuffer_t *buffer;
    int pipe_write_end;
} storage_mgr_data_t;

int open_db();
void *init_storage_manager(void *vargp);
int close_db();


#endif /* _SENSOR_DB_H_ */