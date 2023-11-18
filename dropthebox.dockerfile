FROM ubuntu:22.04

RUN apt-get update && apt-get upgrade -y && apt-get install build-essential -y

RUN mkdir /code
COPY . /code

WORKDIR /code
CMD g++ -pthread src/main.cpp -o dropthebox && ./dropthebox
