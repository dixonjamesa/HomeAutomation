#############################################################################
#
# Makefile for Socket test
#
# Author:  James Dixon
# Date:    02/05/2017 (version 1.0)
#
# Description:
# ------------
# use make all
#

CC=g++
# The recommended compiler flags for the Raspberry Pi
#CFLAGS = -Wall -std=c++11 -pthread
CFLAGS = -Wall -std=c++11 -pthread -Ofast 
#-mfpu=vfp -pthread -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

# define all programs
BIN = TestServer
SRC = $(wildcard *.cpp)
OBJ = $(SRC:%.cpp=%.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) TestServer.o -o TestServer

%.o: %.cpp
	$(CC) $(CFLAGS) -c $(SRC) 

clean:
	rm -f $(OBJ)
	rm -f TestServer
