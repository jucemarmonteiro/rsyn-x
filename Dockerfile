WORKDIR /home

FROM ubuntu:16.04

RUN pwd

RUN ls -ls

RUN cd rsyn-x/x/bin

RUN make all 
