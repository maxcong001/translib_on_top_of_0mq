#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <string>
#include <unordered_set>
#include <queue>
#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"
#include "util.hpp"

class client_base
{
  public:
    struct usrdata_and_cb
    {
        void *usr_data;
        void *cb;
    };
    // start the 0MQ contex with 1 thread and max 1023 socket
    // you need to set IPPort info and then call run() when before
    client_base();

    ~client_base()
    {
        stop();
    }

    size_t send(void *usr_data, USR_CB_FUNC cb, const char *msg, size_t len)
    {
        return send(usr_data, cb, const_cast<char *>(msg), len);
    }

    size_t send(void *usr_data, USR_CB_FUNC cb, char *msg, size_t len);

    bool run();

    void set_monitor_cb(MONITOR_CB_FUNC cb);

    bool stop();

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

    // restart with new IP and port
    bool restart(std::string input)
    {
    }

  private:
    bool monitor_task();
    bool monitor_this_socket();
    bool start();

  private:
    std::string monitor_path;
    std::string IP_and_port_dest;
    std::string IP_and_port_source;
    std::string protocol;

    USR_CB_FUNC *cb_;
    zmq::context_t *ctx_;
    zmq::socket_t *client_socket_;
    std::thread *routine_thread;
    std::thread *monitor_thread;

    bool should_stop;
    bool should_exit_monitor_task;
    MONITOR_CB_FUNC_CLIENT monitor_cb;

    std::shared_ptr<std::mutex> client_mutex;
    std::shared_ptr<std::queue<zmsg_ptr>> queue_s_client;
};
