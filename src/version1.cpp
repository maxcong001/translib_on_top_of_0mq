#if 0
#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include <zmq.hpp>
#include "zhelpers.hpp"

class client_base
{
  public:
    client_base()
        : ctx_(1),
          client_socket_(ctx_, ZMQ_DEALER)
    {
    }

    size_t send(char *msg, size_t len)
    {
        client_socket_.send(msg, len);
    }

    void run()
    {
        auto routine_fun = std::bind(&client_base::start, this);
        std::thread routine_thread(routine_fun);
        routine_thread.detach();
    }

  private:
    void start()
    {
        /*
        // generate random identity
        char identity[10] = {};
        sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
        printf("%s\n", identity);
        client_socket_.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
        */

        int linger = 0;
        client_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));

        /*
        - Change the ZMQ_TIMEOUT?for ZMQ_RCVTIMEO and ZMQ_SNDTIMEO.
        - Value is an uint32 in ms (to be compatible with windows and kept the
        implementation simple).
        - Default to 0, which would mean block infinitely.
        - On timeout, return EAGAIN.
        Note: Maxx will this work for DEALER mode?
        */
        int iRcvTimeout = 5000; // millsecond Make it configurable

        if (zmq_setsockopt(client_socket_, ZMQ_RCVTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
        {
            zmq_close(client_socket_);
            zmq_ctx_destroy(ctx_);
            return 0;
        }

        client_socket_.connect("tcp://localhost:5570");
        //  Initialize poll set
        zmq::pollitem_t items[] = {{client_socket_, 0, ZMQ_POLLIN, 0}};
        while (1)
        {
            try
            {
                zmq::poll(items, 1, -1);
                if (items[0].revents & ZMQ_POLLIN)
                {
                    zmsg msg(client_socket_);
                    // ToDo: now we got the message, do main work
                    std::cout << "receive message form server" << std::endl;
                }
            }
            catch (std::exception &e)
            {
            }
        }
    }

  private:
    zmq::context_t ctx_;
    zmq::socket_t client_socket_;
};
class server_base
{
  public:
    server_base()
        : ctx_(1),
          server_socket_(ctx_, ZMQ_ROUTER)
    {
    }

    void run()
    {
        auto routine_fun = std::bind(&server_base::start, this);
        std::thread routine_thread(routine_fun);
        routine_thread.detach();
    }

  private:
    void start()
    {
        server_socket_.bind("tcp://localhost:5570");
        //  Initialize poll set
        zmq::pollitem_t items[] = {
            {server_socket_, 0, ZMQ_POLLIN, 0},
        };
        while (1)
        {
            try
            {
                zmq::poll(items, 1, -1);
                if (items[0].revents & ZMQ_POLLIN)
                {
                    zmsg msg(server_socket_);
                    // ToDo: now we got the message, do main work
                    std::cout << "receive message form server" << std::endl;
                    // send back message to client, for test
                    msg.send(server_socket_);
                }
            }
            catch (std::exception &e)
            {
            }
        }
    }
    size_t send(char *msg, size_t len)
    {
        client_socket_.send(msg, len);
    }

  private:
    zmq::context_t ctx_;
    zmq::socket_t server_socket_;
};
int main(void)
{
    client_base ct1;
    //client_base ct2;
    server_base st1;
    st1.run();
    ct1.run();
    std::string test_str("this is a test");
    ct1.send(test_str.c_str(), test_str.size());

    return 0;
}
#endif