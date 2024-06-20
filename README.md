# Benchmarking Throughput designed to measure throuput connection Bytes/Seconds [Bps] over TCP connection.
## Execution must be in the following order:
First deploy server listen to a port, second - execute the code from a client.
Execution in the wrong order will leads to failture.
### Server:
./iperf_adi [-s] [-p] <port>

Note: Execute the following is equivalent: ./iperf [-p] <port>

### Client:
./iperf_adi [-c] <server_ip_address> [-p] <server_port>

Note: In order to execute in client mode, [-c] must be specified, since server mode is running by default.

Client sends to server from 1 byte to 1MB where window grows exponentially over TCP, and print to console: <br>
<bytes_sent>	<time_measured>	BPS <br>
.<br>
.<br>
.<br>


