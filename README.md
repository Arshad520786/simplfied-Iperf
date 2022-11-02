## Simplified Iperf


Compile with :

```
gcc iperf.c -o iperf
```

Usage : 

- Server : run `Iperfer -s -p <listen port>` with listen port range from 1023 < port < 65536
- Client : run `Iperfer -c -h <server hostname> -p <server port> -t <time>`
		server host : server's IPv4 address 
		server port : server's listen port number
		time	    : duration of test

