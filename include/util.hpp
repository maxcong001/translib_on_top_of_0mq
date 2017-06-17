#pragma once
#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"
/*
this file contains the util functions

*/

void client_cb(zmsg &input);
void server_cb(zmsg &input);
