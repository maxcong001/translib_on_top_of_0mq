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
        : uniqueID_atomic(1)
    {
        worker_socket_ = NULL;
        ctx_ = NULL;

        protocol = "tcp://";
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
        should_exit_monitor_task = true;
        should_exit_routine_task = true;
        if (monitor_thread)
        {

            //monitor_thread->join();
        }
        if (routine_thread)
        {

            //routine_thread->join();
        }
    }

    bool run()
    {

        ctx_ = new zmq::context_t(1);
        if (!ctx_)
        {
            logger->error(ZMQ_LOG, "\[CLIENT\] new context fail!\n");
            return false;
        }
        worker_socket_ = new zmq::socket_t(*ctx_, ZMQ_DEALER);
        if (!worker_socket_)
        {
            logger->error(ZMQ_LOG, "\[CLIENT\] new socket fail!\n");
            return false;
        }

        auto routine_fun = std::bind(&worker_base::start, this);
        routine_thread = new std::thread(routine_fun);
        routine_thread->detach();
        auto monitor_fun = std::bind(&worker_base::monitor_task, this);
        monitor_thread = new std::thread(monitor_fun);
        monitor_thread->detach();
        // start monitor socket
        bool ret = monitor_this_socket();
        if (ret)
        {
        }
        else
        {
            logger->error(ZMQ_LOG, "\[WORKER\] start monitor socket fail!\n");
            return false;
        }
        return ret;
    }
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
    void set_cb(WORKER_CB_FUNC cb)
    {
        if (cb)
        {
            cb_ = cb;
        }
        else
        {
            logger->error(ZMQ_LOG, "\[WORKER\] invalid callback function\n");
        }
    }

    size_t send(const char *msg, size_t len, void *ID)
    {
        auto iter = Id2MsgMap.find(ID);
        if (iter != Id2MsgMap.end())
        {
            zmsg::ustring tmp_ustr((unsigned char *)msg, len);
            (iter->second)->push_back(tmp_ustr);
            {
                std::lock_guard<M_MUTEX> glock(worker_mutex);
                //note
                worker_q.emplace(iter->second);
                //worker_q.push(iter->second);
            }
            Id2MsgMap.erase(iter);
        }
        else
        {
            logger->error(ZMQ_LOG, "\[WORKER\] did not find the ID\n");
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
            logger->error(ZMQ_LOG, "\[WORKER\] invalid montior callback function \n");
        }
    }

    void *getUniqueID() { return (void *)(uniqueID_atomic++); };

  private:
    bool monitor_task()
    {
        void *WORKER_mon = zmq_socket(worker_socket_->ctxptr, ZMQ_PAIR);
        if (!WORKER_mon)
        {
            logger->error(ZMQ_LOG, "\[WORKER\] 0MQ get socket fail\n");
            return false;
        }
        try
        {
            logger->debug(ZMQ_LOG, "\[WORKER\] monitor path %s\n", monitor_path.c_str());
            int rc = zmq_connect(WORKER_mon, monitor_path.c_str());
            //rc should be 0 if success
            if (rc)
            {
                logger->error(ZMQ_LOG, "\[WORKER\] 0MQ connect fail\n");
                return false;
            }
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, "\[WORKER\] connect to monitor worker socket fail\n");
            return false;
        }

        while (1)
        {
            if (should_exit_monitor_task)
            {
                zmq_close(WORKER_mon);

                logger->warn(ZMQ_LOG, "\[WORKER\] will exit worker monitor task\n");
                return true;
            }
            std::string address;
            int value;
            int event = get_monitor_event(WORKER_mon, &value, address);
            if (event == -1)
            {
                logger->warn(ZMQ_LOG, "\[WORKER\] get monitor event fail\n");
                //return false;
            }
            if (monitor_cb)
            {
                monitor_cb(event, value, address);
            }
        }
    }
    bool monitor_this_socket()
    {
        int rc = zmq_socket_monitor(worker_socket_->ptr, monitor_path.c_str(), ZMQ_EVENT_ALL);
        return ((rc == 0) ? true : false);
    }
    size_t send(zmsg &input)
    {
        input.send(*worker_socket_);
    }
    size_t send(const char *msg, size_t len)
    {
        worker_socket_->send(msg, len);
    }
    // ph1 mainly set the connection option and connect to the borker.
    bool start_ph1()
    {
        try
        {
            // enable IPV6, we had already make sure that we are using TCP then we can set this option
            int enable_v6 = 1;
            worker_socket_->setsockopt(ZMQ_IPV6, &enable_v6, sizeof(enable_v6));
            /*Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.*/
            int iRcvSendTimeout = 5000; // millsecond Make it configurable
            worker_socket_->setsockopt(ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
            worker_socket_->setsockopt(ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));

            int linger = 0;
            //worker_socket_->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
            identity_ = s_set_id(*worker_socket_);

            logger->debug(ZMQ_LOG, "\[WORKER\] worker id set to %s\n", identity_.c_str());
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, "\[CLIENT\] set socket option return fail\n");
            return false;
        }
        try
        {
            std::string IPPort;
            // should be like this tcp://192.168.1.17:5555;192.168.1.1:5555
            if (IP_and_port_source.empty())
            {
                IPPort += protocol + IP_and_port_dest;
            }
            else
            {
                IPPort += protocol + IP_and_port_source + ";" + IP_and_port_dest;
            }
            logger->debug(ZMQ_LOG, "\[WORKER\] connect to : %s\n", IPPort.c_str());
            worker_socket_->connect(IPPort);
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
        zmq::socket_t *tmp_worker = worker_socket_;
        //  Send out heartbeats at regular intervals
        int64_t heartbeat_at = s_clock() + HEARTBEAT_INTERVAL;
        size_t liveness = HEARTBEAT_LIVENESS;
        size_t interval = INTERVAL_INIT;
        //  Initialize poll set
        while (1)
        {
            zmq::pollitem_t items[] = {
                {*tmp_worker, 0, ZMQ_POLLIN, 0}};
            if (should_exit_routine_task)
            {
                try
                {
                    int linger = 0;
                    tmp_worker->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
                }
                catch (std::exception &e)
                {
                    logger->error(ZMQ_LOG, "\[WORKER\] set ZMQ_LINGER return fail\n");
                }

                tmp_worker->close();
                ctx_->close();

                logger->warn(ZMQ_LOG, "\[WORKER\] worker will exit \n");
                return true;
            }
            try
            {
                // by default we wait for 1000ms then so something. like hreatbeat
                zmq::poll(items, 1, HEARTBEAT_INTERVAL);
                if (should_exit_routine_task)
                {
                    try
                    {
                        int linger = 0;
                        tmp_worker->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
                    }
                    catch (std::exception &e)
                    {
                        logger->error(ZMQ_LOG, "\[WORKER\] set ZMQ_LINGER return fail\n");
                    }

                    tmp_worker->close();
                    ctx_->close();

                    logger->warn(ZMQ_LOG, "\[WORKER\] worker will exit \n");
                    return true;
                }
                if (items[0].revents & ZMQ_POLLIN)
                {
                    zmsg_ptr msg(new zmsg(*worker_socket_)); //, [&msg] { msg.reset(); });
                    logger->debug(ZMQ_LOG, "\[WORKER\] get message from broker with %d part", msg->parts());
                    //msg->dump();
                    // now we get the message .
                    // this is the normal message
                    if (msg->parts() == 3)
                    {
                        std::string data = msg->get_body();
                        if (data.empty())
                        {
                            logger->warn(ZMQ_LOG, "\[WORKER\] we get a message without body\n");
                            continue;
                        }
                        void *ID = getUniqueID();
                        Id2MsgMap.emplace(ID, msg);
                        //std::cout << "receive message form client" << std::endl;
                        //msg.dump();
                        if (cb_)
                        {
                            cb_(data.c_str(), data.size(), ID);
                        }
                        else
                        {
                            logger->error(ZMQ_LOG, "\[WORKER\]  no valid callback function, please make sure you had set message callback fucntion\n");
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
                            logger->warn(ZMQ_LOG, "\[WORKER\] invalid message, %s\n", identity_.c_str());
                            msg->dump();
                        }
                    }
                    interval = INTERVAL_INIT;

                    if (worker_q.size())
                    {
                        try
                        {
                            std::lock_guard<M_MUTEX> glock(worker_mutex);
                            logger->debug(ZMQ_LOG, "\[WORKER\] there is %d message, now send message\n", worker_q.size());
                            // check size again under the lock
                            while (worker_q.size())
                            {
                                (worker_q.front())->send(*worker_socket_);
                                // make sure the the reference count is 0
                                // note : delete the following code ???????
                                (worker_q.front()).reset();
                                worker_q.pop();
                            }
                        }
                        catch (std::exception &e)
                        {
                            logger->error(ZMQ_LOG, "\[WORKER\] send message fail!\n");
                            continue;
                        }
                    }
                }
                // epoll time out
                else if (--liveness == 0)
                {
                    logger->warn(ZMQ_LOG, "\[WORKER\]  heartbeat failure, can't reach queue, identity : (%s) \n", identity_.c_str());
                    logger->warn(ZMQ_LOG, "\[WORKER\]  reconnecting in  %d msec..., identity : (%s)", interval, identity_.c_str());
                    s_sleep(interval);
                    if (interval < INTERVAL_MAX)
                    {
                        interval *= 2;
                    }
                    // stop the worker and start again
                    try
                    {
                        worker_socket_->close();
                        tmp_worker = new zmq::socket_t(*ctx_, ZMQ_DEALER);
                        //worker = s_worker_socket(context);
                        worker_socket_ = tmp_worker;
                        if (!start_ph1())
                        {
                            logger->warn(ZMQ_LOG, "\[WORKER\]  connect to broker return fail!\n");
                            continue;
                        }
                    }
                    catch (std::exception &e)
                    {
                        logger->error(ZMQ_LOG, "\[WORKER\]  restart worker socket fail!\n");
                    }
                    liveness = HEARTBEAT_LIVENESS;
                }
                //  Send heartbeat to queue if it's time
                if (s_clock() > heartbeat_at)
                {
                    heartbeat_at = s_clock() + HEARTBEAT_INTERVAL;
                    logger->debug(ZMQ_LOG, "\[WORKER\]  (%s) worker heartbeat\n", identity_.c_str());
                    s_send(*worker_socket_, "HEARTBEAT");
                }
                if (worker_q.size())
                {
                    try
                    {
                        std::lock_guard<M_MUTEX> glock(worker_mutex);
                        logger->debug(ZMQ_LOG, "\[WORKER\] there is %d message, now send message\n", worker_q.size());
                        // check size again under the lock
                        while (worker_q.size())
                        {
                            (worker_q.front())->send(*worker_socket_);
                            // make sure the the reference count is 0
                            (worker_q.front()).reset();
                            worker_q.pop();
                        }
                    }
                    catch (std::exception &e)
                    {
                        logger->error(ZMQ_LOG, "\[WORKER\] send message fail!\n");
                        continue;
                    }
                }
            }
            catch (std::exception &e)
            {
                logger->error(ZMQ_LOG, "\[WORKER\]  something wrong while polling message\n");
            }
        }
    }

  private:
    std::string identity_;
    std::string monitor_path;
    WORKER_CB_FUNC *cb_;
    MONITOR_CB_FUNC *monitor_cb;
    std::string IP_and_port_dest;
    std::string protocol;
    std::string IP_and_port_source;
    zmq::context_t *ctx_;
    zmq::socket_t *worker_socket_;

    std::atomic<long> uniqueID_atomic;
    std::map<void *, zmsg_ptr> Id2MsgMap;
    //std::condition_variable monitor_cond;
    //std::mutex monitor_mutex;
    M_MUTEX worker_mutex;
    std::thread *routine_thread;
    std::thread *monitor_thread;
    bool should_exit_monitor_task;
    bool should_exit_routine_task;
    std::queue<zmsg_ptr> worker_q;
};
