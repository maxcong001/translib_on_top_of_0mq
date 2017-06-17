#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <string>

#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"

class client_base
{
  public:
    typedef std::function<void(zmsg &)> CLIENT_CB_FUNC;

    // start the 0MQ contex with 1 thread and max 1023 socket
    // you need to set IPPort info and then call run() when before
    client_base()
        : ctx_(1),
          client_socket_(ctx_, ZMQ_DEALER)
    {
    }
    client_base(std::string IPPort) : ctx_(1),
                                      client_socket_(ctx_, ZMQ_DEALER)
    {
        run();
    }

    size_t
    send(const char *msg, size_t len)
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
    void setIPPort(std::string ipport)
    {
        IP_and_port = ipport;
    }
    std::string getIPPort()
    {
        return IP_and_port;
    }
    void set_cb(CLIENT_CB_FUNC cb)
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
    // restart with new IP and port
    bool restart(std::string input)
    {
        /*
        if (IP_and_port == input)
        {
            return true;
        }
        client_socket_.close();

        IP_and_port = input;*/
    }

  private:
    bool start()
    {
        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        int enable_v6 = 1;
        if (zmq_setsockopt(client_socket_, ZMQ_IPV6, &enable_v6, sizeof(enable_v6)) < 0)
        {
            zmq_close(client_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }
        /*
        // generate random identity
        char identity[10] = {};
        sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
        printf("%s\n", identity);
        client_socket_.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
        */

        int linger = 0;
        if (zmq_setsockopt(client_socket_, ZMQ_LINGER, &linger, sizeof(linger)) < 0)
        {
            zmq_close(client_socket_);
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
        int iRcvSendTimeout = 5000; // millsecond Make it configurable

        if (zmq_setsockopt(client_socket_, ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            zmq_close(client_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }
        if (zmq_setsockopt(client_socket_, ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            zmq_close(client_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }
        try
        {
            client_socket_.connect(IP_and_port);
        }
        catch (std::exception &e)
        {
            // log here, connect fail
            return false;
        }

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
                    // Note: we should not do heavy work in this thread!!!!
                    //std::cout << "receive message form server, body is " << msg.body() << std::endl;
                    if (cb_)
                    {
                        cb_(msg);
                    }
                }
            }
            catch (std::exception &e)
            {
                // log here
            }
        }
    }

  private:
    std::string IP_and_port;
    CLIENT_CB_FUNC cb_;
    zmq::context_t ctx_;
    zmq::socket_t client_socket_;
};