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
#include <unistd.h>

class broker_base
{
  public:
    broker_base()
        : ctx_(1),
          frontend_socket_(ctx_, ZMQ_ROUTER), backend_socket_(ctx_, ZMQ_ROUTER)
    {
        frontend_protocol = "tcp://";
        backend_protocol = "tcp://";
        frontend_IPPort = "127.0.0.1:5561";
        backend_IPPort = "127.0.0.1:5560";
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

    void set_frontend_protocol(std::string protocol)
    {
        frontend_protocol = protocol;
    }
    std::string get_frontend_protocol()
    {
        return frontend_protocol;
    }
    void set_backtend_protocol(std::string protocol)
    {
        backend_protocol = protocol;
    }
    std::string get_backtend_protocol()
    {
        return backend_protocol;
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
            logger->error(ZMQ_LOG, "IP or Port is empty! please make sure you had set the IP and Port\n");
            return false;
        }

        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        int enable_v6 = 1;
        if (zmq_setsockopt(frontend_socket_, ZMQ_IPV6, &enable_v6, sizeof(enable_v6)) < 0)
        {
            frontend_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set ZMQ_IPV6 return fail\n");
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
            logger->error(ZMQ_LOG, "set ZMQ_LINGER return fail\n");
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
            logger->error(ZMQ_LOG, "set ZMQ_RCVTIMEO return fail\n");
            return false;
        }
        if (zmq_setsockopt(frontend_socket_, ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            frontend_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set ZMQ_SNDTIMEO return fail\n");
            return false;
        }

        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        //int enable_v6 = 1;
        if (zmq_setsockopt(backend_socket_, ZMQ_IPV6, &enable_v6, sizeof(enable_v6)) < 0)
        {
            backend_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set ZMQ_IPV6 for back-end return fail\n");
            return false;
        }
        /*
        // generate random identity
        char identity[10] = {};
        sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
        printf("%s\n", identity);
        backend_socket_.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
        */

        //int linger = 0;
        if (zmq_setsockopt(backend_socket_, ZMQ_LINGER, &linger, sizeof(linger)) < 0)
        {
            backend_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set ZMQ_LINGER for back-end return fail\n");
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
        //int iRcvSendTimeout = 5000; // millsecond Make it configurable

        if (zmq_setsockopt(backend_socket_, ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            backend_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set ZMQ_RCVTIMEO for back-end return fail\n");
            return false;
        }
        if (zmq_setsockopt(backend_socket_, ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            backend_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set ZMQ_SNDTIMEO for back-end return fail\n");

            return false;
        }

        try
        {
            logger->debug(ZMQ_LOG, "\[BROKER\] frontend bind to : %s", (frontend_protocol + frontend_IPPort).c_str());
            frontend_socket_.bind(frontend_protocol + frontend_IPPort);
        }
        catch (std::exception &e)
        {
            // log here
            logger->error(ZMQ_LOG, "frontend fail, IPPort is %s", (frontend_protocol + frontend_IPPort).c_str());
            return false;
        }
        try
        {
            logger->debug(ZMQ_LOG, "\[BROKER\] backend bind to : %s", (backend_protocol + backend_IPPort).c_str());
            backend_socket_.bind(backend_protocol + backend_IPPort);
        }
        catch (std::exception &e)
        {
            // log here
            logger->error(ZMQ_LOG, "\[BROKER\] backend fail, IPPort is %s", (backend_protocol + backend_IPPort).c_str());
            return false;
        }
        //  Send out heartbeats at regular intervals
        int64_t heartbeat_at = s_clock() + HEARTBEAT_INTERVAL;
        while (1)
        {
            try
            {
                zmq::pollitem_t items[] = {
                    {backend_socket_, 0, ZMQ_POLLIN, 0},
                    {frontend_socket_, 0, ZMQ_POLLIN, 0}};
                // Poll frontend only if we have available workers
                if (queue.size())
                {
                    zmq::poll(items, 2, HEARTBEAT_INTERVAL);
                }
                else
                {
                    zmq::poll(items, 1, HEARTBEAT_INTERVAL);
                }

                // do the main work
                //  Handle worker activity on backend
                if (items[0].revents & ZMQ_POLLIN)
                {

                    zmsg msg(backend_socket_);
                    std::string identity((char *)(msg.pop_front()).c_str());
                    logger->debug(ZMQ_LOG, "\[BROKER\] receive message from backend with ID: %s\n", identity.c_str());

                    //  Return reply to client if it's not a control message
                    if (msg.parts() == 1)
                    {
                        if (strcmp(msg.address(), "READY") == 0)
                        {
                            logger->debug(ZMQ_LOG, "\[BROKER\] receive READY message from backend, ID is : %s\n", identity.c_str());
                            s_worker_delete(identity);
                            s_worker_append(identity);
                        }
                        else
                        {
                            if (strcmp(msg.address(), "HEARTBEAT") == 0)
                            {
                                logger->debug(ZMQ_LOG, "\[BROKER\] receive HEARTBEAT message from backend");
                                s_worker_refresh(identity);
                            }
                            else
                            {
                                logger->warn(ZMQ_LOG, "invalid message from %s\n", identity.c_str());
                                msg.dump();
                            }
                        }
                    }
                    else
                    {
                        msg.send(frontend_socket_);
                        s_worker_append(identity);
                    }
                }
                if (items[1].revents & ZMQ_POLLIN)
                {
                    //  Now get next client request, route to next worker
                    zmsg msg(frontend_socket_);
                    std::string identity = std::string(s_worker_dequeue());
                    logger->debug(ZMQ_LOG, "\[BROKER\] receive message from frontend, send message to worker with ID : %s", identity.c_str());
                    msg.push_front((char *)identity.c_str());
                    msg.send(backend_socket_);
                }

                //  Send heartbeats to idle workers if it's time
                if (s_clock() > heartbeat_at)
                {
                    logger->debug(ZMQ_LOG, "\[BROKER\] now it is time to send heart beat, %d workers avaliable\n", queue.size());

                    for (std::vector<worker_t>::iterator it = queue.begin(); it < queue.end(); it++)
                    {
                        logger->debug(ZMQ_LOG, "\[BROKER\] send heart beat to ID: %s\n", it->identity.c_str());
                        zmsg msg("HEARTBEAT");
                        msg.wrap(it->identity.c_str(), NULL);
                        msg.send(backend_socket_);
                    }
                    heartbeat_at = s_clock() + HEARTBEAT_INTERVAL;
                }
                s_queue_purge();
            }
            catch (std::exception &e)
            {
                // log here
                logger->error(ZMQ_LOG, "start broker fail\n");
                return false;
            }
        }
        logger->error(ZMQ_LOG, "should not run into here!!!!!!!!!!!!!!\n");
    }

    // for queue related

    //  Insert worker at end of queue, reset expiry
    //  Worker must not already be in queue
    void s_worker_append(std::string &identity)
    {
        bool found = false;
        for (std::vector<worker_t>::iterator it = queue.begin(); it < queue.end(); it++)
        {
            if (it->identity.compare(identity) == 0)
            {
                logger->debug(ZMQ_LOG, "duplicate worker identity %s\n", identity.c_str());
                found = true;
                break;
            }
        }
        if (!found)
        {
            worker_t worker;
            worker.identity = identity;
            worker.expiry = s_clock() + HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;
            queue.push_back(worker);
        }
    }

    //  Remove worker from queue, if present
    void s_worker_delete(std::string &identity)
    {
        for (std::vector<worker_t>::iterator it = queue.begin(); it < queue.end(); it++)
        {
            if (it->identity.compare(identity) == 0)
            {
                it = queue.erase(it);
                break;
            }
        }
    }

    //  Reset worker expiry, worker must be present
    void s_worker_refresh(std::string &identity)
    {
        bool found = false;
        for (std::vector<worker_t>::iterator it = queue.begin(); it < queue.end(); it++)
        {
            if (it->identity.compare(identity) == 0)
            {
                it->expiry = s_clock() + HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;
                found = true;
                break;
            }
        }
        if (!found)
        {
            logger->warn(ZMQ_LOG, "worker %s not ready", identity.c_str());
        }
    }

    //  Pop next available worker off queue, return identity
    std::string s_worker_dequeue()
    {
        assert(queue.size());
        std::string identity = queue[0].identity;
        queue.erase(queue.begin());
        return identity;
    }

    //  Look for & kill expired workers
    void s_queue_purge()
    {
        int64_t clock = s_clock();
        for (std::vector<worker_t>::iterator it = queue.begin(); it < queue.end(); it++)
        {
            if (clock > it->expiry)
            {
                it = queue.erase(it) - 1;
            }
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
    // for queue
    //  Queue of available workers
    std::vector<worker_t> queue;
};