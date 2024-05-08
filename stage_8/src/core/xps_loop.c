#include "../xps.h"

int search_event(vec_void_t events, loop_event_t* event) {
    for (int i = 0; i < events.length; i++) {
        loop_event_t* current = events.data[i];
        if (current == event)
            return i;
    }
    return -1;
}


loop_event_t* loop_event_create(u_int fd, void* ptr, xps_handler_t read_cb, xps_handler_t write_cb, xps_handler_t close_cb) {
    assert(ptr != NULL);

    loop_event_t* event = malloc(sizeof(loop_event_t));
    if (event == NULL) {
        logger(LOG_ERROR, "loop_event_create()", "malloc() failed for 'event'");
        perror("Error message");
        return NULL;
    }

    event->fd = fd;
    event->ptr = ptr;
    event->read_cb = read_cb;
    event->write_cb = write_cb;
    event->close_cb = close_cb;

    logger(LOG_DEBUG, "loop_event_create()", "event created");

    return event;
}


void loop_event_destroy(loop_event_t* event) {
    assert(event != NULL);

    free(event);

    logger(LOG_DEBUG, "loop_event_destroy()", "event destroyed");
}


/**
 * Creates a new event loop instance associated with the given core
 * 
 * This function creates an epoll file descriptor, allocates memory for the xps_loop instance.
 * and inits its values
 * 
 * @param core : The core instance to which the loop belongs
 * @return A pointer to the newly created loop instance or NULL on failure
*/
xps_loop_t* xps_loop_create(xps_core_t* core) {
    assert(core != NULL);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        logger(LOG_ERROR, "xps_loop_create()", "epoll_create1() failed");
        perror("Error message");
        return NULL;
    }

    xps_loop_t* loop = malloc(sizeof(xps_loop_t));
    if (loop == NULL) {
        logger(LOG_ERROR, "xps_loop_create()", "malloc() failed for 'loop'");
        perror("Error message");
        close(epoll_fd);
        return NULL;
    }

    loop->core = core;
    loop->epoll_fd = epoll_fd;
    loop->n_null_events = 0;
    vec_init(&loop->events);

    logger(LOG_DEBUG, "xps_loop_create()", "loop created");

    return loop;
}

/**
 * Destroys the given loop instance and releases associated resources
 * 
 * This function destroys all loop_event_t instances present in loop->events list
 * closes the epoll file descriptor and releases memory allocated for the loop isntance
 * 
 * @param loop the loop instance to be destroyed
*/
void xps_loop_destroy(xps_loop_t* loop) {
    assert(loop != NULL);

    close(loop->epoll_fd);

    for (int i = 0; i < loop->events.length; i++) {
        loop_event_t* current_event = loop->events.data[i];
        if (current_event != NULL)
            loop_event_destroy(current_event);
    }

    vec_deinit(&loop->events);

    free(loop);

    logger(LOG_DEBUG, "xps_loop_destroy()", "loop destroyed");
}


/** 
 * Attaches a FD to be monitored using epoll
 * 
 * The function creates an instance of loop_event_t and attaches it to the epoll
 * Add the pointer to loop_event_t to the events list in the loop
 * 
 * @param loop : loop to which FD should be attached
 * @param fd : FD to be attached to epoll
 * @param event_flags : epoll event flags
 * @param ptr : Pointer to instance of xps_listener_t or xps_connection_t
 * @param read_cb : Callback function to be calledd on a read event
 * @return : OK on success and E_FAIL on error
*/
int xps_loop_attach(xps_loop_t* loop, u_int fd, int event_flags, void* ptr, xps_handler_t read_cb, xps_handler_t write_cb, xps_handler_t close_cb) {
    assert(loop != NULL);
    assert(ptr != NULL);

    loop_event_t* event = loop_event_create(fd, ptr, read_cb, write_cb, close_cb);
    if (event == NULL) {
        logger(LOG_ERROR, "xps_loop_attach()", "loop_create_event() failed");
        return E_FAIL;
    }

    struct epoll_event e;
    e.events = event_flags;
    e.data.fd = fd;
    e.data.ptr = event;

    int status = epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &e);
    if (status < 0) {
        logger(LOG_ERROR, "xps_loop_attach()", "epoll_ctl() failed");
        perror("Error message");
        return E_FAIL;
    }

    vec_push(&loop->events, event);
    logger(LOG_DEBUG, "xps_loop_attach()", "attached event to loop");

    return OK;
}

/**
 * Remove FD from epoll
 * 
 * Find the instance of loop_event_t from loop->events that matches fd param
 * and detach FD from epoll. Destroy the loop_event_t instance and set the pointer
 * to NULL in loop->events list. Increment loop->n_null_events.
 * 
 * @param loop : loop instance from which to detach fd
 * @param fd : FD to be detached
 * @return : OK on success and E_FAIL on error
*/
int xps_loop_detach(xps_loop_t* loop, u_int fd) {
    assert(loop != NULL);

    int loop_event_index = -1;
    for (int i = 0; i < loop->events.length; i++) {
        loop_event_t* current_event = loop->events.data[i];
        if (current_event && current_event->fd == fd) {
            loop_event_index = i;
            break;
        }
    }

    if (loop_event_index == -1) {
        logger(LOG_ERROR, "xps_loop_detach()", "event not found -- exiting");
        return E_FAIL;
    }

    int status = epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (status < 0) {
        logger(LOG_ERROR, "xps_loop_detach()", "epoll_ctl() failed");
        perror("Error message");
        return E_FAIL;
    }

    loop_event_t* event = loop->events.data[loop_event_index];
    loop_event_destroy(event);
    loop->events.data[loop_event_index] = NULL;

    loop->n_null_events++;

    logger(LOG_DEBUG, "xps_loop_detached", "loop detached");
    return OK;
}



void xps_loop_run(xps_loop_t* loop) {
    assert(loop != NULL);

    while(1) {
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait");
        int n_ready_fds = epoll_wait(loop->epoll_fd, loop->epoll_events, MAX_EPOLL_EVENTS, -1);
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait over");

        if (n_ready_fds < 0) {
            logger(LOG_ERROR, "xps_loop_run()", "epoll_wait() failed");
            perror("Error message");
            return;
        }
        for (int i = 0; i < n_ready_fds; i++) {
            logger(LOG_DEBUG, "xps_loop_run()", "handling event no: %d", i+1);

            struct epoll_event current_epoll_event = loop->epoll_events[i];
            loop_event_t* current_event = loop->epoll_events[i].data.ptr;

            int current_event_index = search_event(loop->events, current_event);
            if (current_event_index == -1) {
                logger(LOG_DEBUG, "handle_epoll_events()", "event not found -- skipping");
                continue;
            }

            if (current_epoll_event.events & (EPOLLERR | EPOLLHUP)) {
                logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / close");
                if (current_event->close_cb != NULL) 
                    current_event->close_cb(current_event->ptr);
            }

            if (loop->events.data[current_event_index] == NULL) {
                logger(LOG_DEBUG, "handle_epoll_events()", "event not found -- skipping");
                continue;
            }


            if(current_epoll_event.events & EPOLLIN) {
                logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / read");
                if (current_event->read_cb != NULL)
                    current_event->read_cb(current_event->ptr);
            }

            if (loop->events.data[current_event_index] == NULL) {
                logger(LOG_DEBUG, "handle_epoll_events()", "event not found -- skipping");
                continue;
            }
            
            
            if (current_epoll_event.events & EPOLLOUT) {
                logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / write");
                if (current_event->write_cb != NULL)
                    current_event->write_cb(current_event->ptr);
            }
        }
    }
}