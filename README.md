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
| 2017/07/07  | add Destructor                                        | 1.2     |
| 2017/07/05  | change send/receive to one thread                     | 1.2     |
| 2017/06/27  | add heart beat and worker recovery                    | 1.2     |

# Known issue
while using dynamic lib, when the programm exit, there will be SIGABORT. 
this is caused as when exit, the destructor will free the queue of shared_ptr(in server and worker).
when the dynamic lib exit, the memory is deleted??

I am busy on other things, will fix this later, this should not affect your program.

```
#0  0x00007ffff6b6e1d7 in raise () from /lib64/libc.so.6
#1  0x00007ffff6b6f8c8 in abort () from /lib64/libc.so.6
#2  0x00007ffff6badf07 in __libc_message () from /lib64/libc.so.6
#3  0x00007ffff6bb5503 in _int_free () from /lib64/libc.so.6
#4  0x00000000004510b4 in __gnu_cxx::new_allocator<std::shared_ptr<zmsg> >::deallocate (this=0x6666c8 <wk1+200>,
    __p=0x67b7b0) at /usr/include/c++/4.8.2/ext/new_allocator.h:110
#5  0x000000000044fa04 in std::_Deque_base<std::shared_ptr<zmsg>, std::allocator<std::shared_ptr<zmsg> > >::_M_deallocate_node (this=0x6666c8 <wk1+200>, __p=0x67b7b0) at /usr/include/c++/4.8.2/bits/stl_deque.h:539
#6  0x000000000044cd58 in std::_Deque_base<std::shared_ptr<zmsg>, std::allocator<std::shared_ptr<zmsg> > >::_M_destroy_nodes (this=0x6666c8 <wk1+200>, __nstart=0x67b308, __nfinish=0x67b310) at /usr/include/c++/4.8.2/bits/stl_deque.h:642
#7  0x0000000000449ed3 in std::_Deque_base<std::shared_ptr<zmsg>, std::allocator<std::shared_ptr<zmsg> > >::~_Deque_base (
    this=0x6666c8 <wk1+200>, __in_chrg=<optimized out>) at /usr/include/c++/4.8.2/bits/stl_deque.h:565
#8  0x000000000044796b in std::deque<std::shared_ptr<zmsg>, std::allocator<std::shared_ptr<zmsg> > >::~deque (
    this=0x6666c8 <wk1+200>, __in_chrg=<optimized out>) at /usr/include/c++/4.8.2/bits/stl_deque.h:918
#9  0x0000000000440488 in std::queue<std::shared_ptr<zmsg>, std::deque<std::shared_ptr<zmsg>, std::allocator<std::shared_ptr<zmsg> > > >::~queue (this=0x6666c8 <wk1+200>, __in_chrg=<optimized out>) at /usr/include/c++/4.8.2/bits/stl_queue.h:93
#10 0x0000000000445840 in worker_base::~worker_base (this=0x666600 <wk1>, __in_chrg=<optimized out>)
    at /home/mcong/study/github/translib_on_top_of_0mq/include/worker.hpp:45
#11 0x00007ffff6b71dba in __cxa_finalize () from /lib64/libc.so.6
#12 0x00007ffff79af433 in __do_global_dtors_aux () from /home/mcong/study/github/translib_on_top_of_0mq/lib/libM_empty.so.1
#13 0x00007fffffffe1d0 in ?? ()
#14 0x00007ffff7dec85a in _dl_fini () from /lib64/ld-linux-x86-64.so.2
```

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

