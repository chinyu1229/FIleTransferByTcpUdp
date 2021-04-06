# FileTransferByTcpUdp
socket programming

## envirnoment
Ubuntu 20.04.2 LTS (GNU/Linux 5.8.0-44-generic x86_64)
## compiler
gcc

### TCP Sender
    gcc file_tranfer.c -o sender
    ./sender tcp send \<ip\> \<port\> \<file\>
### TCP receiver
    gcc file_tranfer.c -o receiver
    ./sender tcp recv \<ip\> \<port\>
 
### UDP Sender
    gcc file_tranfer.c -o sender
    ./sender udp send \<ip\> \<port\> \<file\>
### UDP receiver
    gcc file_tranfer.c -o receiver
    ./sender udp recv \<ip\> \<port\>
  
### Log
Print the transfer log, total transfer time, file size on teriminal screen in sender

### check
Use md5sum to provide the file transfer to receiver is ordered and correct
