#pragma once
#include "util.hpp"
#include <atomic>
#include <stdlib.h>
#include "client_base.hpp"
#include "server_base.hpp"
#include "worker.hpp"

std::atomic<long> message_count;
std::atomic<long> message_count_recv;
void *user_data = (void *)0;

size_t time_str(uint32_t secs, uint32_t msec, char *out_ptr, size_t sz);
void logging_cb(const char *file_ptr, int line, const char *func_ptr, Logger::Level lev, const char *msg);
void client_cb_001(const char *msg, size_t len, void *usr_data);
void client_monitor_func(int event, int value, std::string &address);
void server_monitor_func(int event, int value, std::string &address);

