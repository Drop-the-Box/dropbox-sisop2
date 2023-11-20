FROM ubuntu:22.04

RUN apt-get update && apt-get upgrade -y && apt-get install build-essential -y && apt-get install -y gdb

RUN mkdir /code
COPY . /code

EXPOSE 6999 

WORKDIR /code
