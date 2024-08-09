# WebServer

This is my personal WebServer implemented by C++17. It has following features:  
- Thread pool
- Proactor
- Support for GET and POST methods of HTTP
- Synchronous or asynchronous log system
- Support for the register and logging in of clients

## Environments

- Ubuntu 22.04 LTS
- Aliyun Cloud Server, 2 (virtual) CPU, 2 GiB Memory, 3 Mbps Network Bandwidth
- Gcc 11.4.0
- MYSQL 8.0

## How to use

`mkdir build`
`cd build`
`cmake ..`
`make`
`./server`

## Notes
The server run on 9006 port by default

## Reference
I use the following repo as reference:
https://github.com/qinguoyi/TinyWebServer.git
