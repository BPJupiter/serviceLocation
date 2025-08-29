# Determine Service Location

Currently, you can download and compile the file 'ping.c' in flaskr/static/c/.
Despite the name, it is an implementation of traceroute using ICMP echo requests.
```shell
$ gcc ping.c -o ping.out -lm
```
Usage is:
```shell
$ sudo ./ping.out <ipv4addr> [pingsperhop] [show_hops]
```
root access is required as program uses raw sockets.
