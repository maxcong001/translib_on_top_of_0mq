#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>
#include <map>
#include <condition_variable>
#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"
#include <memory>
#include <util.hpp>
// for test, delete later
#include <unistd.h>
#include <queue>

class worker_base
{
  public:
    worker_base()
        : uniqueID_atomic(1)
    {
        worker_socket_ = NULL;
        ctx_ = NULL;

        protocol = "tcp://";
        IP_and_port_dest = "127.0.0.1:5560";
        monitor_cb = NULL;
        routine_thread = NULL;
        monitor_thread = NULL;
        should_exit_monitor_task = false;
        should_exit_routine_task = false;
        // set random monitor path
        monitor_path.clear();
        if (monitor_path.empty())
        {
            /*  set random ID */
            std::stringstream ss;
            ss << std::hex << std::uppercase
               << std::setw(4) << std::setfill('0') << within(0x10000) << "-"
               << std::setw(4) << std::setfill('0') << within(0x10000);
            monitor_path = "inproc://" + ss.str();
        }
    }

    ~worker_base()
    {
        should_exit_monitor_task = true;
        should_exit_routine_task = true;
        if (monitor_thread)
        {

            monitor_thread->join();
        }
        if (routine_thread)
        {

            routine_thread->join();
        }
    }

    bool run();

    size_t send(const char *msg, size_t len, void *ID);
    void *getUniqueID() { return (void *)(uniqueID_atomic++); };

    void set_monitor_cb(MONITOR_CB_FUNC cb)
    {
        if (cb)
        {
            monitor_cb = cb;
        }
        else
        {
            logger->error(ZMQ_LOG, "\[WORKER\] invalid montior callback function \n");
        }
    }

    void set_protocol(std::string protocol_)
    {
        protocol = protocol_;
    }
    std::string get_protocol()
    {
        return protocol;
    }
    void setIPPort(std::string ipport)
    {
        IP_and_port_dest = ipport;
    }
    std::string getIPPort()
    {
        return IP_and_port_dest;
    }
    void setIPPortSource(std::string ipport)
    {
        IP_and_port_source = ipport;
    }
    std::string getIPPortSource()
    {
        return IP_and_port_source;
    }
    void set_cb(WORKER_CB_FUNC cb)
    {
        if (cb)
        {
            cb_ = cb;
        }
        else
        {
            logger->error(ZMQ_LOG, "\[WORKER\] invalid callback function\n");
        }
    }

  private:
    // ph1 mainly set the connection option and connect to the borker.
    bool start_ph1(zmq::socket_t *tmp_worker_socket_, std::string IP_Port);
    bool start();
    bool monitor_task();
    bool monitor_this_socket()
    {
        int rc = zmq_socket_monitor(worker_socket_->ptr, monitor_path.c_str(), ZMQ_EVENT_ALL);
        return ((rc == 0) ? true : false);
    }
    size_t send(zmsg &input)
    {
        input.send(*worker_socket_);
    }
    size_t send(const char *msg, size_t len)
    {
        worker_socket_->send(msg, len);
    }

  private:
    
    std::string monitor_path;
    std::string IP_and_port_dest;
    std::string protocol;
    std::string IP_and_port_source;

    std::atomic<long> uniqueID_atomic;

    std::thread *routine_thread;
    std::thread *monitor_thread;
    bool should_exit_monitor_task;
    bool should_exit_routine_task;

    std::string identity_;

    zmq::context_t *ctx_;
    zmq::socket_t *worker_socket_;

    WORKER_CB_FUNC *cb_;
    MONITOR_CB_FUNC *monitor_cb;

    std::shared_ptr<std::queue<zmsg_ptr>> worker_q;
    std::shared_ptr<std::map<void *, zmsg_ptr>> Id2MsgMap;
    std::shared_ptr<std::mutex> worker_mutex;
};
