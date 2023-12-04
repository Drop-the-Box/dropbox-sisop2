#!/bin/bash

if [ "$DEBUG" == "true" ]; then
    g++ -g src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    gdb ./bin/server;

elif [ "$DEBUG_MEM" == "true" ]; then
    g++ -g src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    valgrind --leak-check=yes ./bin/server;
else
    g++ src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    ./bin/server;
fi
