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

# The recommended compiler flags for the Raspberry Pi
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

# define all programs
PROGRAMS = HomeController
SOURCES = ${PROGRAMS:=.cpp}

all: ${PROGRAMS}

${PROGRAMS}: ${SOURCES}
	g++ ${CCFLAGS} -Wall -I../ -lrf24-bcm -lrf24network -lmosquittopp $@.cpp -o $@

clean:
	rm -rf $(PROGRAMS) *.o

install: all
	test -d $(installdir) || mkdir $(installdir)
	test -d $(installdir)/bin || mkdir $(installdir)/bin
	for prog in $(PROGRAMS); do \
	  install -m 0755 $$prog $(installdir)/bin; \
	done

.PHONY: install