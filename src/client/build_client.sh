#!/bin/bash

g++ -std=c++17 -Wall \
    main.cpp \
    client.cpp \
    protocol.cpp \
    gui_chat.cpp \
    -o chat_client \
    -lfltk -lX11 -lpthread