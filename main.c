/**
 * \author Wout L.
 */

#include <stdio.h>
#include <stdlib.h>

#include "connmgr.h"
#include "sbuffer.h"

sbuffer_t *buffer;

int main(int argc, char *argv[]){

    if(argc < 3) {
        printf("Please provide the right arguments: first the port, then the max nb of clients");
        return -1;
    }

    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    if (sbuffer_init(&buffer) == SBUFFER_FAILURE){
        return -1;
    }

    init_connection_manager(MAX_CONN, PORT, buffer);

    if (sbuffer_free(&buffer) == SBUFFER_FAILURE){
        return -1;
    }

    return 0;
}