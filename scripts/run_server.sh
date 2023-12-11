#!/bin/bash

if [ "$DEBUG" == "true" ]; then
    g++ -I src/include -g -O0 src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    gdb ./bin/server;

elif [ "$DEBUG_MEM" == "true" ]; then
    g++ -I src/include -g -O0 src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    valgrind --leak-check=full --track-origins=yes --log-file="./server-vgrind.log" ./bin/server;
else
    g++ -I ./src/include src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    ./bin/server;
fi
