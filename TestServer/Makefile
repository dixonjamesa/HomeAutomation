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

VPATH = . ../HAController
CC=g++
# The recommended compiler flags for the Raspberry Pi
#CFLAGS = -Wall -std=c++11 -pthread
CFLAGS = -Wall -std=c++11 -pthread -Ofast 
#-mfpu=vfp -pthread -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

ODIR = obj
INC = -Iinc

# define all programs
BIN = TestServer
SRC = $(wildcard *.cpp) SimpleServer.cpp
IOBJ = $(notdir $(SRC))
OBJ = $(IOBJ:%.cpp=$(ODIR)/%.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o TestServer

$(ODIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $(INC) -o $@ $<

clean:
	rm -f $(OBJ)
	rm -f TestServer
