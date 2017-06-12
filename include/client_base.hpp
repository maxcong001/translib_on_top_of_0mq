#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"

class client_base
{
    typedef std::function<void(char *input)> CB_FUNC;

  public:
    client_base()
        : ctx_(1),
          client_socket_(ctx_, ZMQ_DEALER)
    {
    }

    size_t send(const char *msg, size_t len)
    {
        client_socket_.send(msg, len);
    }
    size_t send(char *msg, size_t len)
    {
        client_socket_.send(msg, len);
    }
    void run()
    {
        auto routine_fun = std::bind(&client_base::start, this);
        std::thread routine_thread(routine_fun);
        routine_thread.detach();
    }

    void set_cb(CB_FUNC cb)
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
    void start()
    {
        /*
        // generate random identity
        char identity[10] = {};
        sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
        printf("%s\n", identity);
        client_socket_.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
        */

        int linger = 0;
        client_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));

        /*
        - Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.
        - Value is an uint32 in ms (to be compatible with windows and kept the
        implementation simple).
        - Default to 0, which would mean block infinitely.
        - On timeout, return EAGAIN.
        Note: Maxx will this work for DEALER mode?
        */
        int iRcvTimeout = 5000; // millsecond Make it configurable

        if (zmq_setsockopt(client_socket_, ZMQ_RCVTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
        {
            zmq_close(client_socket_);
            zmq_ctx_destroy(&ctx_);
        }
        if (zmq_setsockopt(client_socket_, ZMQ_SNDTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
        {
            zmq_close(client_socket_);
            zmq_ctx_destroy(&ctx_);
        }
        client_socket_.connect("tcp://127.0.0.1:5570");

        //  Initialize poll set
        zmq::pollitem_t items[] = {{client_socket_, 0, ZMQ_POLLIN, 0}};
        while (1)
        {
            try
            {
                // to do  now poll forever, we can set a timeout and then so something like heartbeat
                zmq::poll(items, 1, -1);
                if (items[0].revents & ZMQ_POLLIN)
                {
                    zmsg msg(client_socket_);
                    //std::cout << "receive message from server with " << msg.parts() << " parts" << std::endl;
                    //msg.dump();
                    // ToDo: now we got the message, do main work
                    //std::cout << "receive message form server, body is " << msg.body() << std::endl;
                    if (cb_)
                    {
                        cb_(msg.body());
                    }
                }
            }
            catch (std::exception &e)
            {
            }
        }
    }

  private:
    CB_FUNC cb_;
    zmq::context_t ctx_;
    zmq::socket_t client_socket_;
};