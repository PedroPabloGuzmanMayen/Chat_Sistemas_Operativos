#!/bin/bash

g++ -std=c++17 -Wall \
    client.cpp \
    network.cpp \
    -o chat_client \
    -lfltk -lX11 -lpthread -lwebsockets