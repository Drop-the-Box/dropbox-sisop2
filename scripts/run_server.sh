#!/bin/bash
echo $PID
if [ "$DEBUG" == "true" ]; then
    g++ -I src/include -g -O0 src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    gdb ./bin/server 6999 $PID;

elif [ "$DEBUG_MEM" == "true" ]; then
    g++ -I src/include -g -O0 src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    valgrind --leak-check=full --track-origins=yes --log-file="./server-vgrind.log" ./bin/server 6999 $PID;
else
    g++ -I ./src/include src/server/**/*.cpp src/common/**/*.cpp src/server/main.cpp -o ./bin/server;
    ./bin/server 6999 $PID;
fi
