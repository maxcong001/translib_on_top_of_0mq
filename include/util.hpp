#pragma once
#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"
#include <memory>
/*
this file contains the util functions

*/
typedef std::shared_ptr<zmsg> zmsg_ptr;
typedef std::function<void(zmsg &)> CLIENT_CB_FUNC;
typedef void USR_CB_FUNC(const char *, size_t, void *);

typedef void SERVER_CB_FUNC(const char *, size_t, void *);
void client_cb(zmsg &input);
void server_cb(zmsg_ptr msg);
