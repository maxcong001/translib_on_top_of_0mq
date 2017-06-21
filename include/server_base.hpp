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

class server_base
{

  public:
    server_base()
        : ctx_(1),
          server_socket_(ctx_, ZMQ_ROUTER), uniqueID_atomic(1)
    {
        monitor_cb = NULL;
        routine_thread = NULL;
        monitor_thread = NULL;
        should_exit_monitor_task = false;
        should_exit_routine_task = false;
        monitor_path.clear();
        // set random monitor path
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
    ~server_base()
    {
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
        auto routine_fun = std::bind(&server_base::start, this);
        routine_thread = new std::thread(routine_fun);
        //routine_thread->detach();
        auto monitor_fun = std::bind(&server_base::monitor_task, this);
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
        return ret;
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
            (iter->second)->send(server_socket_);
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
        return len;
    }

    void set_monitor_cb(MONITOR_CB_FUNC cb)
    {
        if (cb)
        {
            monitor_cb = cb;
        }
        else
        {
            logger->error(ZMQ_LOG, "invalid callback fucntion\n");
        }
    }

    void *getUniqueID() { return (void *)(uniqueID_atomic++); };

  private:
#if 0
    void epoll_task()
    {

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
                    auto routine_fun = std::bind(&server_base::start, this);
                    std::thread routine_thread(routine_fun);
                    routine_thread.detach();
                }
            }
        }

    }
#endif
    bool monitor_task()
    {
        void *server_mon = zmq_socket((void *)ctx_, ZMQ_PAIR);
        if (!server_mon)
        {
            logger->error(ZMQ_LOG, "get 0MQ socket fail\n");
            return false;
        }
        int rc = zmq_connect(server_mon, monitor_path.c_str());

        //rc should be 0 if success
        if (rc)
        {
            logger->error(ZMQ_LOG, "connect fail\n");
            return false;
        }
        while (1)
        {
            if (should_exit_monitor_task)
            {
                logger->warn(ZMQ_LOG, "monitor task will exit\n");
                return true;
            }
            std::string address;
            int value;
            int event = get_monitor_event(server_mon, &value, address);
            if (event == -1)
            {
                logger->error(ZMQ_LOG, "get monitor event fail\n");
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
        int rc = zmq_socket_monitor(server_socket_, monitor_path.c_str(), ZMQ_EVENT_ALL);
        return ((rc == 0) ? true : false);
    }
    size_t send(zmsg &input)
    {
        input.send(server_socket_);
    }
    size_t send(const char *msg, size_t len)
    {
        server_socket_.send(msg, len);
    }
    bool start()
    {
        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        int enable_v6 = 1;
        if (zmq_setsockopt(server_socket_, ZMQ_IPV6, &enable_v6, sizeof(enable_v6)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            logger->error(ZMQ_LOG, "set socket optioon ZMQ_IPV6 fail\n");
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
            logger->error(ZMQ_LOG, "set socket optioon ZMQ_RCVTIMEO fail\n");
            return false;
        }
        if (zmq_setsockopt(server_socket_, ZMQ_SNDTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            logger->error(ZMQ_LOG, "set socket optioon ZMQ_SNDTIMEO fail\n");

            return false;
        }

        int linger = 0;
        if (zmq_setsockopt(server_socket_, ZMQ_LINGER, &linger, sizeof(linger)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            logger->error(ZMQ_LOG, "set socket optioon ZMQ_LINGER fail\n");

            return false;
        }
        try
        {
            if (IP_and_port.empty())
            {
                logger->error(ZMQ_LOG, "please make sure you had set the IP and port info\n");
                return false;
            }
            std::string tmp;
            tmp += "tcp://" + IP_and_port;
            server_socket_.bind(tmp);
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, " socket bind error\n");
            return false;
        }

        //  Initialize poll set
        zmq::pollitem_t items[] = {
            {server_socket_, 0, ZMQ_POLLIN, 0}};
        while (1)
        {
            if (should_exit_routine_task)
            {
                logger->warn(ZMQ_LOG, " server routine task will exit\n");
                return true;
            }
            try
            {
                // by default we wait for 500ms then so something. like hreatbeat
                zmq::poll(items, 1, -1);
                if (items[0].revents & ZMQ_POLLIN)
                {
                    // this is for test, delete it later
                    //sleep(5);

                    zmsg_ptr msg(new zmsg(server_socket_));
                    std::string data = msg->get_body();
                    if (data.empty())
                    {
                        logger->warn(ZMQ_LOG, " get a message without body\n");
                        continue;
                    }
                    void *ID = getUniqueID();
                    Id2MsgMap.emplace(ID, msg);

                    // ToDo: now we got the message, do main work
                    //std::cout << "receive message form client" << std::endl;
                    //msg.dump();
                    // send back message to client, for test
                    //msg.send(server_socket_);
                    if (cb_)
                    {
                        cb_(data.c_str(), data.size(), ID);
                    }
                    else
                    {
                        logger->warn(ZMQ_LOG, " no invalid callback function, please make sure you had set it\n");
                    }
                }
                else
                {
                    logger->error(ZMQ_LOG, "epoll error !\n");
                }
            }
            catch (std::exception &e)
            {
                logger->error(ZMQ_LOG, "catch exception. epoll error\n");
            }
        }
        // should never return
        return false;
    }

  private:
    std::string monitor_path;
    SERVER_CB_FUNC *cb_;
    MONITOR_CB_FUNC *monitor_cb;
    std::string IP_and_port;
    zmq::context_t ctx_;
    zmq::socket_t server_socket_;
    std::atomic<long> uniqueID_atomic;
    std::map<void *, zmsg_ptr> Id2MsgMap;

    std::condition_variable monitor_cond;
    std::mutex monitor_mutex;

    std::thread *routine_thread;
    std::thread *monitor_thread;
    bool should_exit_monitor_task;
    bool should_exit_routine_task;
};