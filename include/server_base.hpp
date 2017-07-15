#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>
#include <map>
#include <queue>
#include <condition_variable>
#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"
#include <memory>
#include <util.hpp>
#include <unistd.h>

class server_base
{

  public:
    server_base()
        : uniqueID_atomic(1)
    {

        server_socket_ = NULL;
        ctx_ = NULL;

        monitor_cb = NULL;
        routine_thread = NULL;
        monitor_thread = NULL;
        should_exit_monitor_task = false;
        should_exit_routine_task = false;
        monitor_path.clear();
        // set random monitor path
        if (monitor_path.empty())
        {
            /*  set random ID */
            std::stringstream ss;
            ss << std::hex << std::uppercase
               << std::setw(4) << std::setfill('0') << within(0x10000) << "-"
               << std::setw(4) << std::setfill('0') << within(0x10000);
            monitor_path = "inproc://" + ss.str();
        }
        protocol = "tcp://";
    }
    ~server_base()
    {
        should_exit_monitor_task = true;
        should_exit_routine_task = true;

        //delete Id2MsgMap_server;
        //Id2MsgMap_server.reset();
    }
    void set_protocol(std::string protocol_)
    {
        protocol = protocol_;
    }
    std::string get_protocol()
    {
        return protocol;
    }
    bool run();

    void setIPPort(std::string ipport)
    {
        IP_and_port = ipport;
    }
    std::string getIPPort()
    {
        return IP_and_port;
    }

    void set_cb(SERVER_CB_FUNC cb)
    {
        if (cb)
        {
            cb_ = cb;
        }
        else
        {
            logger->error(ZMQ_LOG, "\[SERVER\] invalid callback function\n");
        }
    }

    size_t send(const char *msg, size_t len, void *ID);
    void set_monitor_cb(MONITOR_CB_FUNC cb)
    {
        if (cb)
        {
            monitor_cb = cb;
        }
        else
        {
            logger->error(ZMQ_LOG, "\[SERVER\] invalid callback fucntion\n");
        }
    }

    void *getUniqueID() { return (void *)(uniqueID_atomic++); };

  private:
    bool monitor_task();
    bool monitor_this_socket()
    {
        int rc = zmq_socket_monitor(server_socket_->ptr, monitor_path.c_str(), ZMQ_EVENT_ALL);
        return ((rc == 0) ? true : false);
    }
    size_t send(zmsg &input)
    {
        input.send(*server_socket_);
    }
    size_t send(const char *msg, size_t len)
    {
        server_socket_->send(msg, len);
    }
    bool start();

  private:
    std::string monitor_path;
    std::string IP_and_port;
    std::string protocol;

    std::atomic<long> uniqueID_atomic;

    std::thread *routine_thread;
    std::thread *monitor_thread;

    bool should_exit_monitor_task;
    bool should_exit_routine_task;

    zmq::context_t *ctx_;
    zmq::socket_t *server_socket_;

    SERVER_CB_FUNC *cb_;
    MONITOR_CB_FUNC *monitor_cb;

    std::shared_ptr<std::mutex> server_mutex;
    std::shared_ptr<std::map<void *, zmsg_ptr>> Id2MsgMap_server;
    std::shared_ptr<std::queue<zmsg_ptr>> server_q;
};