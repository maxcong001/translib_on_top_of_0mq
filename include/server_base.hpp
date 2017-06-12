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

  private:
    void start()
    {
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
        }
        if (zmq_setsockopt(server_socket_, ZMQ_SNDTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
        }

        int linger = 0;
        server_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
        server_socket_.bind("tcp://127.0.0.1:5570");

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
                    msg.send(server_socket_);
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
    zmq::context_t ctx_;
    zmq::socket_t server_socket_;
};