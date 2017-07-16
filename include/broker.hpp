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
#include <queue>

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
        should_return = false;
    }
    virtual ~broker_base()
    {
        should_return = true;
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
    void stop()
    {
        should_return = true;
 
    }
    bool run()
    {
        if (frontend_IPPort.empty())
        {
            logger->error(ZMQ_LOG, "\[BROKER\] IP or Port is empty! please make sure you had set the IP and Port\n");
            return false;
        }
        try
        {
            /***front end***/
            // enable IPV6, we had already make sure that we are using TCP then we can set this option
            int enable_v6 = 1;
            frontend_socket_.setsockopt(ZMQ_IPV6, &enable_v6, sizeof(enable_v6));
            /*Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.*/
            int iRcvSendTimeout = 5000; // millsecond Make it configurable
            frontend_socket_.setsockopt(ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
            frontend_socket_.setsockopt(ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));

            /***back end***/
            // enable IPV6, we had already make sure that we are using TCP then we can set this option
            backend_socket_.setsockopt(ZMQ_IPV6, &enable_v6, sizeof(enable_v6));
            /*Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.*/
            backend_socket_.setsockopt(ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
            backend_socket_.setsockopt(ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, "\[BROKER\] set socket option return fail\n");
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
            logger->error(ZMQ_LOG, "\[BROKER\] frontend fail, IPPort is %s", (frontend_protocol + frontend_IPPort).c_str());
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
            if (should_return)
            {
                try
                {
                    int linger = 0;
                    frontend_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
                    backend_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
                }
                catch (std::exception &e)
                {
                    logger->error(ZMQ_LOG, "\[BROKER\] set ZMQ_LINGER return fail\n");
                }
                //frontend_socket_.close();
                //backend_socket_.close();
                //ctx_.close();
                return false;
            }
            try
            {
                zmq::pollitem_t items[] = {
                    {backend_socket_, 0, ZMQ_POLLIN, 0},
                    {frontend_socket_, 0, ZMQ_POLLIN, 0}};
                // Poll frontend only if we have available workers
                if (queue_worker.size())
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
                    //std::lock_guard<M_MUTEX> backendlock(backend_mutex);

                    zmsg_ptr msg(new zmsg(backend_socket_));

                    std::string identity((char *)(msg->pop_front()).c_str());
                    logger->debug(ZMQ_LOG, "\[BROKER\] receive message from backend with ID: %s\n", identity.c_str());

                    //  Return reply to client if it's not a control message
                    if (msg->parts() == 1)
                    {
                        if (strcmp(msg->address(), "READY") == 0)
                        {
                            logger->debug(ZMQ_LOG, "\[BROKER\] receive READY message from backend, ID is : %s\n", identity.c_str());
                            s_worker_delete(identity);
                            s_worker_append(identity);
                        }
                        else
                        {
                            if (strcmp(msg->address(), "HEARTBEAT") == 0)
                            {
                                //s_worker_delete(identity);
                                //  note: be careful about the following code, for performance/ send/receive problem.
                                s_worker_append(identity);

                                logger->debug(ZMQ_LOG, "\[BROKER\] receive HEARTBEAT message from backend");
                                s_worker_refresh(identity);
                            }
                            else
                            {
                                logger->warn(ZMQ_LOG, "\[BROKER\] invalid message from %s\n", identity.c_str());
                                msg->dump();
                            }
                        }
                    }
                    else
                    {
                        {
                            std::lock_guard<M_MUTEX> glock(broker_mutex);
                            front_end_q_broker.emplace(msg);
                        }
                        s_worker_append(identity);
                    }

                    if (back_end_q_broker.size())
                    {

                        try
                        {
                            std::lock_guard<M_MUTEX> glock(broker_mutex);
                            logger->debug(ZMQ_LOG, "\[BROKER\] there is %d message to send\n", back_end_q_broker.size());
                            // check size again under the lock
                            while (back_end_q_broker.size())
                            {
                                (back_end_q_broker.front())->send(backend_socket_);
                                //(back_end_q_broker.front()).reset();
                                back_end_q_broker.pop();
                            }
                        }
                        catch (std::exception &e)
                        {
                            logger->error(ZMQ_LOG, "\[BROKER\] send message fail!\n");
                            continue;
                        }
                    }
                }
                if (items[1].revents & ZMQ_POLLIN)
                {
                    //  Now get next client request, route to next worker

                    zmsg_ptr msg(new zmsg(frontend_socket_));
                    // for debug
                    // this is for test
                    //logger->error(ZMQ_LOG, "\[BROKER\] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
                    //msg->dump();

                    std::string identity = std::string(s_worker_dequeue());
                    logger->debug(ZMQ_LOG, "\[BROKER\] receive message from frontend, send message to worker with ID : %s", identity.c_str());
                    msg->push_front((char *)identity.c_str());
                    {
                        std::lock_guard<M_MUTEX> glock(broker_mutex);
                        back_end_q_broker.emplace(msg);
                    }

                    if (front_end_q_broker.size())
                    {
                        try
                        {
                            std::lock_guard<M_MUTEX> glock(broker_mutex);
                            logger->debug(ZMQ_LOG, "\[BROKER\] there is %d message to send\n", front_end_q_broker.size());
                            // check size again under the lock
                            while (front_end_q_broker.size())
                            {
                                (front_end_q_broker.front())->send(frontend_socket_);
                                //(front_end_q_broker.front()).reset();
                                front_end_q_broker.pop();
                            }
                        }
                        catch (std::exception &e)
                        {
                            logger->error(ZMQ_LOG, "\[BROKER\] send message fail!\n");
                            continue;
                        }
                    }
                }

                // epoll timeout
                {
                    if (front_end_q_broker.size())
                    {
                        try
                        {
                            std::lock_guard<M_MUTEX> glock(broker_mutex);
                            logger->debug(ZMQ_LOG, "\[BROKER\] there is %d message to send\n", front_end_q_broker.size());
                            // check size again under the lock
                            while (front_end_q_broker.size())
                            {
                                (front_end_q_broker.front())->send(frontend_socket_);
                                //(front_end_q_broker.front()).reset();
                                front_end_q_broker.pop();
                            }
                        }
                        catch (std::exception &e)
                        {
                            logger->error(ZMQ_LOG, "\[BROKER\] send message fail!\n");
                            continue;
                        }
                    }
                    if (back_end_q_broker.size())
                    {
                        try
                        {
                            std::lock_guard<M_MUTEX> glock(broker_mutex);
                            logger->debug(ZMQ_LOG, "\[BROKER\] there is %d message to send\n", back_end_q_broker.size());
                            // check size again under the lock
                            while (back_end_q_broker.size())
                            {
                                (back_end_q_broker.front())->send(backend_socket_);
                                //(back_end_q_broker.front()).reset();
                                back_end_q_broker.pop();
                            }
                        }
                        catch (std::exception &e)
                        {
                            logger->error(ZMQ_LOG, "\[BROKER\] send message fail!\n");
                            continue;
                        }
                    }
                }

                //  Send heartbeats to idle workers if it's time
                if (s_clock() > heartbeat_at)
                {
                    logger->debug(ZMQ_LOG, "\[BROKER\] now it is time to send heart beat, %d workers avaliable\n", queue_worker.size());

                    for (std::vector<worker_t>::iterator it = queue_worker.begin(); it < queue_worker.end(); it++)
                    {
                        logger->debug(ZMQ_LOG, "\[BROKER\] send heart beat to ID: %s\n", it->identity.c_str());
                        zmsg_ptr msg(new zmsg("HEARTBEAT"));
                        msg->wrap(it->identity.c_str(), NULL);
                        {
                            std::lock_guard<M_MUTEX> glock(broker_mutex);
                            back_end_q_broker.emplace(msg);
                        }
                    }
                    heartbeat_at = s_clock() + HEARTBEAT_INTERVAL;
                }
                s_queue_purge();
            }
            catch (std::exception &e)
            {
                // log here
                logger->error(ZMQ_LOG, "\[BROKER\] start broker fail\n");
                return false;
            }
        }
        logger->error(ZMQ_LOG, "\[BROKER\] should not run into here!!!!!!!!!!!!!!\n");
    }

    // for queue_worker related

    //  Insert worker at end of queue_worker, reset expiry
    //  Worker must not already be in queue_worker
    void s_worker_append(std::string &identity)
    {
        bool found = false;
        for (std::vector<worker_t>::iterator it = queue_worker.begin(); it < queue_worker.end(); it++)
        {
            if (it->identity.compare(identity) == 0)
            {
                //logger->debug(ZMQ_LOG, "\[BROKER\] duplicate worker identity %s\n", identity.c_str());
                found = true;
                break;
            }
        }
        if (!found)
        {
            worker_t worker;
            worker.identity = identity;
            worker.expiry = s_clock() + HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;
            queue_worker.push_back(worker);
        }
    }

    //  Remove worker from queue_worker, if present
    void s_worker_delete(std::string &identity)
    {
        for (std::vector<worker_t>::iterator it = queue_worker.begin(); it < queue_worker.end(); it++)
        {
            if (it->identity.compare(identity) == 0)
            {
                it = queue_worker.erase(it);
                break;
            }
        }
    }

    //  Reset worker expiry, worker must be present
    void s_worker_refresh(std::string &identity)
    {
        bool found = false;
        for (std::vector<worker_t>::iterator it = queue_worker.begin(); it < queue_worker.end(); it++)
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
            logger->warn(ZMQ_LOG, "\[BROKER\] worker %s not ready", identity.c_str());
        }
    }

    //  Pop next available worker off queue_worker, return identity
    std::string s_worker_dequeue()
    {
        assert(queue_worker.size());
        std::string identity = queue_worker[0].identity;
        queue_worker.erase(queue_worker.begin());
        return identity;
    }

    //  Look for & kill expired workers
    void s_queue_purge()
    {
        int64_t clock = s_clock();
        for (std::vector<worker_t>::iterator it = queue_worker.begin(); it < queue_worker.end(); it++)
        {
            if (clock > it->expiry)
            {
                it = queue_worker.erase(it) - 1;
            }
        }
    }

  private:
    bool should_return;

    std::string frontend_IPPort;
    std::string backend_IPPort;
    std::string frontend_protocol;
    std::string backend_protocol;

    M_MUTEX broker_mutex;

    std::queue<zmsg_ptr> front_end_q_broker;
    std::queue<zmsg_ptr> back_end_q_broker;
    std::vector<worker_t> queue_worker;

    zmq::context_t ctx_;
    zmq::socket_t frontend_socket_;
    zmq::socket_t backend_socket_;
};
