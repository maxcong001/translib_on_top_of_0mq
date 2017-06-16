#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <string>
#include <unordered_set>
#include <zmq.hpp>
#include "zhelpers.hpp"
#include "zmsg.hpp"
#include "util.hpp"

class client_base
{
  public:
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
    struct usrdata_and_cb
    {
        void *usr_data;
        void *cb;
    };
    size_t send(void *usr_data, USR_CB_FUNC cb, const char *msg, size_t len)
    {
        usrdata_and_cb tmp_struct;
        tmp_struct.usr_data = usr_data;
        tmp_struct.cb = (void *)cb;
        zmsg::ustring tmp_str((unsigned char *)&tmp_struct, sizeof(usrdata_and_cb));
        zmsg::ustring tmp_msg((unsigned char *)(msg), len);

        sand_box.emplace((void *)cb);

        zmsg messsag;
        messsag.push_back(tmp_str);
        messsag.push_back(tmp_msg);

        try
        {
            messsag.send(client_socket_);
        }
        catch (std::exception &e)
        {
            // log here, send fail
            return -1;
        }
    }

    size_t send(void *usr_data, USR_CB_FUNC cb, char *msg, size_t len)
    {
        usrdata_and_cb tmp_struct;
        tmp_struct.usr_data = usr_data;
        tmp_struct.cb = (void *)cb;
        zmsg::ustring tmp_str((unsigned char *)&tmp_struct, sizeof(usrdata_and_cb));
        zmsg::ustring tmp_msg((unsigned char *)(msg), len);

        sand_box.emplace((void *)cb);

        zmsg messsag;
        messsag.push_back(tmp_str);
        messsag.push_back(tmp_msg);

        try
        {
            messsag.send(client_socket_);
        }
        catch (std::exception &e)
        {
            // log here, send fail
            return -1;
        }
    }
    void run()
    {
        auto routine_fun = std::bind(&client_base::start, this);
        std::thread routine_thread(routine_fun);
        routine_thread.detach();
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
    /*
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
    */
    // restart with new IP and port
    bool restart(std::string input)
    {
        /*
        if (IP_and_port_dest == input)
        {
            return true;
        }
        client_socket_.close();

        IP_and_port_dest = input;*/
    }

  private:
    size_t
    send(const char *msg, size_t len)
    {
        return client_socket_.send(msg, len);
    }
    size_t send(char *msg, size_t len)
    {
        return client_socket_.send(msg, len);
    }

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

            client_socket_.connect(IPPort);
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
                    if (msg.parts() != 2)
                    {
                        // log here, the received message should have two parts.
                        std::cout << "Maxx receive message have " << msg.parts() << "parts" << std::endl;
                    }
                    std::string tmp_str = msg.get_body();
                    std::string tmp_data_and_cb = msg.get_body();
                    usrdata_and_cb *usrdata_and_cb_p = (usrdata_and_cb *)(tmp_data_and_cb.c_str());

                    void *user_data = usrdata_and_cb_p->usr_data;
                    if (sand_box.find((void *)(usrdata_and_cb_p->cb)) == sand_box.end())
                    {
                        std::cout << "Warning! the message is crrupted or someone is hacking us !!" << std::endl;
                        continue;
                    }
                    cb_ = (USR_CB_FUNC *)(usrdata_and_cb_p->cb);

                    //std::cout << "receive message from server with " << msg.parts() << " parts" << std::endl;
                    //msg.dump();
                    // ToDo: now we got the message, do main work
                    // Note: we should not do heavy work in this thread!!!!
                    //std::cout << "receive message form server, body is " << msg.body() << std::endl;
                    if (cb_)
                    {
                        cb_(tmp_str.c_str(), tmp_str.size(), user_data);
                    }
                    else
                    {
                        // log here , no callback function
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
    std::string IP_and_port_dest;
    std::string IP_and_port_source;
    std::unordered_set<void *> sand_box;
    USR_CB_FUNC *cb_;
    zmq::context_t ctx_;
    zmq::socket_t client_socket_;
};