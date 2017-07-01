FROM ubuntu:16.04

RUN pwd

RUN ls -ls

RUN ls /home

RUN cd /home

RUN ls 

RUN cd rsyn-x/x/bin

RUN make all 
