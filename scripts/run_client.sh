#!/bin/bash

if [ "$DEBUG" == "true" ]; then
    g++ -I src/include -g src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    gdb ./bin/client $1;

elif [ "$DEBUG_MEM" == "true" ]; then
    g++ -I src/include -g src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    valgrind --leak-check=yes ./bin/client $1;
else
    g++ -I src/include src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    ./bin/client $1;
fi
