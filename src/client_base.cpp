#include "client_base.hpp"

std::unordered_set<void *> sand_box_client;
client_base::client_base()
{

    client_socket_ = NULL;
    ctx_ = NULL;

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

    routine_thread = NULL;
    monitor_thread = NULL;
    should_stop = false;
    should_exit_monitor_task = false;
    protocol = "tcp://";
    IP_and_port_dest = "127.0.0.1:5561";
}
bool client_base::run()
{
    std::shared_ptr<std::mutex> tmp_mutex(new std::mutex());
    client_mutex = tmp_mutex;

    std::shared_ptr<std::queue<zmsg_ptr>> queue_s_client_tmp(new std::queue<zmsg_ptr>());
    queue_s_client = queue_s_client_tmp;

    ctx_ = new zmq::context_t(1);
    if (!ctx_)
    {
        logger->error(ZMQ_LOG, "\[CLIENT\] new context fail!\n");
        return false;
    }
    client_socket_ = new zmq::socket_t(*ctx_, ZMQ_DEALER);
    if (!client_socket_)
    {
        logger->error(ZMQ_LOG, "\[CLIENT\] new socket fail!\n");
        return false;
    }

    auto routine_fun = std::bind(&client_base::start, this);
    routine_thread = new std::thread(routine_fun);
    //routine_thread->detach();
    auto monitor_fun = std::bind(&client_base::monitor_task, this);
    monitor_thread = new std::thread(monitor_fun);
    //monitor_thread->detach();
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
    return ret;
}
// if the return value is 0, that means send fail and will not resend. you should send again
size_t client_base::send(void *usr_data, USR_CB_FUNC cb, char *msg, size_t len)
{
    usrdata_and_cb tmp_struct;
    tmp_struct.usr_data = usr_data;
    tmp_struct.cb = (void *)cb;
    zmsg::ustring tmp_str((unsigned char *)&tmp_struct, sizeof(usrdata_and_cb));
    zmsg_ptr messsag(new zmsg(tmp_str));
    zmsg::ustring tmp_msg((unsigned char *)(msg), len);
    messsag->push_back(tmp_msg);

    sand_box_client.emplace((void *)cb);

    // send message to the queue
    {
        if (client_mutex)
        {
            std::lock_guard<M_MUTEX> glock(*client_mutex);
        }
        else
        {
            logger->error(ZMQ_LOG, "\[CLIENT\] client_mutex is invalid!\n");
            return 0;
        }

        queue_s_client->emplace(messsag);
        //logger->debug(ZMQ_LOG, "\[CLIENT\] size of queue is %d!\n", queue_s_client->size());
    }

    // note: return len here
    return len;
}

