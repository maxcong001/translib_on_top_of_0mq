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

class worker_base
{
  public:
    worker_base()
        : ctx_(1),
          worker_socket_(ctx_, ZMQ_DEALER), uniqueID_atomic(1)
    {
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
#if 0
        if (worker_socket_)
        {
            delete worker_socket_;
        }
        if (ctx_)
        {
            delete ctx_;
        }
#endif
        should_exit_monitor_task = true;
        should_exit_routine_task = true;
        if (routine_thread)
        {
            routine_thread->join();
        }
        if (monitor_thread)
        {
            monitor_thread->join();
        }
    }

    bool run()
    {
        auto routine_fun = std::bind(&worker_base::start, this);
        routine_thread = new std::thread(routine_fun);
        //routine_thread->detach();
        auto monitor_fun = std::bind(&worker_base::monitor_task, this);
        monitor_thread = new std::thread(monitor_fun);
        //monitor_thread->detach();
        // start monitor socket
        bool ret = monitor_this_socket();
        if (ret)
        {
        }
        else
        {
            logger->error(ZMQ_LOG, "start monitor socket fail!\n");
            return false;
        }
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
    void set_cb(SERVER_CB_FUNC cb)
    {
        if (cb)
        {
            cb_ = cb;
        }
        else
        {
            logger->error(ZMQ_LOG, "invalid callback function\n");
        }
    }

    size_t send(const char *msg, size_t len, void *ID)
    {
        auto iter = Id2MsgMap.find(ID);
        if (iter != Id2MsgMap.end())
        {
            zmsg::ustring tmp_ustr((unsigned char *)msg, len);
            // to do add the send code
            (iter->second)->push_back(tmp_ustr);
            (iter->second)->send(worker_socket_);
            // make sure delete the memory of the message
            //(iter->second)->clear();
            (iter->second).reset();
            Id2MsgMap.erase(iter);
        }
        else
        {
            logger->error(ZMQ_LOG, "did not find the ID\n");
            return -1;
        }
    }

    void set_monitor_cb(MONITOR_CB_FUNC cb)
    {
        if (cb)
        {
            monitor_cb = cb;
        }
        else
        {
            logger->error(ZMQ_LOG, "invalid montior callback function \n");
        }
    }

    void *getUniqueID() { return (void *)(uniqueID_atomic++); };

  private:
    void epoll_task()
    {
#if 0
        std::unique_lock<std::mutex> monitor_lock(monitor_mutex);
        // to do, receive signal then do other thing.
        // if signal timeout, that means routine thread is abnormal. Or exit. start again.
        while (1)
        {
            {
                //monitor_cond.wait(monitor_lock);
                if (monitor_cond.wait_for(dnsLock, std::chrono::milliseconds(EPOLL_TIMEOUT + 5000)) == std::cv_status::timeout)
                {
                    // timeout waitting for signal. there must be something wrong with epoll
                    auto routine_fun = std::bind(&worker_base::start, this);
                    std::thread routine_thread(routine_fun);
                    routine_thread.detach();
                }
            }
        }
#endif
    }

    bool monitor_task()
    {

        void *server_mon = zmq_socket((void *)ctx_, ZMQ_PAIR);
        if (!server_mon)
        {
            logger->error(ZMQ_LOG, "0MQ get socket fail\n");
            return false;
        }
        try
        {

            logger->debug(ZMQ_LOG, "\[WORKER\] monitor path %s\n", monitor_path.c_str());
            int rc = zmq_connect(server_mon, monitor_path.c_str());

            //rc should be 0 if success
            if (rc)
            {
                logger->error(ZMQ_LOG, "0MQ connect fail\n");
                return false;
            }
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, "connect to monitor worker socket fail\n");
            return false;
        }

        while (1)
        {
            if (should_exit_monitor_task)
            {
                logger->warn(ZMQ_LOG, "will exit monitor task\n");
                return true;
            }
            std::string address;
            int value;
            int event = get_monitor_event(server_mon, &value, address);
            if (event == -1)
            {
                logger->warn(ZMQ_LOG, "get monitor event fail\n");
                //return false;
            }

            //std::cout << "receive event form server monitor task, the event is " << event << ". Value is : " << value << ". string is : " << address << std::endl;
            if (monitor_cb)
            {
                monitor_cb(event, value, address);
            }
        }
    }
    bool monitor_this_socket()
    {
        int rc = zmq_socket_monitor(worker_socket_, monitor_path.c_str(), ZMQ_EVENT_ALL);
        return ((rc == 0) ? true : false);
    }
    size_t send(zmsg &input)
    {
        input.send(worker_socket_);
    }
    size_t send(const char *msg, size_t len)
    {
        worker_socket_.send(msg, len);
    }
    // ph1 mainly set the connection option and connect to the borker.
    bool start_ph1()
    {
        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        int enable_v6 = 1;
        if (zmq_setsockopt(worker_socket_, ZMQ_IPV6, &enable_v6, sizeof(enable_v6)) < 0)
        {
            worker_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set socket option ZMQ_IPV6 fail\n");
            return false;
        }

        identity_ = s_set_id(worker_socket_);
        logger->debug(ZMQ_LOG, "\[WORKER\] set ID %s to worker\n", identity_.c_str());

        int linger = 0;
        if (zmq_setsockopt(worker_socket_, ZMQ_LINGER, &linger, sizeof(linger)) < 0)
        {
            worker_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set socket option ZMQ_LINGER fail\n");
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

        if (zmq_setsockopt(worker_socket_, ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            worker_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set socket option ZMQ_RCVTIMEO fail\n");
            return false;
        }
        if (zmq_setsockopt(worker_socket_, ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout)) < 0)
        {
            worker_socket_.close();
            ctx_.close();
            logger->error(ZMQ_LOG, "set socket option ZMQ_SNDTIMEO fail\n");

            return false;
        }

        try
        {
            std::string IPPort;
            // should be like this tcp://192.168.1.17:5555;192.168.1.1:5555

            if (IP_and_port_source.empty())
            {
                IPPort += "tcp://" + IP_and_port_dest;
            }
            else
            {
                IPPort += "tcp://" + IP_and_port_source + ";" + IP_and_port_dest;
            }

            logger->debug(ZMQ_LOG, "\[WORKER\] connect to : %s\n", IPPort.c_str());
            worker_socket_.connect(IPPort);
            // tell the broker that we are ready
            std::string ready_str("READY");
            send(ready_str.c_str(), ready_str.size());
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, "\[WORKER\] connect fail!!!!\n");
            return false;
        }
        return true;
    }
    bool start()
    {
        if (!start_ph1())
        {
            return false;
        }

        //  Send out heartbeats at regular intervals
        int64_t heartbeat_at = s_clock() + HEARTBEAT_INTERVAL;
        size_t liveness = HEARTBEAT_LIVENESS;
        size_t interval = INTERVAL_INIT;

        //  Initialize poll set

        while (1)
        {
            zmq::pollitem_t items[] = {
                {worker_socket_, 0, ZMQ_POLLIN, 0}};
            if (should_exit_routine_task)
            {
                logger->warn(ZMQ_LOG, "will exit monitor task\n");
                return true;
            }
            try
            {
                // by default we wait for 1000ms then so something. like hreatbeat
                zmq::poll(items, 1, HEARTBEAT_INTERVAL);
                if (items[0].revents & ZMQ_POLLIN)
                {

                    // this is for test, delete it later
                    //sleep(5);

                    zmsg_ptr msg(new zmsg(worker_socket_));
                    logger->debug(ZMQ_LOG, "\[WORKER\] get message from broker with %d part", msg->parts());
                    msg->dump();
                    // now we get the message .
                    // this is the normal message
                    if (msg->parts() == 3)
                    {
#if 0
                        ///// this is for test, simulate various problems after a fwe cycles
                        static int cycles;
                        cycles++;
/*
                        if (cycles > 3 && within(5) == 0)
                        {
                            logger->debug(ZMQ_LOG, "Simulate a crash, ID : %s, cycle is %d\n", identity_.c_str(), cycles);
                            msg->clear();
                            continue;
                        }
                        else
                        {*/
#if 0
                        if (cycles > 3 && within(5) == 0)
                        {
                            logger->debug(ZMQ_LOG, " simulating CPU overload, ID : %s, cycle is %d\n", identity_.c_str(), cycles);
                            sleep(5);
                        }
#endif
                        // }
                        //liveness = 1;
#endif
                        std::string data = msg->get_body();
                        if (data.empty())
                        {
                            logger->warn(ZMQ_LOG, "we get a message without body\n");
                            continue;
                        }
                        void *ID = getUniqueID();
                        Id2MsgMap.emplace(ID, msg);

                        // ToDo: now we got the message, do main work
                        //std::cout << "receive message form client" << std::endl;
                        //msg.dump();
                        // send back message to client, for test
                        //msg.send(worker_socket_);
                        if (cb_)
                        {
                            cb_(data.c_str(), data.size(), ID);
                        }
                        else
                        {
                            logger->error(ZMQ_LOG, " no valid callback function, please make sure you had set message callback fucntion\n");
                        }
                    }
                    else
                    {
                        if (msg->parts() == 1 && strcmp(msg->body(), "HEARTBEAT") == 0)
                        {
                            liveness = HEARTBEAT_LIVENESS;
                        }
                        else
                        {
                            logger->warn(ZMQ_LOG, "invalid message, %s\n", identity_.c_str());
                            msg->dump();
                        }
                    }
                    interval = INTERVAL_INIT;
                }
                // epoll time out
                else if (--liveness == 0)
                {

                    logger->warn(ZMQ_LOG, " heartbeat failure, can't reach queue, identity : (%s) \n", identity_.c_str());
                    logger->warn(ZMQ_LOG, " reconnecting in  %d msec..., identity : (%s)", interval, identity_.c_str());
                    s_sleep(interval);

                    if (interval < INTERVAL_MAX)
                    {
                        interval *= 2;
                    }
                    // stop the worker and start again
                    try
                    {
                        worker_socket_.close();
                        zmq::socket_t tmp_worker(ctx_, ZMQ_DEALER);
                        //worker = s_worker_socket(context);
                        worker_socket_ = std::move(tmp_worker);
                        if (!start_ph1())
                        {
                            logger->warn(ZMQ_LOG, " connect to broker return fail!\n");
                        }
                    }
                    catch (std::exception &e)
                    {
                        logger->error(ZMQ_LOG, " restart worker socket fail!\n");
                    }
                    liveness = HEARTBEAT_LIVENESS;
                }
                //  Send heartbeat to queue if it's time
                if (s_clock() > heartbeat_at)
                {
                    heartbeat_at = s_clock() + HEARTBEAT_INTERVAL;
                    logger->debug(ZMQ_LOG, " (%s) worker heartbeat\n", identity_.c_str());
                    s_send(worker_socket_, "HEARTBEAT");
                }
            }
            catch (std::exception &e)
            {
                logger->error(ZMQ_LOG, " something wrong while polling message\n");
            }
        }
    }

  private:
    std::string identity_;
    std::string monitor_path;
    SERVER_CB_FUNC *cb_;
    MONITOR_CB_FUNC *monitor_cb;
    std::string IP_and_port_dest;
    std::string IP_and_port_source;
    zmq::context_t ctx_;
    zmq::socket_t worker_socket_;
    std::atomic<long> uniqueID_atomic;
    std::map<void *, zmsg_ptr> Id2MsgMap;

    std::condition_variable monitor_cond;
    std::mutex monitor_mutex;

    std::thread *routine_thread;
    std::thread *monitor_thread;
    bool should_exit_monitor_task;
    bool should_exit_routine_task;
};