#include "../xps.h"

xps_core_t* xps_core_create() {
    xps_core_t* core = malloc(sizeof(xps_core_t));
    if (core == NULL) {
        logger(LOG_ERROR, "xps_core_create()", "malloc() failed for 'core'");
        perror("Error message");
        return NULL;
    }

    xps_loop_t* loop = xps_loop_create(core);
    core->loop = loop;
    
    vec_init(&core->listeners);
    vec_init(&core->connections);

    core->n_null_connections = 0;
    core->n_null_listeners = 0;

    logger(LOG_DEBUG, "xps_core_create()", "core created");
    return core;
}

void xps_core_destroy(xps_core_t* core) {
    assert(core != NULL);

    for (int i = 0; i < core->connections.length; i++) {
        xps_connection_t* current_connection = core->connections.data[i];
        if (current_connection != NULL)
            xps_connection_destroy(current_connection);
    }

    for (int i = 0; i < core->listeners.length; i++) {
        xps_listener_t* current_listener = core->listeners.data[i];
        if (current_listener != NULL)
            xps_listener_destroy(current_listener);
    }

    vec_deinit(&core->connections);
    vec_deinit(&core->listeners);

    xps_loop_destroy(core->loop);

    free(core);

    logger(LOG_DEBUG, "xps_core_destroy()", "core destroyed");
}

void xps_core_start(xps_core_t* core) {
    assert(core != NULL);

    logger(LOG_DEBUG, "xps_core_start()", "core starting");

    for (int port = 8001; port <= 8004; port++) {
        xps_listener_t* listener = xps_listener_create(core, "0.0.0.0", port);
        if (listener == NULL) {
            logger(LOG_ERROR, "xps_core_start()", "xps_listener_create() failed");
            return;
        }
        logger(LOG_INFO, "xps_core_start()", "Server running on http://0.0.0.0:%d", port);
    }

    xps_loop_run(core->loop);
}

