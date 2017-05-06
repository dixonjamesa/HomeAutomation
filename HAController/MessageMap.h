#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <mosquittopp.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include "../Libraries/HomeAutomation/HACommon.h"

#ifndef __MESSAGEMAP_H
#define __MESSAGEMAP_H

struct mapitem
{
	char *channel; // name of channel
	bool isreg; // register or a subscribe
	uint16_t nodeid; // node id
	unsigned char type; // DT_xxx from HACommon.h
	unsigned char code; // id to send with message
};

// class to handle storage and retrieval of the message maps
// Basically a very simplistic dictionary
class MessageMap
{
	std::list<mapitem *> all;

	public:
	void AddMap(const char *c, uint16_t ni, unsigned char tp, unsigned char code, bool isreg /* register or subscribe*/);
	int MatchAll(const char *channel, std::list<mapitem *> *_list, bool isreg);
	mapitem *Match(const char *channel, bool isreg);
	mapitem *Match(uint16_t nodeid, unsigned char code, bool isreg);
	// Remove all messages registered by this node
	void RemoveAll(uint16_t nodeid);
	// log out all mapped items
	void DumpAll(char *_out, int _buflen);
};

#endif