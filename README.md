[MMQ - An transport library based on 0MQ](https://github.com/maxcong001/translib_on_top_of_0mq)
===================================

Copyright Max Cong.

# Documentation

You can find more detailed documentation and examples in the [doc](doc) and [example](example) directories respectively.

# Installation & Testing

See [INSTALL](INSTALL.md) for installation instructions for various platforms.


See [test](test) for more guidance on how to run various test suites (e.g. unit tests, interop tests, benchmarks)

# project Status


| time        | change                                                | version |
|-------------|-------------------------------------------------------|---------|
| 2017/06/27  | add heart beat and worker recovery                    | 1.2     |

# Known issue
### If client send a mess number of messages(as we are async, these message will "send" immediately), then the client will exit with sig-abort. This is cause by 0MQ, when a larg amount of messages come at the same time, the link list corrupted.
Now we had test send message 4,000/s, that is fine. 
note: please do not write code like below(send to many messages in a while/for loop, we are async, will "send" immediately):
```
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		for (int i = 0; i < 1000; i++)
		{
			ct1.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
		}
````
### If client send message too fast, sometimes the program will hang at "poll" or "epoll wait". This is cause by 0MQ, that when we send a message, even in the async mode, it will wait for a "response". It hang waiting for "response" even we set the send/receive timeout to 5s.

# Overview


MMQ provide a useful abstraction for communication via TCP. MMQ enable communication between clients and servers using a simple API.


## Interface


Developers using MMQ typically start by setting some configuration(IP,callback function...). Then just call run() function. After that, you can call send() to send your message. Other thing, MMQ will handle.

#### Full asynchronous
Synchronous calls block until a response arrives from the server. This mode is slow. MMQ is full asynchronous.

# Usage

MMQ is pretty easy to use, just see the [example](example) 



## Connection interface
An interface may be specified by either of the following:

    The wild-card *, meaning all available interfaces.
    The primary IPv4 or IPv6 address assigned to the interface, in its numeric representation.
    The non-portable interface name as defined by the operating system.

The TCP port number may be specified by:

    A numeric value, usually above 1024 on POSIX systems.
    The wild-card *, meaning a system-assigned ephemeral port.

### Examples:

Assigning a local address to a socket
```
// TCP port 5555 on all available interfaces
rc = zmq_bind(socket, "tcp://*:5555");
assert (rc == 0);
// TCP port 5555 on the local loop-back interface on all platforms
rc = zmq_bind(socket, "tcp://127.0.0.1:5555");
assert (rc == 0);
// TCP port 5555 on the first Ethernet network interface on Linux
rc = zmq_bind(socket, "tcp://eth0:5555"); assert (rc == 0);
```
Connecting a socket
```
// Connecting using an IP address
rc = zmq_connect(socket, "tcp://192.168.1.1:5555");
assert (rc == 0);
// Connecting using a DNS name
rc = zmq_connect(socket, "tcp://server1:5555");
assert (rc == 0);
// Connecting using a DNS name and bind to eth1
rc = zmq_connect(socket, "tcp://eth1:0;server1:5555");
assert (rc == 0);
// Connecting using a IP address and bind to an IP address
rc = zmq_connect(socket, "tcp://192.168.1.17:5555;192.168.1.1:5555"); assert (rc == 0);
```

