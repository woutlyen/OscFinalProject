/**
 * \author Wout Lyen
 */

#include "sbuffer.h"

typedef struct {
    int max_conn;
    int port;
    int pipe_write_end;
    sbuffer_t *buffer;
} conn_mgr_data_t;

void *init_connection_manager(void *vargp);
void *connectionHandler(void *vargp);