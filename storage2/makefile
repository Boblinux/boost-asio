BINARIES = main

CC = g++
CXXFLAGS += -std=c++11 -g -Wall -pthread -lboost_system -lssl -lcrypto -I /usr/local/opt/openssl@1.1/include -L /usr/local/opt/openssl@1.1/lib
all: $(BINARIES)

main : main.cc