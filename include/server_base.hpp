#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>
#include <map>

#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"
#include <memory>
#include <util.hpp>

class server_base
{

  public:
    server_base()
        : ctx_(1),
          server_socket_(ctx_, ZMQ_ROUTER), uniqueID_atomic(1)
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
                return true;
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
            try
            {
                zmq::poll(items, 1, -1);
                if (items[0].revents & ZMQ_POLLIN)
                {
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
            }
            catch (std::exception &e)
            {
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
};