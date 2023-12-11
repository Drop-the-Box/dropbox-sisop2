#!/bin/bash

if [ "$DEBUG" == "true" ]; then
    g++ -I src/include -g src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    gdb ./bin/client $1 $2 $3;

elif [ "$DEBUG_MEM" == "true" ]; then
    g++ -I src/include -g src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    valgrind --leak-check=yes ./bin/client $1 $2 $3;
else
    g++ -I src/include src/client/**/*.cpp src/common/**/*.cpp src/client/main.cpp -o ./bin/client;
    echo $1;
    echo $2;
    echo $3;
    ./bin/client $1 $2 $3;
fi
