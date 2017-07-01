FROM ubuntu:16.04

RUN apt install git-core

RUN git clone https://github.com/jucemarmonteiro/rsyn-x.git

RUN pwd

RUN ls -ls

RUN cd rsyn-x/x/bin

RUN make all 
