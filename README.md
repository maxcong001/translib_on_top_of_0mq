[MMQ - An transport library based on 0MQ](https://github.com/maxcong001/translib_on_top_of_0mq/edit/master/README.md)
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


# Overview


MMQ provide a useful abstraction for communication via TCP. MMQ enable communication between clients and servers using a simple API.


## Interface


Developers using MMQ typically start by setting some configuration(IP,callback function...). Then just call run() function. After that, you can call send() to send your message. Other thing, MMQ will handle.

#### Full asynchronous
Synchronous calls block until a response arrives from the server. This mode is slow. MMQ is full asynchronous.

# Usage

MMQ is pretty easy to use, just see the [example](example) 


