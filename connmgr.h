/**
 * \author Wout L.
 */

#include "sbuffer.h"

int init_connection_manager(int max_conn, int port, sbuffer_t *buffer);
void *connectionHandler(void *vargp);