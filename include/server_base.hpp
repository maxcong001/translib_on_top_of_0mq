#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"

class server_base
{

  public:
    typedef std::function<void(zmsg &)> SERVER_CB_FUNC;
    server_base()
        : ctx_(1),
          server_socket_(ctx_, ZMQ_ROUTER)
    {
        // this is for test
        //seq_num = 0;
    }

    void run()
    {
        auto routine_fun = std::bind(&server_base::start, this);
        std::thread routine_thread(routine_fun);
        routine_thread.detach();
    }
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
            //log here
        }
    }

  private:
    bool start()
    {
        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        int enable_v6 = 1;
        if (zmq_setsockopt(server_socket_, ZMQ_IPV6, &enable_v6, sizeof(enable_v6)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }
        /*
        - Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.
        - Value is an uint32 in ms (to be compatible with windows and kept the
        implementation simple).
        - Default to 0, which would mean block infinitely.
        - On timeout, return EAGAIN.
        Note: Maxx will this work for DEALER mode?
        */
        int iRcvTimeout = 5000; // millsecond Make it configurable

        if (zmq_setsockopt(server_socket_, ZMQ_RCVTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }
        if (zmq_setsockopt(server_socket_, ZMQ_SNDTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }

        int linger = 0;
        if (zmq_setsockopt(server_socket_, ZMQ_LINGER, &linger, sizeof(linger)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }
        try
        {
            server_socket_.bind("tcp://127.0.0.1:5570");
        }
        catch (std::exception &e)
        {
            // need log here
            return false;
        }

        //  Initialize poll set
        zmq::pollitem_t items[] = {
            {server_socket_, 0, ZMQ_POLLIN, 0},
        };
        while (1)
        {
            try
            {
                zmq::poll(items, 1, -1);
                if (items[0].revents & ZMQ_POLLIN)
                {
                    zmsg msg(server_socket_);
                    // ToDo: now we got the message, do main work
                    //std::cout << "receive message form client" << std::endl;
                    //msg.dump();
                    // send back message to client, for test
                    //msg.send(server_socket_);
                    cb_(msg);
                }
            }
            catch (std::exception &e)
            {
            }
        }
    }
    size_t send(const char *msg, size_t len)
    {
        server_socket_.send(msg, len);
    }
    size_t send(char *msg, size_t len)
    {
        server_socket_.send(msg, len);
    }

  private:
    // this is for test
    //int seq_num;
    SERVER_CB_FUNC cb_;
    std::string IP_and_port;
    zmq::context_t ctx_;
    zmq::socket_t server_socket_;
};