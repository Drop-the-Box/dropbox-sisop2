FROM ubuntu:22.04

RUN apt-get update && apt-get install -y build-essential g++ gdb libstdc++6-10-dbg valgrind
RUN mkdir /code
RUN mkdir /code/bin

EXPOSE 6999 

WORKDIR /code
