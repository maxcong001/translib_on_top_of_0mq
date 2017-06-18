#include "util.hpp"
#include <iostream>
#include <string>
#include <memory>

int get_monitor_event(void *monitor, int *value, char **address)
{
    // First frame in message contains event number and value
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    if (zmq_msg_recv(&msg, monitor, 0) == -1)
        return -1; // Interrupted, presumably
    assert(zmq_msg_more(&msg));
    uint8_t *data = (uint8_t *)zmq_msg_data(&msg);
    uint16_t event = *(uint16_t *)(data);
    if (value)
        *value = *(uint32_t *)(data + 2);
    // Second frame in message contains event address
    zmq_msg_init(&msg);
    if (zmq_msg_recv(&msg, monitor, 0) == -1)
        return -1; // Interrupted, presumably
    assert(!zmq_msg_more(&msg));
    if (address)
    {
        uint8_t *data = (uint8_t *)zmq_msg_data(&msg);
        size_t size = zmq_msg_size(&msg);
        *address = (char *)malloc(size + 1);
        memcpy(*address, data, size);
        (*address)[size] = 0;
    }
    return event;
}