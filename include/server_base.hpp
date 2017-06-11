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
                    std::cout << "receive message form client" << std::endl;
                    msg.dump();
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
    zmq::context_t ctx_;
    zmq::socket_t server_socket_;
};