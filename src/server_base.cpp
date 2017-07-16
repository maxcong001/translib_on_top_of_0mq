#include "server_base.hpp"

bool server_base::run()
{
    std::shared_ptr<std::map<void *, zmsg_ptr>> tmp_map(new std::map<void *, zmsg_ptr>());
    Id2MsgMap_server = tmp_map;

    std::shared_ptr<std::queue<zmsg_ptr>> tmp_queue(new std::queue<zmsg_ptr>());
    server_q = tmp_queue;

    std::shared_ptr<std::mutex> tmp_server_mutex(new std::mutex);
    server_mutex = tmp_server_mutex;

    ctx_ = new zmq::context_t(1);
    if (!ctx_)
    {
        logger->error(ZMQ_LOG, "\[SERVER\] new context fail!\n");
        return false;
    }

    server_socket_ = new zmq::socket_t(*ctx_, ZMQ_ROUTER);
    if (!server_socket_)
    {
        logger->error(ZMQ_LOG, "\[SERVER\] new socket fail!\n");
        return false;
    }

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
        logger->debug(ZMQ_LOG, "\[SERVER\] start monitor socket success!\n");
    }
    else
    {
        logger->error(ZMQ_LOG, "\[SERVER\] start monitor socket fail!\n");
        return false;
    }
    return ret;
}

// if return 0, that means send fail, please send again;
size_t server_base::send(const char *msg, size_t len, void *ID)
{
    std::lock_guard<M_MUTEX> glock(*server_mutex);

    auto iter = Id2MsgMap_server->find(ID);
    if (iter != Id2MsgMap_server->end())
    {
        zmsg::ustring tmp_ustr((unsigned char *)msg, len);
        (iter->second)->push_back(tmp_ustr);
        {
            server_q->emplace(iter->second);
        }
        Id2MsgMap_server->erase(iter);
        logger->debug(ZMQ_LOG, "\[SERVER\] send message to client\n");
    }
    else
    {
        logger->error(ZMQ_LOG, "\[SERVER\] did not find the ID\n");
        return -1;
    }
    return len;
}

bool server_base::monitor_task()
{
    MONITOR_CB_FUNC_CLIENT tmp_monitor_cb = monitor_cb;
    
    void *server_mon = zmq_socket(server_socket_->ctxptr, ZMQ_PAIR);
    if (!server_mon)
    {
        logger->error(ZMQ_LOG, "\[SERVER\] get 0MQ socket fail\n");
        return false;
    }

    int rc = zmq_connect(server_mon, monitor_path.c_str());
    //rc should be 0 if success
    if (rc)
    {
        logger->error(ZMQ_LOG, "\[SERVER\] connect fail\n");
        return false;
    }
    while (1)
    {  
        if (should_exit_monitor_task)
        {
            zmq_close(server_mon);
            logger->warn(ZMQ_LOG, "\[SERVER\] monitor task will exit\n");
            return true;
        }
        std::string address;
        int value;
        int event = get_monitor_event(server_mon, &value, address);
        if (event == -1)
        {
            logger->error(ZMQ_LOG, "\[SERVER\] get monitor event fail\n");
            //return false;
        }
        if (tmp_monitor_cb)
        {
            tmp_monitor_cb(event, value, address);
        }
    }
}

