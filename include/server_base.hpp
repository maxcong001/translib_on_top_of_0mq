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

class server_base
{

  public:
    server_base()
        : ctx_(1),
          server_socket_(ctx_, ZMQ_ROUTER), uniqueID_atomic(1)
    {
        routine_thread = NULL;
        monitor_thread = NULL;
        should_exit_monitor_task = false;
        should_exit_routine_task = false;
    }
    ~server_base()
    {
#if 0
        if (server_socket_)
        {
            delete server_socket_;
        }
        if (ctx_)
        {
            delete ctx_;
        }
#endif
        should_exit_monitor_task = false;
        should_exit_routine_task = false;
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
            // log here, start monitor socket fail!
            return false;
        }
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
            //log here
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
            // log here, did not find the ID
            return -1;
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
            // log here
            return false;
        }
        int rc = zmq_connect(server_mon, "inproc://monitor-server");

        //rc should be 0 if success
        if (rc)
        {
            //
            return false;
        }
        while (1)
        {
            if (should_exit_monitor_task)
            {
                return true;
            }
            std::string address;
            int value;
            int event = get_monitor_event(server_mon, &value, address);
            if (event == -1)
            {
                return false;
            }
            std::cout << "receive event form server monitor task, the event is " << event << ". Value is : " << value << ". string is : " << address << std::endl;
        }
    }
    bool monitor_this_socket()
    {
        int rc = zmq_socket_monitor(server_socket_, "inproc://monitor-server", ZMQ_EVENT_ALL);
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
            return false;
        }
        if (zmq_setsockopt(server_socket_, ZMQ_SNDTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }

        int linger = 0;
        if (zmq_setsockopt(server_socket_, ZMQ_LINGER, &linger, sizeof(linger)) < 0)
        {
            zmq_close(server_socket_);
            zmq_ctx_destroy(&ctx_);
            return false;
        }
        try
        {
            if (IP_and_port.empty())
            {
                return false;
            }
            std::string tmp;
            tmp += "tcp://" + IP_and_port;
            server_socket_.bind(tmp);
        }
        catch (std::exception &e)
        {
            // need log here
            return false;
        }

        //  Initialize poll set
        zmq::pollitem_t items[] = {
            {server_socket_, 0, ZMQ_POLLIN, 0}};
        while (1)
        {
            if (should_exit_routine_task)
            {
                return true;
            }
            try
            {
                // by default we wait for 500ms then so something. like hreatbeat
                zmq::poll(items, 1, EPOLL_TIMEOUT);
                if (items[0].revents & ZMQ_POLLIN)
                {
                    // this is for test, delete it later
                    //sleep(5);

                    zmsg_ptr msg(new zmsg(server_socket_));
                    std::string data = msg->get_body();
                    if (data.empty())
                    {
                        // log here, we get a message without body
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
                        //log here as there is no callback function
                    }
                }
                else
                {
                    std::cout << " epoll timeout !" << std::endl;
                    // to do, signal the monitor thread
                }
            }
            catch (std::exception &e)
            {
                std::unique_lock<std::mutex> monitor_lock(monitor_mutex);
                monitor_cond.notify_all();
                //return false;
            }
        }
    }

  private:
    // this is for test
    //int seq_num;
    SERVER_CB_FUNC *cb_;
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