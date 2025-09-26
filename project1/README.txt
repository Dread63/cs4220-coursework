Joshua Douglas - jdougla8@uccs.edu

Statement of Work:
I have neither given nor received unauthorized assistance on this work

Description:
This assignment is an example of a server and client tcp socket which transfers a given text file.
I solved this problem by following beej's networking socket programming guide as well as other TCP socket
programming guides online. Using I started by opening a socket, binding it, and listening on the defined port.
After that I connected to the open socket on my client and began to receive data fromt he server socket. I used a
nested while loop to implemented the stop and wait ARQ which would re-transmit a file on timeout or an incorrect ACK.

How To:
Inside a terminal in the root directory of this project, three different make commands can be used. These include the following:
1. `make all` : builds both server and client binaries
2. `make server` : builds server binary
3. `make client` : builds client binary 

Lessons Learned:
Before I began this project, I had no network socket programming experience at all. I learned how to manage sockets and
how the netowrk structures in C work. However, the greatest challenge for me was figuring out how to create the stop-and-wait
ARQ functionality. I re-wrote the code multiple times, but ended up deciding to send the 1 bit ACK data at the head of the data
frame rather than as a separate frame. After making this decision, the implementation became much easier and I just had to overcome
validating the current ACK on both ends. I learned how to reading into an array starting after the first index and how to pull the
ACK data out separately from the file data on the client side. Another challeneging aspect of this project was deciding how to implement
the timer for the ACK requests. In hindsight I should have looked at the future lecture slides sooner, because Sully went over it in class.
But, I was working ahead ont he project and ended up wasting time on some rabit trails for timers in C. Overall this project was quite
challending for me, but taught me a lot about networking socket programming and how sockets function.

Test Case:
I generated a text file with 5000 lines of text using ChatGPT. I wanted to make sure that my server and client could handle the situation where
a file was larger than one frame size (1025 bytes). I also tested the code with shorter files to ensure each arrived in the client_files directory
in full. All tests completed as expected.

Resources:
https://beej.us/guide/bgnet/
https://www.geeksforgeeks.org/computer-networks/stop-and-wait-arq/
https://mrvataliya.blogspot.com/2016/07/implementation-of-stop-and-wait.html
https://github.com/kziraddin/stop-and-wait-protocol-in-C
https://thelinuxcode.com/use-select-system-call-c/
https://github.com/benton-gray/uccs_networking_project/blob/master/tcp_client.c