bool server_base::start()
{
    SERVER_CB_FUNC *tmp_cb = cb_;
    zmq::socket_t *tmp_socket = server_socket_;
    zmq::context_t *tmp_ctx = ctx_;
    std::shared_ptr<std::map<void *, zmsg_ptr>> tmp_Id2MsgMap_server = Id2MsgMap_server;
    std::shared_ptr<std::queue<zmsg_ptr>> tmp_server_q = server_q;
    std::shared_ptr<std::mutex> tmp_server_mutex_ = server_mutex;
    //std::mutex* server_mutex;
    try
    {
        // enable IPV6, we had already make sure that we are using TCP then we can set this option
        int enable_v6 = 1;
        tmp_socket->setsockopt(ZMQ_IPV6, &enable_v6, sizeof(enable_v6));
        /*Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.*/
        int iRcvSendTimeout = 5000; // millsecond Make it configurable
        tmp_socket->setsockopt(ZMQ_RCVTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
        tmp_socket->setsockopt(ZMQ_SNDTIMEO, &iRcvSendTimeout, sizeof(iRcvSendTimeout));
        int linger = 0;
        //tmp_socket->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    }
    catch (std::exception &e)
    {
        logger->error(ZMQ_LOG, "\[SERVER\] set socket option return fail\n");
        return false;
    }
    try
    {
        if (IP_and_port.empty())
        {
            logger->error(ZMQ_LOG, "\[SERVER\] please make sure you had set the IP and port info\n");
            return false;
        }
        std::string tmp;
        tmp += protocol + IP_and_port;
        tmp_socket->bind(tmp);
    }
    catch (std::exception &e)
    {
        logger->error(ZMQ_LOG, "\[SERVER\]  socket bind error\n");
        return false;
    }
    logger->debug(ZMQ_LOG, "\[SERVER\] bind success\n");
    //  Initialize poll set
    zmq::pollitem_t items[] = {
        {*tmp_socket, 0, ZMQ_POLLIN, 0}};
    while (1)
    {
        if (should_exit_routine_task)
        {
            try
            {
                int linger = 0;
                tmp_socket->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
            }
            catch (std::exception &e)
            {
                logger->error(ZMQ_LOG, "\[SERVER\] set ZMQ_LINGER return fail\n");
            }

            tmp_socket->close();

            tmp_ctx->close();
            //tmp_server_q->clear();

            logger->warn(ZMQ_LOG, "\[SERVER\]  server routine task will exit\n");
            return true;
        }
        try
        {
            // by default we wait for 500ms then so something. like hreatbeat
            zmq::poll(items, 1, EPOLL_TIMEOUT);
            if (should_exit_routine_task)
            {
                try
                {
                    int linger = 0;
                    tmp_socket->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
                }
                catch (std::exception &e)
                {
                    logger->error(ZMQ_LOG, "\[SERVER\] set ZMQ_LINGER return fail\n");
                }

                tmp_socket->close();
                tmp_ctx->close();
                //tmp_server_q->clear();

                logger->warn(ZMQ_LOG, "\[SERVER\]  server routine task will exit\n");
                return true;
            }

            if (items[0].revents & ZMQ_POLLIN)
            {

                zmsg_ptr msg(new zmsg(*tmp_socket));
                std::string data = msg->get_body();
                if (data.empty())
                {
                    logger->warn(ZMQ_LOG, "\[SERVER\]  get a message without body\n");
                    continue;
                }
                void *ID = getUniqueID();
                {
                    std::lock_guard<M_MUTEX> glock(*tmp_server_mutex_);
                    if (!tmp_Id2MsgMap_server.use_count())
                    {
                        logger->error(ZMQ_LOG, "\[SERVER\] invalid map!\n");
                        continue;
                    }
                    tmp_Id2MsgMap_server->emplace(ID, msg);
                }
                // ToDo: now we got the message, do main work
                //std::cout << "receive message form client" << std::endl;
                //msg.dump();
                // send back message to client, for test
                //msg.send(tmp_socket);
                if (tmp_cb)
                {
                    tmp_cb(data.c_str(), data.size(), ID);
                }
                else
                {
                    logger->warn(ZMQ_LOG, "\[SERVER\]  no invalid callback function, please make sure you had set it\n");
                }

                if (tmp_server_q->size())
                {
                    try
                    {
                        std::lock_guard<M_MUTEX> glock(*tmp_server_mutex_);
                        logger->debug(ZMQ_LOG, "\[SERVER\] there is %d message, now send message\n", tmp_server_q->size());
                        // check size again under the lock
                        while (tmp_server_q->size())
                        {
                            (tmp_server_q->front())->send(*tmp_socket);

                            //logger->error(ZMQ_LOG, "\[SERVER\] tmp_server_q size is %d, reference count is %d\n", tmp_server_q->size(), (tmp_server_q->front()).use_count());
                            // make sure the the reference count is 0
                            (tmp_server_q->front()).reset();
                            tmp_server_q->pop();
                        }
                    }
                    catch (std::exception &e)
                    {
                        logger->error(ZMQ_LOG, "\[SERVER\] send message fail!\n");
                        continue;
                    }
                }
            }
            // poll time out, now send message if there is one
            else
            {
                if (tmp_server_q->size())
                {
                    try
                    {
                        std::lock_guard<M_MUTEX> glock(*tmp_server_mutex_);
                        logger->debug(ZMQ_LOG, "\[SERVER\] poll timeout, and there is %d message, now send message\n", tmp_server_q->size());
                        // check size again under the lock
                        while (tmp_server_q->size())
                        {
                            (tmp_server_q->front())->send(*tmp_socket);
                            // make sure the the reference count is 0
                            (tmp_server_q->front()).reset();
                            tmp_server_q->pop();
                        }
                    }
                    catch (std::exception &e)
                    {
                        logger->error(ZMQ_LOG, "\[SERVER\] send message fail!\n");
                        continue;
                    }
                }
            }
        }
        catch (std::exception &e)
        {
            logger->error(ZMQ_LOG, "\[SERVER\] catch exception. epoll error\n");
        }
    }
    // should never return
    return false;
}