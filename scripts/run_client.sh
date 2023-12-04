#!/bin/bash

if [ "$DEBUG" == "true" ]; then
    g++ -g src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    gdb ./bin/client claudinoac dtb-server 6999;

elif [ "$DEBUG_MEM" == "true" ]; then
    g++ -g src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    valgrind --leak-check=yes ./bin/client claudinoac dtb-server 6999;
else
    g++ src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    ./bin/client claudinoac dtb-server 6999;
fi
