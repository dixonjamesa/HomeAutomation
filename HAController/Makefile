#############################################################################
#
# Makefile for Raspberry Pi Home Automation rf24 to mosquitto controller
#
# Author:  James Dixon
# Date:    26/12/2015 (version 1.0)
#
# Description:
# ------------
# use make all and make install to install the examples
#

installdir := /usr/local

CC=g++
# The recommended compiler flags for the Raspberry Pi
CFLAGS= -Wall -g -pthread -std=c++11 -Ofast
#-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s
DPORT=5001 

# define all programs
OBJECTS = HomeController.o MessageMap.o SimpleServer.o
DEPS = MessageMap.h HomeController.h

all: HomeController
test: TestControl

%.o:%.cpp $(DEPS)
	$(CC) $(CFLAGS) -I../ -I../Libraries/HomeAutomation -DPortNum=$(DPORT) -lrf24-bcm -lrf24network -lmosquittopp -c -o $@ $<

HomeController: $(OBJECTS) $(DEPS)
	$(CC) $(CFLAGS) -I../ -I../Libraries/HomeAutomation -lrf24-bcm -lrf24network -lmosquittopp -o $@ $^ 

TestControl: DPORT=5002
TestControl: $(OBJECTS) $(DEPS)
	$(CC) $(CFLAGS) -I../ -I../Libraries/HomeAutomation -lrf24-bcm -lrf24network -lmosquittopp -o $@ $^ 

clean:
	rm -rf HomeController TestControl *.o

install: all
	test -d $(installdir) || mkdir $(installdir)
	test -d $(installdir)/bin || mkdir $(installdir)/bin
	install -m 0755 HomeController $(installdir)/bin; \

.PHONY: install
