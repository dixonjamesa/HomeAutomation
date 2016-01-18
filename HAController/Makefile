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
CFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

# define all programs
OBJECTS = HomeController.o MessageMap.o
DEPS = MessageMap.h HomeController.h

all: HomeController

%.o:%.c $(DEPS)
	$(CC) -Wall -I../ -I../Libraries/HomeAutomation -lrf24-bcm -lrf24network -lmosquittopp -c -o $@ $< $(CFLAGS)  

HomeController: $(OBJECTS) $(DEPS)
	$(CC) -Wall -I../ -I../Libraries/HomeAutomation -lrf24-bcm -lrf24network -lmosquittopp -o $@ $^ $(CFLAGS) 

clean:
	rm -rf $(PROGRAMS) *.o

install: all
	test -d $(installdir) || mkdir $(installdir)
	test -d $(installdir)/bin || mkdir $(installdir)/bin
	install -m 0755 HomeController $(installdir)/bin; \

.PHONY: install