bool client_base::stop()
{
    should_exit_monitor_task = true;
    should_stop = true;

    if (monitor_thread)
    {

        monitor_thread->join();
    }
    if (routine_thread)
    {

        routine_thread->join();
    }

    return true;
}
bool client_base::start()
{
    zmq::socket_t *tmp_socket = client_socket_;
    zmq::context_t *tmp_ctx = ctx_;

    std::shared_ptr<std::mutex> tmp_client_mutex = client_mutex;
    std::shared_ptr<std::queue<zmsg_ptr>> tmp_queue_s_client = queue_s_client;

    try
    {
        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        int enable_v6 = 1;
        tmp_socket->setsockopt(ZMQ_IPV6, &enable_v6, sizeof(enable_v6));
        /*Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.*/
        int iRcvSendTimeout = 5000; // millsecond Make it configurable
        tmp_socket->setsockopt(ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
        tmp_socket->setsockopt(ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
        //int linger = 0;
        //tmp_socket->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
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
        tmp_socket->connect(IPPort);
    }
    catch (std::exception &e)
    {
        logger->error(ZMQ_LOG, "\[CLIENT\] connect return fail\n");
        return false;
    }

    //  Initialize poll set
    zmq::pollitem_t items[] = {{*tmp_socket, 0, ZMQ_POLLIN, 0}};
    while (1)
    {
        if (should_stop)
        {
            logger->warn(ZMQ_LOG, "\[CLIENT\] client thread will exit !");
            try
            {
                int linger = 0;
                tmp_socket->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
            }
            catch (std::exception &e)
            {
                logger->error(ZMQ_LOG, "\[CLIENT\] set ZMQ_LINGER return fail\n");
            }
            tmp_socket->close();
            tmp_ctx->close();

            return true;
        }
        try
        {
            zmq::poll(items, 1, EPOLL_TIMEOUT);
            // note this is a work around
            // will find a new way to fix the multi-thread issue
            if (should_stop)
            {
                logger->warn(ZMQ_LOG, "\[CLIENT\] client thread will exit !");
                try
                {
                    int linger = 0;
                    tmp_socket->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
                }
                catch (std::exception &e)
                {
                    logger->error(ZMQ_LOG, "\[CLIENT\] set ZMQ_LINGER return fail\n");
                }
                tmp_socket->close();
                tmp_ctx->close();
                return true;
            }
            if (items[0].revents & ZMQ_POLLIN)
            {
                std::string tmp_str;
                std::string payload;
                //std::string tmp_data_and_cb;
                {
                    zmsg msg(*tmp_socket);
                    if (msg.parts() < 2)
                    {
                        logger->error(ZMQ_LOG, "\[CLIENT\] receive message with %d parts, less than 2 parts!\n", msg.parts());
                        continue;
                    }
                    logger->debug(ZMQ_LOG, "\[CLIENT\] rceive message with %d parts\n", msg.parts());
                    payload = msg.get_body();
                    tmp_str = msg.get_body();
                    
                }
                usrdata_and_cb *usrdata_and_cb_p = (usrdata_and_cb *)(tmp_str.c_str());

                void *user_data = usrdata_and_cb_p->usr_data;

                if (sand_box_client.find((void *)(usrdata_and_cb_p->cb)) == sand_box_client.end())
                {
                    logger->warn(ZMQ_LOG, "\[CLIENT\] Warning! the message is crrupted or someone is hacking us !!");
                    continue;
                }
                USR_CB_FUNC *tmp_cb = (USR_CB_FUNC *)(usrdata_and_cb_p->cb);

                if (tmp_cb)
                {
                    tmp_cb((char *)(payload.c_str()), payload.size(), user_data);
                }
                else
                {
                    logger->error(ZMQ_LOG, "\[CLIENT\] no callback function is set!\n");
                }
                // why we check if there is message and send it?
                // because if the message keep coming. there will be no change to send message/
                // why two  queue_s_client->size()
                // avoid add lock. only queue_s_client->size() is not 0, then we add lock.
                // to do: how many message to send? or send all the message(we are async, that is fine)
                if (tmp_queue_s_client->size())
                {
                    try
                    {
                        std::lock_guard<M_MUTEX> glock(*tmp_client_mutex);

                        logger->debug(ZMQ_LOG, "\[CLIENT\] there is %d message to send\n", tmp_queue_s_client->size());
                        // check size again under the lock
                        // there maybe context switch issue, to do
                        while (tmp_queue_s_client->size())
                        {
                            (tmp_queue_s_client->front())->send(*tmp_socket);
                            (tmp_queue_s_client->front()).reset();
                            tmp_queue_s_client->pop();
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
                if (tmp_queue_s_client->size() > 0)
                {
                    try
                    {
                        std::lock_guard<M_MUTEX> glock(*tmp_client_mutex);
                        // logger->debug(ZMQ_LOG, "\[CLIENT\] poll timeout, and there is %d message, now send message\n", tmp_queue_s_client->size());
                        // check size again under the lock
                        while (tmp_queue_s_client->size() > 0)
                        {
                            // logger->debug(ZMQ_LOG, "\[CLIENT\] tmp_queue_s_client->size() is %d, (tmp_queue_s_client->front()) is %d\n", tmp_queue_s_client->size(), (tmp_queue_s_client->front()).use_count());
                            (tmp_queue_s_client->front())->send(*tmp_socket);
                            (tmp_queue_s_client->front()).reset();
                            tmp_queue_s_client->pop();
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

void client_base::set_monitor_cb(MONITOR_CB_FUNC cb)
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
bool client_base::monitor_task()
{
    MONITOR_CB_FUNC_CLIENT tmp_cb = monitor_cb;
    void *client_mon = zmq_socket(client_socket_->ctxptr, ZMQ_PAIR);
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

        if (tmp_cb)
        {
            tmp_cb(event, value, address);
        }
    }
}

bool client_base::monitor_this_socket()
{
    int rc = zmq_socket_monitor(client_socket_->ptr, monitor_path.c_str(), ZMQ_EVENT_ALL);
    logger->debug(ZMQ_LOG, "\[CLIENT\] rc is : %d\n", rc);
    return ((rc == 0) ? true : false);
}