#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <mosquittopp.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include "MessageMap.h"
#include "HomeController.h"

//std::list<mapitem *> all;

void MessageMap::AddMap(const char *c, uint16_t ni, unsigned char tp, unsigned char code, bool isreg /* register or subscribe*/)
{
	mapitem *newitem = new mapitem();
	newitem->channel = new char[strlen(c)+1];
	strcpy(newitem->channel, c);
	newitem->nodeid = ni;
	newitem->type = tp;
	newitem->code = code;
	newitem->isreg = isreg;
	all.push_front(newitem);
	if( !isreg )
	{
		mosquittoBroker->subscribe(0, c);
	}
}
int MessageMap::MatchAll(const char *channel, std::list<mapitem *> *_list, bool isreg)
{
	int count = 0;
	for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
			iterator != end; iterator++)
	{
		mapitem *mi = *iterator;
		if(!strcmp(mi->channel, channel) && mi->isreg == isreg)
		{
			_list->push_front(mi);
			count++;
		}
	}
	return count;

}
mapitem *MessageMap::Match(const char *channel, bool isreg)
{
	for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
			iterator != end; iterator++)
	{
		mapitem *mi = *iterator;
		if(!strcmp(mi->channel, channel) && mi->isreg == isreg )
		{
			return mi;
		}
	}
	return NULL;
}
mapitem *MessageMap::Match(uint16_t nodeid, unsigned char code, bool isreg)
{
	for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
			iterator != end; iterator++)
	{
		mapitem *mi = *iterator;
		if(mi->nodeid == nodeid && mi->code == code && mi->isreg == isreg )
		{
			return mi;
		}
	}
	return NULL;
}
// Remove all messages registered by this node
void MessageMap::RemoveAll(uint16_t nodeid)
{
	for( std::list<mapitem *>::iterator iterator = all.begin(), end = all.end();
			iterator != end; /*deliberately nothing*/ )
	{
		mapitem *mi = *iterator;
		if( mi->nodeid == nodeid )
		{
			if( !mi->isreg )
			{
				//mosquittoBroker->unsubscribe(0, mi->channel);
				// NEED TO CHECK IF ANYBODY ELSE IS ACTUALLY STILL SUBSCRIBED
				// SO FOR NOW, JUST DON'T BOTHER TIDYING UP THE SUBSCRIPTION
			}
			delete mi;
			iterator = all.erase(iterator);
		}
		else
		{
			iterator++;
		}
	}
}

// output all maps to _out
void MessageMap::DumpAll(char *_out, int _buflen)
{
	char tbuf[128];
	int written;
	written = snprintf(_out, _buflen, "<table border=\"1\"><tr><th>Channel</th><th>0=sub, 1=reg</th><th>nodeid</th><th>type</th><th>code</th></tr>");
	for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
			iterator != end; iterator++)
	{
		int len;
		mapitem *mi = *iterator;
		len = sprintf(tbuf, "<tr><td>%s</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>", mi->channel, mi->isreg, mi->nodeid, mi->type, mi->code);
		if(written+len+9 < _buflen)
		{
			written += len;
			strcat(_out, tbuf);
		}
	}
	strcat(_out, "</table>");
}

