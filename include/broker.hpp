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

class broker_base
{

  public:
    broker_base()
        : ctx_(1),
          frontend_socket_(ctx_, ZMQ_ROUTER), backend_socket_(ctx_, ZMQ_DEALER)
    {
        frontend_protocol = "tcp://";
        backend_protocol = "tcp://";
        frontend_IPPort = "*:5559";
        backend_IPPort = "*:5560"
    }
    virtual ~broker_base()
    {
    }
    void set_frontend_IPPort(std::string IPPort)
    {
        frontend_IPPort = IPPort;
    }
    std::string get_frontend_IPPort()
    {
        return frontend_IPPort;
    }
    void set_backtend_IPPort(std::string IPPort)
    {
        backend_IPPort = IPPort;
    }
    std::string get_backend_IPPort()
    {
        return backend_IPPort;
    }
    bool run()
    {
        if (frontend_IPPort.empty())
        {
            return false;
        }

        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        int enable_v6 = 1;
        if (zmq_setsockopt(frontend_socket_, ZMQ_IPV6, &enable_v6, sizeof(enable_v6)) < 0)
        {
            frontend_socket_.close();
            ctx_.close();
            return false;
        }
        /*
        // generate random identity
        char identity[10] = {};
        sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
        printf("%s\n", identity);
        frontend_socket_.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
        */

        int linger = 0;
        if (zmq_setsockopt(frontend_socket_, ZMQ_LINGER, &linger, sizeof(linger)) < 0)
        {
            frontend_socket_.close();
            ctx_.close();
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

        if (zmq_setsockopt(frontend_socket_, ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            frontend_socket_.close();
            ctx_.close();
            return false;
        }
        if (zmq_setsockopt(frontend_socket_, ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            frontend_socket_.close();
            ctx_.close();
            return false;
        }

        frontend_socket_.bind(frontend_protocol + frontend_IPPort);
        if (backend_IPPort.empty())
        {
            return false;
        }
        else
        {
            backend_socket_.bind(backend_protocol + frontend_IPPort);
        }
        try
        {
            zmq::proxy(frontend_socket_, backend_socket_, nullptr);
        }
        catch (std::exception &e)
        {
            // log here
        }
    }

  private:
    zmq::context_t ctx_;
    zmq::socket_t frontend_socket_;
    zmq::socket_t backend_socket_;
    std::string frontend_IPPort;
    std::string backend_IPPort;
    std::string frontend_protocol;
    std::string backend_protocol;
};