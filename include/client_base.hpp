#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <string>
#include <unordered_set>
#include <queue>
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

        cb_ = NULL;
        routine_thread = NULL;
        monitor_thread = NULL;
        should_stop = false;
        should_exit_monitor_task = false;
        protocol = "tcp://";
        IP_and_port_dest = "127.0.0.1:5561";
    }
    client_base(std::string IPPort) : ctx_(1),
                                      client_socket_(ctx_, ZMQ_DEALER)
    {
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
        cb_ = NULL;
        routine_thread = NULL;
        monitor_thread = NULL;
        protocol = "tcp://";
        IP_and_port_dest = IPPort;
        should_stop = false;
        should_exit_monitor_task = false;

        run();
    }

    ~client_base()
    {
        stop();
    }
    void set_protocol(std::string protocol_)
    {
        protocol = protocol_;
    }
    std::string get_protocol()
    {
        return protocol;
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
        tmp_str += tmp_msg;

        sand_box.emplace((void *)cb);

        //zmsg messsag;

        zmsg_ptr messsag(new zmsg((char *)tmp_str.c_str()));
        messsag->push_back(tmp_str);
        //messsag->dump();
        // this is for test
        //logger->error(ZMQ_LOG, "\[CLIENT\] !messsag.part is %d\n", messsag->parts());
        // send message to the queue
        {
            std::lock_guard<M_MUTEX> glock(client_mutex);
            queue_s.emplace(messsag);
        }

        // note: return len here
        return len;
    }

    size_t send(void *usr_data, USR_CB_FUNC cb, char *msg, size_t len)
    {
        usrdata_and_cb tmp_struct;
        tmp_struct.usr_data = usr_data;
        tmp_struct.cb = (void *)cb;
        zmsg::ustring tmp_str((unsigned char *)&tmp_struct, sizeof(usrdata_and_cb));
        zmsg::ustring tmp_msg((unsigned char *)(msg), len);
        tmp_str += tmp_msg;

        sand_box.emplace((void *)cb);

        //zmsg messsag;
        zmsg_ptr messsag(new zmsg((char *)tmp_str.c_str()));
        messsag->push_back(tmp_str);
        // this is for test
        logger->error(ZMQ_LOG, "\[CLIENT\] !messsag.part is %d\n", messsag->parts());
        // send message to the queue
        {
            std::lock_guard<M_MUTEX> glock(client_mutex);
            queue_s.emplace(messsag);
        }

        // note: return len here
        return len;
    }
    bool run()
    {
        auto routine_fun = std::bind(&client_base::start, this);
        routine_thread = new std::thread(routine_fun);
        routine_thread->detach();
        auto monitor_fun = std::bind(&client_base::monitor_task, this);
        monitor_thread = new std::thread(monitor_fun);
        monitor_thread->detach();

        bool ret = monitor_this_socket();
        if (ret)
        {
            logger->debug(ZMQ_LOG, "\[CLIENT\] start monitor socket success!\n");
        }
        else
        {
            logger->error(ZMQ_LOG, "\[CLIENT\] start monitor socket fail!\n");
            return false;
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
            logger->error(ZMQ_LOG, "\[CLIENT\] Invalid callback function\n");
        }
    }

    bool stop()
    {
        should_exit_monitor_task = true;
        if (monitor_thread)
        {
            //monitor_thread->join();
        }
        // let the routine thread exit
        should_stop = true;
        if (routine_thread)
        {
            //routine_thread->join();
        }
        // to do, just return true

        return true;
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
    bool monitor_task()
    {
        void *client_mon = zmq_socket((void *)ctx_, ZMQ_PAIR);
        if (!client_mon)
        {
            logger->error(ZMQ_LOG, "\[CLIENT\] start PAIR socket fail!\n");
            return false;
        }

        try
        {
            int rc = zmq_connect(client_mon, monitor_path.c_str());

            //rc should be 0 if success
            if (rc)
            {

                logger->error(ZMQ_LOG, "\[CLIENT\] connect to nomitor pair fail!\n");
                return false;
            }
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, "\[CLIENT\] connect to monitor socket fail\n");
            return false;
        }
        while (1)
        {
            if (should_exit_monitor_task)
            {
                zmq_close(client_mon);
                logger->warn(ZMQ_LOG, "\[CLIENT\] will exit monitor task\n");
                return true;
            }
            std::string address;
            int value;

            int event = get_monitor_event(client_mon, &value, address);
            if (event == -1)
            {
                logger->warn(ZMQ_LOG, "\[CLIENT\] get monitor event fail\n");
                //return false;
            }
            logger->debug(ZMQ_LOG, "\[CLIENT\] receive event form client monitor task, the event is %d. Value is :%d. String is %s\n", event, value, address.c_str());

            if (monitor_cb)
            {
                monitor_cb(event, value, address);
            }
        }
    }
    bool monitor_this_socket()
    {
        int rc = zmq_socket_monitor(client_socket_, monitor_path.c_str(), ZMQ_EVENT_ALL);
        return ((rc == 0) ? true : false);
    }

    bool start()
    {

        try
        {
            // enable IPV6, we had already make sure that we are using TCP then we can set this option
            int enable_v6 = 1;
            client_socket_.setsockopt(ZMQ_IPV6, &enable_v6, sizeof(enable_v6));
            /*Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.*/
            int iRcvSendTimeout = 5000; // millsecond Make it configurable
            client_socket_.setsockopt(ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
            client_socket_.setsockopt(ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
            int linger = 0;
            //client_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
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
            logger->debug(ZMQ_LOG, "\[CLIENT\] connect to : %s\n", IPPort.c_str());
            client_socket_.connect(IPPort);
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, "\[CLIENT\] connect return fail\n");
            return false;
        }

        //  Initialize poll set
        zmq::pollitem_t items[] = {{client_socket_, 0, ZMQ_POLLIN, 0}};
        while (1)
        {
            if (should_stop)
            {
                logger->warn(ZMQ_LOG, "\[CLIENT\] client thread will exit !");
                /*
                try
                {
                    int linger = 0;
                    client_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
                }
                catch (std::exception &e)
                {
                    logger->error(ZMQ_LOG, "\[CLIENT\] set ZMQ_LINGER return fail\n");
                }
                */
                //client_socket_.close();
                //logger->warn(ZMQ_LOG, "\[CLIENT\] client_socket_.close() success\n");

                //ctx_.close();
                //logger->warn(ZMQ_LOG, "\[CLIENT\] ctx_.close() success\n");
                return true;
            }
            try
            {
                zmq::poll(items, 1, EPOLL_TIMEOUT);
                if (items[0].revents & ZMQ_POLLIN)
                {
                    std::string tmp_str;
                    //std::string tmp_data_and_cb;
                    {

                        zmsg msg(client_socket_);
                        logger->debug(ZMQ_LOG, "\[CLIENT\] rceive message with %d parts\n", msg.parts());
                        tmp_str = msg.get_body();
                    }
                    usrdata_and_cb *usrdata_and_cb_p = (usrdata_and_cb *)(tmp_str.c_str());

                    void *user_data = usrdata_and_cb_p->usr_data;

                    if (sand_box.find((void *)(usrdata_and_cb_p->cb)) == sand_box.end())
                    {
                        logger->warn(ZMQ_LOG, "\[CLIENT\] Warning! the message is crrupted or someone is hacking us !!");
                        continue;
                    }
                    cb_ = (USR_CB_FUNC *)(usrdata_and_cb_p->cb);

                    if (cb_)
                    {
                        cb_((char *)((usrdata_and_cb *)(tmp_str.c_str()) + 1), tmp_str.size() - sizeof(usrdata_and_cb), user_data);
                    }
                    else
                    {
                        logger->error(ZMQ_LOG, "\[CLIENT\] no callback function is set!\n");
                    }
                    // why we check if there is message and send it?
                    // because if the message keep coming. there will be no change to send message/
                    // why two  queue_s.size()
                    // avoid add lock. only queue_s.size() is not 0, then we add lock.
                    // to do: how many message to send? or send all the message(we are async, that is fine)

                    if (queue_s.size())
                    {
                        try
                        {
                            std::lock_guard<M_MUTEX> glock(client_mutex);
                            logger->debug(ZMQ_LOG, "\[CLIENT\] there is %d message to send\n", queue_s.size());
                            // check size again under the lock
                            while (queue_s.size())
                            {
                                (queue_s.front())->send(client_socket_);
                                (queue_s.front()).reset();
                                queue_s.pop();
                            }
                        }
                        catch (std::exception &e)
                        {
                            logger->error(ZMQ_LOG, "\[CLIENT\] send message fail!\n");
                            continue;
                        }
                    }
                }
                else
                // poll timeout, now it is the time we send message.
                {
                    if (queue_s.size())
                    {
                        try
                        {
                            std::lock_guard<M_MUTEX> glock(client_mutex);
                            logger->debug(ZMQ_LOG, "\[CLIENT\] poll timeout, and there is %d message, now send message\n", queue_s.size());
                            // check size again under the lock
                            while (queue_s.size())
                            {
                                (queue_s.front())->send(client_socket_);
                                (queue_s.front()).reset();
                                queue_s.pop();
                            }
                        }
                        catch (std::exception &e)
                        {
                            logger->error(ZMQ_LOG, "\[CLIENT\] send message fail!\n");
                            continue;
                        }
                    }
                }
            }
            catch (std::exception &e)
            {
                logger->error(ZMQ_LOG, "\[CLIENT\] exception is catched for 0MQ epoll");
            }
        }
        // should never run here
        return false;
    }

  private:
    std::string monitor_path;
    std::string IP_and_port_dest;
    std::string IP_and_port_source;
    std::string protocol;
    std::unordered_set<void *> sand_box;
    USR_CB_FUNC *cb_;
    zmq::context_t ctx_;
    zmq::socket_t client_socket_;
    std::thread *routine_thread;
    std::thread *monitor_thread;

    bool should_stop;
    bool should_exit_monitor_task;
    MONITOR_CB_FUNC_CLIENT monitor_cb;
    M_MUTEX client_mutex;
    // queue the message to send
    std::queue<zmsg_ptr> queue_s;
};
