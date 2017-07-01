FROM ubuntu:16.04

RUN pwd

RUN ls -ls

RUN ls home

RUN cd rsyn-x/x/bin

RUN make all 
