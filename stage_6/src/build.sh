#!/bin/bash
if [ $# -eq 0 ]; then
    export XPS_DEBUG=0
else 
    export XPS_DEBUG=$1
fi

gcc -g -o xps main.c lib/vec/vec.c network/xps_connection.c network/xps_listener.c utils/xps_logger.c utils/xps_utils.c   
